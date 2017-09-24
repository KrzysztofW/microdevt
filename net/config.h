#ifndef _CONFIG_H_
#define _CONFIG_H_

#ifndef TEST
#include <avr/io.h>
#else
#include <stdint.h>
#endif

#include "../sys/buf.h"
#include "../sys/utils.h"
#include "pkt-mempool.h"
#include "proto_defs.h"
#include "if.h"

#endif
