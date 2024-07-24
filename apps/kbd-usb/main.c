#include <log.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <watchdog.h>
#include <sys/timer.h>
#include <sys/scheduler.h>
#include <sys/buf.h>
#include <net/pkt-mempool.h>
#include <net/event.h>
#include <net/swen.h>
#include <net/if.h>
#include <drivers/rf.h>
#include <interrupts.h>
#include <LUFA/Drivers/USB/USB.h>
#include "Descriptors.h"
#include "gpio.h"

static tim_t led_timer = TIMER_INIT(led_timer);
static tim_t kbd_timer = TIMER_INIT(kbd_timer);
static uint8_t kbd_tim_delay;

static uint8_t rf_addr = 0xEB;
static iface_t rf_iface;
static rf_ctx_t rf_ctx;
static struct iface_queues {
	RING_DECL_IN_STRUCT(pkt_pool, CONFIG_PKT_DRIVER_NB_MAX);
	RING_DECL_IN_STRUCT(rx, CONFIG_PKT_NB_MAX);
} iface_queues = {
	.pkt_pool = RING_INIT(iface_queues.pkt_pool),
	.rx = RING_INIT(iface_queues.rx),
};

#define KBD_DATA_SIZE 21
static const uint8_t PROGMEM kbd_data[] = {
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
static uint8_t kbd_cnt;

static uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/* USB Class driver callback function prototypes */
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);
void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData);

/* LUFA USB Class driver structure for the HID keyboard interface */
static USB_ClassInfo_HID_Device_t Keyboard_HID_Interface = {
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

static USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface = {
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

static void rf_recv_cb(uint8_t from, uint8_t events, buf_t *buf)
{
	buf_addc(buf, '\0');
	printf("%s\r\n", buf->data);
}

void EVENT_USB_Device_Connect(void)
{
}

void EVENT_USB_Device_Disconnect(void)
{
}

static int putchar0(char c, FILE *stream)
{
	(void)stream;
	if (c == '\r')
		CDC_Device_SendString(&VirtualSerial_CDC_Interface, "\r\n");
	CDC_Device_SendByte(&VirtualSerial_CDC_Interface, c);
	return 0;
}

static int getchar0(FILE *stream)
{
	(void)stream;
	return 0;
}

static FILE stream0 =
	FDEV_SETUP_STREAM(putchar0, getchar0, _FDEV_SETUP_RW);

void init_stream0(FILE **out_fd, FILE **in_fd)
{
	*out_fd = &stream0;
	*in_fd = &stream0;
}

int main(void)
{
	init_stream0(&stdout, &stdin);

	timer_subsystem_init();
	timer_add(&led_timer, 0, tim_cb, NULL);

	watchdog_shutdown();

	USB_Init();
	irq_enable();
	gpio_init();

	pkt_mempool_init();
	rf_iface.hw_addr = &rf_addr;
	rf_iface.recv = rf_input;
	rf_iface.flags = IF_UP|IF_RUNNING|IF_NOARP;

	if_init(&rf_iface, IF_TYPE_RF, &iface_queues.pkt_pool,
		&iface_queues.rx, NULL, 1);

	rf_init(&rf_iface, &rf_ctx);
	swen_ev_set(rf_recv_cb);

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
	uint8_t ret = 0;

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

void CreateKeyboardReport(USB_KeyboardReport_Data_t* ReportData)
{
	uint8_t i = 0;
	uint8_t cur_ks;
	static uint8_t prev;

	if (!kbd_tim_delay)
		return;
	kbd_tim_delay = 0;

	if (kbd_cnt >= KBD_DATA_SIZE) {
		timer_del(&kbd_timer);
		kbd_cnt = 0;
		return;
	}

	if (pgm_read_byte_near(kbd_data + kbd_cnt) == prev) {
		prev = 0;
		return;
	}
	prev = pgm_read_byte_near(kbd_data + kbd_cnt);
	switch (prev) {
	case 0:
		kbd_cnt++;
		return;
	case 0xFE:
		kbd_cnt++;
		while (pgm_read_byte_near(kbd_data + kbd_cnt) !=
		       0xFF && i < 6) {
			ReportData->KeyCode[i++] =
				pgm_read_byte_near(kbd_data + kbd_cnt);
			kbd_cnt++;
		}
		if (pgm_read_byte_near(kbd_data + kbd_cnt) == 0xFF)
			kbd_cnt++;
		return;
	case 0xFF:
		/* should never happen */
		kbd_cnt++;
		return;
	default:
		ReportData->KeyCode[0] =
			pgm_read_byte_near(kbd_data + kbd_cnt++);
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
		timer_add(&kbd_timer, 0, tim_kbd_cb, NULL);
	} else {
		/* echo back text */
		CDC_Device_SendString(&VirtualSerial_CDC_Interface,
				      ReportData);

	}
}

/** CDC class driver callback function the processing of changes to the virtual
 *  control lines sent from the host..
 *
 *  \param[in] CDCInterfaceInfo  Pointer to the CDC class
 *  interface configuration structure being referenced
 */
void
EVENT_CDC_Device_ControLineStateChanged(USB_ClassInfo_CDC_Device_t *const
					CDCInterfaceInfo)
{
	/* You can get changes to the virtual CDC lines in this callback;
	 * a common use-case is to use the Data Terminal Ready (DTR) flag
	 * to enable and disable CDC communications in your application when
	 * set to avoid the application blocking while waiting for a host to
	 * become ready and read in the pending data from the USB endpoints.
	*/
	bool HostReady = (CDCInterfaceInfo->State.ControlLineStates.HostToDevice &
			  CDC_CONTROL_LINE_OUT_DTR) != 0;

	(void)HostReady;
}
