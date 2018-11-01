#ifndef _SENSORS_H_
#define _SENSORS_H_

/* HIH Honeywell humidity sensor */
#define HIH_4000_TO_RH(millivolts) (((millivolts) - 826) / 31)

/* tmp36gz temperature sensor */
#define TMP36GZ_TO_C_DEGREES(millivolts) (((millivolts) - 500) / 10)

#endif
