#ifndef _SENSORS_H_
#define _SENSORS_H_

/* HIH Honeywell humidity sensor */
#define HIH_4000_ZERO_OFFSET_MV 826
static inline uint8_t hih_4000_to_rh(uint16_t millivolts)
{
	uint16_t ret;

	if (millivolts < HIH_4000_ZERO_OFFSET_MV)
		return 0;
	ret = (millivolts - HIH_4000_ZERO_OFFSET_MV) / 31;
	return ret > 100 ? 100 : ret;
}

/* tmp36gz temperature sensor */
#define TMP36GZ_TO_C_DEGREES(millivolts) (((millivolts) - 500) / 10)
#define TMP36GZ_TO_C_CENTI_DEGREES(millivolts) ((millivolts) - 500)
#define TMP36GZ_TO_F_DEGREES(millivolts)		\
	((((millivolts) - 500) * 9) / 50 + 32)

/* LM35dz temperature sensor */
#define LM35DZ_TO_C_DEGREES(millivolts) ((millivolts) / 10)
#define LM35DZ_TO_C_CENTI_DEGREES(millivolts) (millivolts)
#define LM35DZ_TO_F_DEGREES(millivolts) (((millivolts) * 9) / 50 + 32)

#endif
