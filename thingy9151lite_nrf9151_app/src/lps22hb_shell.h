#ifndef LPS22HB_SHELL_INCLUDED
#define LPS22HB_SHELL_INCLUDED

#ifdef CONFIG_LPS22HB_TRIGGER

#include <zephyr/drivers/sensor.h>

extern int lps22hb_trig_cnt;

void lps22hb_handler(const struct device *dev,
				     const struct sensor_trigger *trig);

#endif

#endif
