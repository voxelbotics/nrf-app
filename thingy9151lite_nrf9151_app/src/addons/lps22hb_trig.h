#ifndef LPS22HB_SHELL_INCLUDED
#define LPS22HB_SHELL_INCLUDED

#include <zephyr/drivers/sensor.h>

#define LPS22HB_CMD_SET_MODE 0
#define LPS22HB_CMD_SET_THRESHOLD 1

#ifdef CONFIG_LPS22HB_TRIGGER

extern int lps22hb_trig_cnt;

void lps22hb_handler(const struct device *dev,
				     const struct sensor_trigger *trig);

#endif	/* CONFIG_LPS22HB_TRIGGER */

void lps22hb_init();

#endif
