#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(lps22hh, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

#ifdef CONFIG_LPS22HH_TRIGGER

int lps22hh_trig_cnt;

void lps22hh_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	struct sensor_value pressure;
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);

	lps22hh_trig_cnt++;
}

#endif /* CONFIG_LPS22HH_TRIGGER */

void lps22hh_init()
{
	if (IS_ENABLED(CONFIG_LPS22HH_TRIGGER)) {
		const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hh);
		struct sensor_value attr;
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};

		if (!device_is_ready(dev)) {		
			return;
		}
		
		if (sensor_trigger_set(dev, &trig, lps22hh_handler) < 0) {
			LOG_ERR("Cannot configure trigger");
		}

		attr.val1 = 10;
		attr.val2 = 0;

		sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	}
}

