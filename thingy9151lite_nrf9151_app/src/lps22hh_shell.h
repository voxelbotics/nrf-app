#ifndef LPS22HH_SHELL_INCLUDED
#define LPS22HH_SHELL_INCLUDED

#ifdef CONFIG_LPS22HH_TRIGGER

#include <zephyr/drivers/sensor.h>

extern int lps22hh_trig_cnt;

void lps22hh_handler(const struct device *dev,
				     const struct sensor_trigger *trig);

#endif

#endif
