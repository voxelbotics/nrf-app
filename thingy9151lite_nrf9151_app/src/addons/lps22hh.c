#include <zephyr/drivers/sensor.h>

#ifdef CONFIG_LPS22HH_TRIGGER
int lps22hh_trig_cnt;

void lps22hh_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	struct sensor_value pressure;
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);

	lps22hh_trig_cnt++;
}
#endif
