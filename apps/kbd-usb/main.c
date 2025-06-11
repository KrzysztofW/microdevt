/*
 * microdevt - Microcontroller Development Toolkit
 *
 * Copyright (c) 2024, Krzysztof Witek
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <eeprom.h>
#include <watchdog.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/buf.h>
#include "gpio.h"
#include "payload-script.h"
#include "rf.h"
#ifdef USB
#include <log.h>
#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"

#ifdef KBD
#define KBD_INITIAL_DELAY 10000
static uint32_t kbd_delay = KBD_INITIAL_DELAY;
static tim_t kbd_timer = TIMER_INIT(kbd_timer);
static uint8_t kbd_tim_delay;
static uint8_t kbd_run;
static uint8_t jitter_on;
static uint8_t led_report;
static uint8_t wait_for;
static uint8_t wait_for_mask;
static uint8_t inject_mode;
#endif
#endif
static tim_t rf_timer = TIMER_INIT(rf_timer);
static rf_ctx_t rf_ctx;
static uint8_t rf_cmd[] = {
	0x04, 0x70, 0x82,
};
#ifdef USB
static uint8_t EEMEM persistent_payload_data[PAYLOAD_DATA_LENGTH];
static uint8_t serial_data_started;
static uint16_t serial_data_written;
#define STORAGE_DATA_OFFSET 2
static uint16_t payload_pos = STORAGE_DATA_OFFSET;
static script_hdr_t script_hdr;
static int8_t magic_pos;
unsigned int rnd_seed;

#ifdef KBD
static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];
#endif
/* USB Class driver callback function prototypes */
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
#ifdef KBD

/* LUFA USB Class driver structure for the HID keyboard interface */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface = {
	.Config = {
		.InterfaceNumber          = INTERFACE_ID_Keyboard,
		.ReportINEndpoint         = {
			.Address          = KEYBOARD_EPADDR,
			.Size             = KEYBOARD_EPSIZE,
			.Banks            = 1,
		},
		.PrevReportINBuffer       = PrevKeyboardHIDReportBuffer,
		.PrevReportINBufferSize   = sizeof(PrevKeyboardHIDReportBuffer),
	},
};
#endif
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
	.Config = {
		.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
		.DataINEndpoint           = {
			.Address          = CDC_TX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.DataOUTEndpoint          = {
			.Address          = CDC_RX_EPADDR,
			.Size             = CDC_TXRX_EPSIZE,
			.Banks            = 1,
		},
		.NotificationEndpoint     = {
			.Address          = CDC_NOTIFICATION_EPADDR,
			.Size             = CDC_NOTIFICATION_EPSIZE,
			.Banks            = 1,
		},
	},
};

void EVENT_USB_Device_Connect(void)
{
}

void EVENT_USB_Device_Disconnect(void)
{
}
static int putchar0(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r') {
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, '\r');
		CDC_Device_SendByte(&VirtualSerial_CDC_Interface, '\n');
	}
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, c);
	return 0;
}

static FILE stream0 =
	FDEV_SETUP_STREAM(putchar0, NULL, _FDEV_SETUP_RW);

void init_stream0(FILE **out_fd, FILE **in_fd, uint8_t interrupt)
{
	*out_fd = &stream0;
	*in_fd = &stream0;
}

#endif

#if defined(USB) && defined(KBD)
static void tim_kbd_cb(void *arg)
{
	if (jitter_on) {
		srand(rnd_seed);
		kbd_delay = (uint32_t)rand() * 8 + KBD_INITIAL_DELAY;
	} else
		kbd_delay = KBD_INITIAL_DELAY;
	timer_reschedule(&kbd_timer, kbd_delay);
	kbd_tim_delay = 1;
}

static void tim_run_cb(void *arg)
{
	kbd_run = 1;
	timer_add(&kbd_timer, 0, tim_kbd_cb, NULL);
}

static int8_t storage_has_more(void)
{
	return payload_pos < script_hdr.size + STORAGE_DATA_OFFSET;
}

static void storage_set_next_byte(void)
{
	payload_pos++;
}

static void storage_set_pos(uint16_t pos)
{
	payload_pos = pos;
}

static void storage_inc(uint16_t nb)
{
	payload_pos += nb;
}

static void storage_get_cur_byte(uint8_t *byte)
{
	eeprom_load(byte, persistent_payload_data + payload_pos, 1);
}

static int storage_get_next_byte(uint8_t *byte)
{
	if (!storage_has_more())
		return -1;
	storage_set_next_byte();
	eeprom_load(byte, persistent_payload_data + payload_pos, 1);
	return 0;
}

static void storage_reset_pos(void)
{
	payload_pos = STORAGE_DATA_OFFSET;
}

static int storage_check_size(void)
{
	eeprom_load(&script_hdr.size, &persistent_payload_data,
		    sizeof(script_hdr.size));
	if (script_hdr.size == 0xFFFF ||
	    script_hdr.size > PAYLOAD_DATA_LENGTH) {
		script_hdr.size = 0;
		return -1;
	}
	return 0;
}

static void serial_parse_byte(uint8_t byte)
{
	uint8_t ret;

	if (byte == SERIAL_DATA_START_SEQ) {
		memset(&script_hdr, 0, sizeof(script_hdr_t));
		magic_pos = 0;
		serial_data_started = 1;
		serial_data_written = 0;
		return;
	}
	if (!serial_data_started)
		return;

	if (byte == SERIAL_DATA_END_SEQ) {
		if (storage_check_size() < 0)
			ret = SERIAL_DATA_INVALID;
		else
			ret = SERIAL_DATA_OK;
		serial_data_started = 0;
		storage_reset_pos();
		goto end;
	}
	if (serial_data_written >= PAYLOAD_DATA_LENGTH) {
		ret = SERIAL_DATA_STORAGE_ERROR;
		goto end;
	}
	if (script_hdr.magic == PAYLOAD_DATA_MAGIC) {
		eeprom_update(persistent_payload_data +
			      serial_data_written, &byte, 1);
		serial_data_written++;
	} else if (magic_pos < sizeof(script_hdr.magic)) {
		uint8_t *m = (uint8_t *)&script_hdr.magic;

		m[magic_pos++] = byte;
	} else {
		ret = SERIAL_DATA_INVALID;
		goto end;
	}
	return;
 end:
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ret);
	serial_data_started = 0;
}
#endif

static int rf_cb(buf_t *buf);
static void rf_timer_cb(void *arg)
{
	rf_init(&rf_ctx, rf_cb);
}
static int rf_cb(buf_t *buf)
{
	if (memcmp(rf_cmd, buf->data, RF_CMD_LEN) == 0) {
#ifdef KBD
		kbd_delay = KBD_INITIAL_DELAY;
		kbd_run = 1;
#endif
		timer_add(&rf_timer, 1000000, rf_timer_cb, NULL);
		return -1; /* -1 disables receiving */
	}
	return 0;
}

int main(void)
{
	timer_subsystem_init();
	watchdog_shutdown();
#if defined(USB) && defined(KBD)
	storage_check_size();
#endif
	rf_init(&rf_ctx, rf_cb);
#ifdef USB
	USB_Init();
	init_stream0(&stdout, &stdin, 1);
#ifdef KBD
	timer_add(&kbd_timer, 3000000, tim_run_cb, NULL);
#endif
#endif
	gpio_init();
	gpio_led_off();
	irq_enable();

	while (1) {
#ifdef USB
		uint8_t c;

		/* Must throw away unused bytes from the host,
		 * or it will lock up while waiting for the device */
		c = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (c != 0xff) {
#ifdef KBD
			serial_parse_byte(c);
#endif
			LOG("%c", c);

		}
		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
#ifdef KBD
		HID_Device_USBTask(&Keyboard_HID_Interface);
#endif
		USB_USBTask();
#endif
		scheduler_run_task();
	}
}
#ifdef USB
/* Event handler for USB device configuration change */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	uint8_t ret;
#ifdef KBD
	ret = HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
#endif
	ret &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	(void)ret;
	USB_Device_EnableSOFEvents();
}

/* Event handler for USB control request reception */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
#ifdef KBD
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
#endif
}

/* Event handler for USB device Start Of Frame (SOF) events */
void EVENT_USB_Device_StartOfFrame(void)
{
#ifdef KBD
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
#endif
}
#ifdef KBD
static int kbd_get_modifier(uint8_t c)
{
	switch (c) {
	case HID_KEYBOARD_SC_LEFT_ALT:
		return HID_KEYBOARD_MODIFIER_LEFTALT;
	case HID_KEYBOARD_SC_RIGHT_ALT:
		return HID_KEYBOARD_MODIFIER_RIGHTALT;
	case HID_KEYBOARD_SC_RIGHT_CONTROL:
		return HID_KEYBOARD_MODIFIER_RIGHTCTRL;
	case HID_KEYBOARD_SC_LEFT_CONTROL:
		return HID_KEYBOARD_MODIFIER_LEFTCTRL;
	case HID_KEYBOARD_SC_LEFT_SHIFT:
		return HID_KEYBOARD_MODIFIER_LEFTSHIFT;
	case HID_KEYBOARD_SC_RIGHT_SHIFT:
		return HID_KEYBOARD_MODIFIER_RIGHTSHIFT;
	case HID_KEYBOARD_SC_LEFT_GUI:
		return HID_KEYBOARD_MODIFIER_LEFTGUI;
	case HID_KEYBOARD_SC_RIGHT_GUI:
		return HID_KEYBOARD_MODIFIER_RIGHTGUI;
	default:
		return -1;
	}
}

static int storage_handle_custom_cmd(void)
{
	uint8_t byte;

	if (storage_get_next_byte(&byte) < 0)
		return 0;

	switch (byte) {
	case CUSTOM_CMD_DELAY: {
		uint16_t cmd_delay;

		storage_set_next_byte();
		if (payload_pos + sizeof(uint16_t) >
		    script_hdr.size + STORAGE_DATA_OFFSET)
			return 0;
		eeprom_load(&cmd_delay, persistent_payload_data + payload_pos,
			    sizeof(uint16_t));
		timer_del(&kbd_timer);
		timer_reschedule(&kbd_timer, (uint32_t)cmd_delay * 1000);
		return 2;
	}
	case CUSTOM_CMD_JITTER_ON:
		jitter_on = 1;
		break;
	case CUSTOM_CMD_JITTER_OFF:
		jitter_on = 0;
		break;
	case CUSTOM_CMD_WAIT_FOR_CAPS_ON:
		wait_for_mask = HID_KEYBOARD_LED_CAPSLOCK;
		wait_for = HID_KEYBOARD_LED_CAPSLOCK;
		break;
	case CUSTOM_CMD_WAIT_FOR_CAPS_OFF:
		wait_for_mask = HID_KEYBOARD_LED_CAPSLOCK;
		wait_for = 0;
		break;
	case CUSTOM_CMD_WAIT_FOR_CAPS_CHNG:
		wait_for_mask = HID_KEYBOARD_LED_CAPSLOCK;
		wait_for = (~led_report & HID_KEYBOARD_LED_CAPSLOCK);
		break;
	case CUSTOM_CMD_WAIT_FOR_NUMLOCK_ON:
		wait_for_mask = HID_KEYBOARD_LED_NUMLOCK;
		wait_for = HID_KEYBOARD_LED_NUMLOCK;
		break;
	case CUSTOM_CMD_WAIT_FOR_NUMLOCK_OFF:
		wait_for_mask = HID_KEYBOARD_LED_NUMLOCK;
		wait_for = 0;
		break;
	case CUSTOM_CMD_WAIT_FOR_NUMLOCK_CHNG:
		wait_for_mask = HID_KEYBOARD_LED_NUMLOCK;
		wait_for = (~led_report & HID_KEYBOARD_LED_NUMLOCK);
		break;
	case CUSTOM_CMD_WAIT_FOR_BNT:
		kbd_run = 0;
		break;
	case CUSTOM_CMD_LED_ON:
		gpio_led_on();
		break;
	case CUSTOM_CMD_LED_OFF:
		gpio_led_off();
		break;
	case CUSTOM_CMD_INJECT_MODE:
		inject_mode = 1;
		break;
	default:
		break;
	}
	return 1;
}

static void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData)
{
	uint8_t i = 0;
	uint8_t byte;
	static uint8_t prev;
	uint8_t cur_cmd;

	if (!kbd_run)
		return;

	if (!kbd_tim_delay)
		return;
	if (!storage_has_more()) {
		kbd_run = 0;
		jitter_on = 0;
		storage_reset_pos();
		return;
	}

	kbd_tim_delay = 0;

	storage_get_cur_byte(&byte);
	if (byte == prev) {
		prev = 0;
		return;
	}
	prev = byte;
	if (byte == 0) {
		storage_set_next_byte();
		return;
	}

	if ((led_report & wait_for_mask) != wait_for)
		return;
	wait_for = 0;
	wait_for_mask = 0;

	cur_cmd = payload_pos;
	if (byte == CUSTOM_CMD) {
		uint16_t skip = storage_handle_custom_cmd();

		storage_inc(skip);
		return;
	}
	while (!inject_mode && storage_has_more() &&
	       byte >= HID_KEYBOARD_SC_LEFT_CONTROL &&
	       byte <= HID_KEYBOARD_SC_RIGHT_GUI &&
	       i < 6) {
		ReportData->Modifier |= kbd_get_modifier(byte);
		storage_set_next_byte();
		storage_get_cur_byte(&byte);
		i++;
	}
	ReportData->KeyCode[i] = byte;
	storage_set_next_byte();
	inject_mode = 0;
}

/* HID class driver callback for creating reports */
bool
CALLBACK_HID_Device_CreateHIDReport(
				    USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
				    uint8_t* const ReportID,
				    const uint8_t ReportType,
				    void* ReportData,
				    uint16_t* const ReportSize)
{
	USB_KeyboardReport_Data_t *KeyboardReport = ReportData;

	CreateKeyboardReport(KeyboardReport);

	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return false;
}

/* HID class driver callback for processing reports */
void
CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t
				     * const HIDInterfaceInfo,
				     const uint8_t ReportID,
				     const uint8_t ReportType,
				     const void* ReportData,
				     const uint16_t ReportSize)
{
	led_report = *(uint8_t *)ReportData;
}
#endif
/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class interface configuration structure being referenced
 */
void EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback; a common
	   use-case is to use the Data Terminal Ready (DTR) flag to enable and
	   disable CDC communications in your application when set to avoid the
	   application blocking while waiting for a host to become ready and read
	   in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice & CDC_CONTROL_LINE_OUT_DTR) != 0;

	(void)HostReady;
}
#endif
