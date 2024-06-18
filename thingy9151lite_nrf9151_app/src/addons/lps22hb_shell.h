#ifndef LPS22HB_SHELL_INCLUDED
#define LPS22HB_SHELL_INCLUDED

#include <zephyr/drivers/sensor.h>

#ifdef CONFIG_LPS22HB_TRIGGER

extern int lps22hb_trig_cnt;

void lps22hb_handler(const struct device *dev,
				     const struct sensor_trigger *trig);

#endif	/* CONFIG_LPS22HB_TRIGGER */

void lps22hb_init();

#endif
