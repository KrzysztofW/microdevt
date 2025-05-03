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

#include <LUFA/Drivers/USB/USB.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <eeprom.h>
#include <watchdog.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/buf.h>
#include "Descriptors.h"
#include "gpio.h"
#include "payload-script.h"

static tim_t led_timer = TIMER_INIT(led_timer);
static tim_t kbd_timer = TIMER_INIT(kbd_timer);
static uint8_t kbd_tim_delay;

static uint8_t EEMEM persistent_payload_data[PAYLOAD_DATA_LENGTH];
static uint8_t payload_data[PAYLOAD_DATA_LENGTH];
static buf_t payload = BUF_INIT_BIN(payload_data);

static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/* USB Class driver callback function prototypes */
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData);

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

static void tim_cb(void *arg)
{
	timer_reschedule(&led_timer, 2000000);
	gpio_led_toggle();
}

static void tim_kbd_cb(void *arg)
{
	timer_reschedule(&kbd_timer, 10000);
	kbd_tim_delay = 1;
}


static int storage_write_data(buf_t *buf)
{
	script_t *script;
	uint16_t cs, cs_tmp;
	void *dst;

	if (buf->len < sizeof(script_t))
		return SERIAL_DATA_INVALID;
	script = (script_t *)buf->data;
	if (script->magic != PAYLOAD_DATA_MAGIC)
		return SERIAL_DATA_INVALID;
	if (script->size != buf->len - sizeof(script_t))
		return SERIAL_DATA_INVALID;

	if (!eeprom_update_and_check(&persistent_payload_data,
				     (uint8_t *)&script->size,
				    script->size + sizeof(script->size)))
		return SERIAL_DATA_STORAGE_ERROR;
	__buf_skip(buf, sizeof(script_t));
	return SERIAL_DATA_OK;
}

static void serial_parse_byte(uint8_t byte)
{
	uint8_t ret;

	if (byte == SERIAL_DATA_START_SEQ) {
		buf_reset(&payload);
		return;
	}

	if (byte == SERIAL_DATA_END_SEQ) {
		ret = storage_write_data(&payload);
		if (ret != SERIAL_DATA_OK)
			goto error;
		return;
	}
	if (buf_addc(&payload, byte) < 0) {
		ret = SERIAL_DATA_TOO_LONG;
		goto error;
	}
	return;
 error:
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, ret);
	buf_reset(&payload);
}

int main(void)
{
	uint16_t payload_size;

	timer_subsystem_init();
	timer_add(&led_timer, 0, tim_cb, NULL);
	timer_add(&kbd_timer, 0, tim_kbd_cb, NULL);

	watchdog_shutdown();

	eeprom_load(&payload_size, &persistent_payload_data,
		    sizeof(payload_size));
	if (payload_size && payload_size != 0xFFFF) {
		payload_size += sizeof(payload_size);
		eeprom_load(&payload_data,
			    &persistent_payload_data,
			    payload_size);
		payload.len = payload_size;
		__buf_skip(&payload, sizeof(payload_size));
	}
	USB_Init();
	sei();
	gpio_init();

	while (1) {
		uint8_t c;

		/* Must throw away unused bytes from the host,
		 * or it will lock up while waiting for the device */
		c = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (c != 0xff)
			serial_parse_byte(c);

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		HID_Device_USBTask(&Keyboard_HID_Interface);
		USB_USBTask();
		scheduler_run_task();
	}
}

/* Event handler for USB device configuration change */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	uint8_t ret;

	ret = HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);
	ret &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	if (ret)
		gpio_led_toggle();
	USB_Device_EnableSOFEvents();
}

/* Event handler for USB control request reception */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/* Event handler for USB device Start Of Frame (SOF) events */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

static sbuf_t kbd_str;
static uint8_t kbd_cnt;
static uint8_t run;

void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData)
{
	uint8_t i = 0;
	static uint8_t prev;

	if (!run)
		return;

	if (!kbd_tim_delay || kbd_str.len == 0)
		return;
	kbd_tim_delay = 0;

	if (kbd_str.data[0] == prev) {
		prev = 0;
		return;
	}
	prev = kbd_str.data[0];
	if (kbd_str.data[0] == 0) {
		__sbuf_skip(&kbd_str, 1);
		return;
	}
	while (kbd_str.len &&
	       kbd_str.data[0] >= HID_KEYBOARD_SC_LEFT_CONTROL &&
	       kbd_str.data[0] <= HID_KEYBOARD_SC_RIGHT_GUI &&
	       i < 6) {
		ReportData->KeyCode[i++] = kbd_str.data[0];
		__sbuf_skip(&kbd_str, 1);
	}
	if (kbd_str.len) {
		ReportData->KeyCode[i] = kbd_str.data[0];
		__sbuf_skip(&kbd_str, 1);
	}
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
	uint8_t *LEDReport = (uint8_t *)ReportData;

	if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK) {
		run = 1;
		kbd_str = buf2sbuf(&payload);
	} else {
		run = 0;
		kbd_cnt = 0;
	}
}
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
