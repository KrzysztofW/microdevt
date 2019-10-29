#include <watchdog.h>
#include <sys/array.h>

#ifdef FEATURE_SIREN
static tim_t siren_timer = TIMER_INIT(siren_timer);
static uint16_t siren_sec_cnt;
#endif

#if defined (FEATURE_TEMPERATURE) || defined (FEATURE_HUMIDITY)
#define FEATURE_SENSORS
#endif

#ifdef FEATURE_SENSORS
static tim_t sensor_timer = TIMER_INIT(sensor_timer);
static uint16_t sensor_report_elapsed_secs;
static sensor_report_t sensor_report;
#ifdef FEATURE_HUMIDITY
static uint16_t fan_sec_cnt;
static int global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];
static uint8_t prev_tendency;
typedef struct humidity_info {
	int val;
	uint8_t tendency;
} humidity_info_t;
#endif
static uint16_t sensor_sampling_update = SENSOR_SAMPLING;
#endif

#ifdef FEATURE_PWR
static uint8_t pwr_state = CMD_NOTIF_MAIN_PWR_DOWN;
static uint8_t pwr_state_report;
#endif

static uint8_t init_time;

#ifdef FEATURE_SIREN
static void arm_cb(void *arg)
{
	static uint8_t on;

	if (on == 0) {
		if (gpio_is_siren_on())
			return;
		gpio_siren_on();
		timer_reschedule(&siren_timer, 10000);
		on = 1;
	} else {
		gpio_siren_off();
		on = 0;
	}
}

static void module_arm(module_cfg_t *cfg, uint8_t on)
{
	if (on)
		cfg->state = MODULE_STATE_ARMED;
	else {
		cfg->state = MODULE_STATE_DISARMED;
		gpio_siren_off();
	}
	timer_del(&siren_timer);
	timer_add(&siren_timer, 10000, arm_cb, NULL);
}
static void set_siren_on(module_cfg_t *cfg, uint8_t force)
{
	if (cfg->state != MODULE_STATE_ARMED && !force)
		return;

	gpio_siren_on();
	siren_sec_cnt = cfg->siren_duration;
}

static void siren_on_tim_cb(void *cfg)
{
	set_siren_on(cfg, 0);
}

static void pir_interrupt(module_cfg_t *cfg, void (*cb)(void))
{
	if (cfg->state != MODULE_STATE_ARMED || !gpio_is_pir_on() ||
	    init_time < INIT_TIME || gpio_is_siren_on() ||
	    timer_is_pending(&siren_timer))
		return;
	cb();
	if (cfg->siren_timeout)
		timer_add(&siren_timer, cfg->siren_timeout * 1000000,
			  siren_on_tim_cb, cfg);
	else
		set_siren_on(cfg, 0);
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
}
#endif

#ifdef FEATURE_PWR
static void power_interrupt(module_cfg_t *cfg)
{
	uint8_t pwr_on;

	if (cfg->state == MODULE_STATE_DISABLED)
		return;
	pwr_on = gpio_is_main_pwr_on();
	if (pwr_on && pwr_state == CMD_NOTIF_MAIN_PWR_DOWN)
		pwr_state = CMD_NOTIF_MAIN_PWR_UP;
	else if (!pwr_on && pwr_state == CMD_NOTIF_MAIN_PWR_UP)
		pwr_state = CMD_NOTIF_MAIN_PWR_DOWN;
	else
		return;
	pwr_state_report = 2;
#ifdef CONFIG_POWER_MANAGEMENT
	power_management_pwr_down_reset();
#endif
}
#endif

#ifdef FEATURE_SENSORS
#ifdef FEATURE_HUMIDITY
static void set_fan_on(void)
{
	gpio_fan_on();
	fan_sec_cnt = FAN_ON_DURATION;
}

static uint8_t get_hum_tendency(uint8_t threshold)
{
	int val;

	if (global_humidity_array[0] == 0)
		return HUMIDITY_TENDENCY_STABLE;

	val = global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
		- global_humidity_array[0];

	if (abs(val) < threshold)
		return HUMIDITY_TENDENCY_STABLE;
	if (val < 0)
		return HUMIDITY_TENDENCY_FALLING;
	return HUMIDITY_TENDENCY_RISING;
}

static void set_humidity_info(humidity_info_t *info, module_cfg_t *cfg)
{
	uint8_t tendency;

	array_left_shift(global_humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH, 1);
	global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1] =
		sensor_report.humidity;
	tendency = get_hum_tendency(cfg->sensor.humidity_threshold);
	if (info->tendency != tendency)
		prev_tendency = info->tendency;
	info->tendency = tendency;
}
#endif

#define ARRAY_LENGTH 10
static uint16_t read_sensor_val(uint8_t pin)
{
       uint8_t i;
       int arr[ARRAY_LENGTH];

       /* get rid of the extrem values */
       for (i = 0; i < ARRAY_LENGTH; i++)
	       arr[i] = adc_read(pin);
       adc_shutdown();
       return array_get_median(arr, ARRAY_LENGTH);
}
#undef ARRAY_LENGTH

#ifdef FEATURE_HUMIDITY
static inline uint8_t get_humidity_cur_value(void)
{
	uint16_t val;

	ADC_SET_REF_VOLTAGE_AVCC();
	val = read_sensor_val(HUMIDITY_ANALOG_PIN);
	val = hih_4000_to_rh(adc_to_millivolt(val));

	/* Prepare the ADC for internal REF voltage p243 24.6 (Atmega328).
	 * This shuts the AREF pin down which is needed to stabilize
	 * the internal voltage on that pin.
	 */
	ADC_SET_REF_VOLTAGE_INTERNAL();

	return val;
}
#endif

#ifdef FEATURE_TEMPERATURE
static inline int8_t get_temperature_cur_value(void)
{
	uint16_t val = read_sensor_val(TEMPERATURE_ANALOG_PIN);

	return LM35DZ_TO_C_DEGREES(adc_to_millivolt(val));
}
#endif

static void read_sensor_values(void)
{
#ifdef FEATURE_TEMPERATURE
	/* the temperature sensor must be read first */
	sensor_report.temperature = get_temperature_cur_value();
#endif
#ifdef FEATURE_HUMIDITY
	sensor_report.humidity = get_humidity_cur_value();
#endif
}

static void sensor_status_on_ready_cb(void *arg)
{
#ifdef FEATURE_HUMIDITY
	module_cfg_t *cfg = arg;
	humidity_info_t info;
#endif

	read_sensor_values();
#ifdef FEATURE_HUMIDITY
	set_humidity_info(&info, cfg);

	if (!cfg->fan_enabled || global_humidity_array[0] == 0)
		return;
	if (info.tendency == HUMIDITY_TENDENCY_RISING ||
	    global_humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH - 1]
	    >= MAX_HUMIDITY_VALUE) {
		set_fan_on();
	} else if (info.tendency == HUMIDITY_TENDENCY_STABLE &&
		   fan_sec_cnt == 0) {
		gpio_fan_off();
	}
#endif
}

static void sensor_sampling_task_cb(void *arg)
{
	sensor_sampling_update = 0;

	/* at least 400us are needed for the internal voltage to
	 * stabilize */
	adc_enable();
	timer_add(&sensor_timer, 800, sensor_status_on_ready_cb, arg);
}

static void get_sensor_values(sensor_value_t *value, module_cfg_t *cfg)
{
#ifdef FEATURE_HUMIDITY
	int humidity_array[GLOBAL_HUMIDITY_ARRAY_LENGTH];

	value->humidity.val = sensor_report.humidity;
	array_copy(humidity_array, global_humidity_array,
		   GLOBAL_HUMIDITY_ARRAY_LENGTH);
	value->humidity.global_val =
		array_get_median(humidity_array, GLOBAL_HUMIDITY_ARRAY_LENGTH);
	value->humidity.tendency =
		get_hum_tendency(cfg->sensor.humidity_threshold);
#endif
#ifdef FEATURE_TEMPERATURE
	value->temperature = sensor_report.temperature;
#endif
}

static void send_sensor_report(const iface_t *iface, uint8_t notif_addr)
{
	sbuf_t sbuf;
	buf_t buf = BUF(sizeof(sensor_report_t) + 1);

	__buf_addc(&buf, CMD_SENSOR_REPORT);
#ifdef FEATURE_HUMIDITY
	__buf_add(&buf, &sensor_report.humidity,
		  sizeof(sensor_report.humidity));
#endif
#ifdef FEATURE_TEMPERATURE
	__buf_add(&buf, &sensor_report.temperature,
		  sizeof(sensor_report.temperature));
#endif
	sbuf = buf2sbuf(&buf);
	swen_sendto(iface, notif_addr, &sbuf);
}

static void sensor_sampling_task_cb(void *arg);

static void
sensor_report_value(module_cfg_t *cfg, void (*sensor_action)(void))
{
	if (cfg->sensor.report_interval == 0)
		return;
	if (sensor_report_elapsed_secs >= cfg->sensor.report_interval) {
		sensor_report_elapsed_secs = 0;
		sensor_action();
	} else
		sensor_report_elapsed_secs++;
}
#endif

#ifdef CONFIG_POWER_MANAGEMENT
void watchdog_on_wakeup(void *arg)
{
#ifdef FEATURE_SENSORS
	static uint8_t sampling_cnt;

	/* sample every 30 seconds ((8-sec sleep + 2 secs below) * 3) */
	if (sampling_cnt >= 3) {
		schedule_task(sensor_sampling_task_cb, arg);
		sampling_cnt = 0;
	} else
		sampling_cnt++;
	sensor_report_elapsed_secs += WD_TIMEOUT;
#endif
#ifdef FEATURE_HUMIDITY
	if (fan_sec_cnt > WD_TIMEOUT)
		fan_sec_cnt -= WD_TIMEOUT;
#endif
	if (init_time < INIT_TIME)
		init_time += WD_TIMEOUT;

	/* stay active for 2 seconds in order to catch incoming RF packets */
	power_management_set_inactivity(INACTIVITY_TIMEOUT - 2);
}

static void pwr_mgr_on_sleep(void *arg)
{
	gpio_led_off();
	watchdog_enable_interrupt(watchdog_on_wakeup, arg);
}
#endif

static void get_module_status(module_status_t *status, module_cfg_t *cfg,
			      swen_l3_assoc_t *assoc)
{
#ifdef FEATURE_SENSORS
	status->sensor.report_interval = cfg->sensor.report_interval;
	status->sensor.notif_addr = cfg->sensor.notif_addr;
	status->sensor.humidity_threshold = cfg->sensor.humidity_threshold;
#endif
	status->flags = 0;
#ifdef FEATURE_HUMIDITY
	if (gpio_is_fan_on())
		status->flags = STATUS_FLAGS_FAN_ON;
	if (cfg->fan_enabled)
		status->flags |= STATUS_FLAGS_FAN_ENABLED;
#endif
#ifdef FEATURE_SIREN
	if (gpio_is_siren_on())
		status->flags |= STATUS_FLAGS_SIREN_ON;
	status->siren.duration = cfg->siren_duration;
	status->siren.timeout = cfg->siren_timeout;
#endif
#ifdef FEATURE_PWR
	if (pwr_state == CMD_NOTIF_MAIN_PWR_UP)
		status->flags |= STATUS_FLAGS_MAIN_PWR_ON;
#endif
	status->state = cfg->state;
	if (assoc && swen_l3_get_state(assoc) == S_STATE_CONNECTED)
		status->flags |= STATUS_FLAGS_CONN_RF_UP;
}

static void
cron_func(module_cfg_t *cfg,
	  void (*pwr_action)(uint8_t state), void (*sensor_action)(void))
{
#ifdef FEATURE_SENSORS
	/* skip sampling when the siren is on */
	if (sensor_sampling_update++ >= SENSOR_SAMPLING &&
	    !timer_is_pending(&siren_timer) && !gpio_is_siren_on())
		schedule_task(sensor_sampling_task_cb, cfg);
#endif
	gpio_led_toggle();

#if defined(CONFIG_POWER_MANAGEMENT) && defined (FEATURE_SIREN)
	/* do not sleep if the siren is on */
	if (gpio_is_siren_on() || timer_is_pending(&siren_timer))
		power_management_pwr_down_reset();
#endif

	if (cfg->state != MODULE_STATE_DISABLED) {
#ifdef FEATURE_PWR
		if (pwr_state_report > 1)
			pwr_state_report--;
		else if (pwr_state_report) {
			pwr_action(pwr_state);
			pwr_state_report = 0;
		}
#endif
#ifdef FEATURE_SENSORS
		sensor_report_value(cfg, sensor_action);
#endif
	}
#ifdef FEATURE_HUMIDITY
	if (fan_sec_cnt) {
		if (fan_sec_cnt == 1)
			gpio_fan_off();
		fan_sec_cnt--;
	}
#endif
#ifdef FEATURE_SIREN
	if (siren_sec_cnt) {
		if (siren_sec_cnt == 1)
			gpio_siren_off();
		siren_sec_cnt--;
	}
#endif
	if (init_time < INIT_TIME)
		init_time++;
}
