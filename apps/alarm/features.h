#ifndef _FEATURES_H_
#define _FEATURES_H_
#include <stdint.h>

typedef struct module_features {
	uint8_t humidity;
	uint8_t temperature;
	uint8_t fan;
	uint8_t siren;
	uint8_t lan;
	uint8_t rf;
} module_features_t;

#define MAX_HANDLED_MODULES 2
extern module_features_t module_features[MAX_HANDLED_MODULES];

#endif
