#ifndef _EVENT_H_
#define _EVENT_H_

typedef enum event_flags {
	EV_NONE  = 0,
	EV_READ  = 1,
	EV_WRITE = 1 << 1,
	EV_ERROR = 1 << 2,
	/* EV_LAST  = 1 << 7, */
} event_flags_t;

#endif
