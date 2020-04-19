#ifndef _WS2812_H_
#define _WS2812_H_

#include <ws2812_cfg.h>
#include <sys/utils.h>

#ifndef WS2812_RESET_DELAY
#error "define WS2812_RESET_DELAY (50us for WS2812, 300us for WS2813)"
#endif

#ifndef WS2812_PIN
#error "define WS2812_PIN"
#endif

#ifndef WS2812_PORT
#error "define WS2812_PORT"
#endif

/* use this to be able to abort in progress LED programming */
#define TRANSFER_ABORT

typedef struct __PACKED__ rgb {
	uint8_t g;
	uint8_t r;
	uint8_t b;
} rgb_t;

typedef struct __PACKED__ rgbw {
	uint8_t g;
	uint8_t r;
	uint8_t b;
	uint8_t w;
} rgbw_t;

#ifdef TRANSFER_ABORT
extern volatile uint8_t ws2812_n_transfer_abort;
#define WS2812_ABORT_TRANSFER() ws2812_n_transfer_abort = 0
#define WS2812_ABORT_TRANSFER_RESET() ws2812_n_transfer_abort = 1
#else
#define WS2812_ABORT_TRANSFER()
#define WS2812_ABORT_TRANSFER_RESET()
#endif

void ws2812_send_rgb(const rgb_t *leds, unsigned len);
void ws2812_send_rgb_multi(const rgb_t *leds, unsigned len, uint8_t multi);
void ws2812_send_rgbw(const rgbw_t *leds, unsigned len);
void ws2812_stop(void);

#endif
