#ifndef LIS2DW12_SHELL_INCLUDED
#define LIS2DW12_SHELL_INCLUDED

#include "lis2dw12_reg.h"

#define CONFIGURE_PM_MODE 1
#define CONFIGURE_INT_PIN 2

#ifdef CONFIG_LIS2DW12_TRIGGER

extern int lis2dw12_trig_cnt;

void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig);
#endif /* CONFIG_LIS2DW12_TRIGGER */

void lis2dw12_init();

#endif
