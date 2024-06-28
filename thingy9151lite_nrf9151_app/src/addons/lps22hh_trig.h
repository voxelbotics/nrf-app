#ifndef LPS22HH_SHELL_INCLUDED
#define LPS22HH_SHELL_INCLUDED

#include <zephyr/drivers/sensor.h>

#define LPS22HH_CMD_SET_MODE      0
#define LPS22HH_CMD_SET_THRESHOLD 1

#ifdef CONFIG_LPS22HH_TRIGGER

extern int lps22hh_trig_cnt;

void lps22hh_handler(const struct device *dev, const struct sensor_trigger *trig);

#endif /* CONFIG_LPS22HH_TRIGGER */

void lps22hh_init();

#endif
