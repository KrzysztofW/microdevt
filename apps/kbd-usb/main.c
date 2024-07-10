#include <LUFA/Drivers/USB/USB.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <watchdog.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/buf.h>
#include "Descriptors.h"
#include "gpio.h"

static tim_t led_timer = TIMER_INIT(led_timer);
static tim_t kbd_timer = TIMER_INIT(kbd_timer);
static uint8_t kbd_tim_delay;

static void tim_cb(void *arg)
{
	timer_reschedule(&led_timer, 1000000);
	gpio_led_toggle();
}

static void tim_kbd_cb(void *arg)
{
	timer_reschedule(&kbd_timer, 10000);
	kbd_tim_delay = 1;
}

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

int main(void)
{
	timer_subsystem_init();
	timer_add(&led_timer, 0, tim_cb, NULL);
	timer_add(&kbd_timer, 0, tim_kbd_cb, NULL);

	watchdog_shutdown();

	USB_Init();
	sei();
	gpio_init();

	while (1) {
		uint8_t c ;

		/* Must throw away unused bytes from the host,
		 * or it will lock up while waiting for the device */
		c = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (c > 0 && c != 0xFF) {
			CDC_Device_SendByte(&VirtualSerial_CDC_Interface, c);
		}

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

static uint8_t kbd_data[] = {
	0xFE, HID_KEYBOARD_SC_LEFT_SHIFT,
	HID_KEYBOARD_SC_H, 0xFF,
	HID_KEYBOARD_SC_E, HID_KEYBOARD_SC_L, HID_KEYBOARD_SC_L,
	HID_KEYBOARD_SC_O, HID_KEYBOARD_SC_SPACE,
	0xFE, HID_KEYBOARD_SC_LEFT_SHIFT, HID_KEYBOARD_SC_W, 0xFF,
	HID_KEYBOARD_SC_O, HID_KEYBOARD_SC_R,
	HID_KEYBOARD_SC_L, HID_KEYBOARD_SC_D,
	0xFE, HID_KEYBOARD_SC_LEFT_SHIFT, HID_KEYBOARD_SC_1_AND_EXCLAMATION,
	0xFF,
};
static sbuf_t kbd_str = SBUF_INIT_BIN(kbd_data);
static uint8_t kbd_cnt;
static uint8_t run;

void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData)
{
	uint8_t i = 0;
	uint8_t cur_ks;
	static uint8_t prev;

	if (!run)
		return;

	if (!kbd_tim_delay)
		return;
	kbd_tim_delay = 0;

	if (kbd_cnt >= kbd_str.len) {
		const char *ReportString = "keystrokes sent\r\n";

		CDC_Device_SendString(&VirtualSerial_CDC_Interface,
				      ReportString);
		return;
	}

	if (kbd_str.data[kbd_cnt] == prev) {
		prev = 0;
		return;
	}
	prev = kbd_str.data[kbd_cnt];
	switch (kbd_str.data[kbd_cnt]) {
	case 0:
		kbd_cnt++;
		return;
	case 0xFE:
		kbd_cnt++;
		while (kbd_str.data[kbd_cnt] != 0xFF && i < 6) {
			ReportData->KeyCode[i++] = kbd_str.data[kbd_cnt];
			kbd_cnt++;
		}
		if (kbd_str.data[kbd_cnt] == 0xFF)
			kbd_cnt++;
		return;
	case 0xFF:
		/* should never happen */
		kbd_cnt++;
		return;
	default:
		ReportData->KeyCode[0] = kbd_str.data[kbd_cnt++];
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
