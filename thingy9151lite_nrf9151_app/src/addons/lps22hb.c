#include <zephyr/drivers/sensor.h>

#ifdef CONFIG_LPS22HB_TRIGGER
int lps22hb_trig_cnt;

void lps22hb_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	struct sensor_value pressure;
	if (sensor_sample_fetch(dev) < 0) {
		return 0;
	}

	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);

	lps22hb_trig_cnt++;
}
#endif
