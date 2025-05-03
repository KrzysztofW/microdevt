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
static uint8_t serial_data_started;
static uint16_t serial_data_written;
#define STORAGE_DATA_OFFSET 2
static uint16_t payload_pos = STORAGE_DATA_OFFSET;
static script_hdr_t script_hdr;
static int8_t magic_pos;

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

static void storage_get_cur_byte(uint8_t *byte)
{
	eeprom_load(byte, persistent_payload_data + payload_pos, 1);
}

static void storage_set_next_byte(void)
{
	payload_pos++;
}

static int8_t storage_has_more(void)
{
	return payload_pos < script_hdr.size + STORAGE_DATA_OFFSET;
}

static void storage_reset_pos(void)
{
	payload_pos = STORAGE_DATA_OFFSET;
}

int main(void)
{
	uint16_t payload_size;

	timer_subsystem_init();
	timer_add(&led_timer, 0, tim_cb, NULL);
	timer_add(&kbd_timer, 0, tim_kbd_cb, NULL);

	watchdog_shutdown();

	storage_check_size();

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

static uint8_t run;

void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData)
{
	uint8_t i = 0;
	uint8_t byte;
	static uint8_t prev;

	if (!run)
		return;

	if (!kbd_tim_delay)
		return;
	if (!storage_has_more()) {
		run = 0;
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
	while (storage_has_more() &&
	       byte >= HID_KEYBOARD_SC_LEFT_CONTROL &&
	       byte <= HID_KEYBOARD_SC_RIGHT_GUI &&
	       i < 6) {
		ReportData->KeyCode[i++] = byte;
		storage_set_next_byte();
		storage_get_cur_byte(&byte);
	}
	if (storage_has_more()) {
		ReportData->KeyCode[i] = byte;
		storage_set_next_byte();
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
		storage_reset_pos();
	} else {
		run = 0;
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
