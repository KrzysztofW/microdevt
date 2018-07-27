#include "features.h"

module_features_t module_features[MAX_HANDLED_MODULES] = {
	{
		.temperature = 1,
		.siren = 1,
		.lan = 1,
		.rf = 1,
	},
	{
		.humidity = 1,
		.fan = 1,
		.siren = 1,
		.rf = 1,
	},
};
