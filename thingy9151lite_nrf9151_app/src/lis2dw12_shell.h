#ifndef LIS2DW12_SHELL_INCLUDED
#define LIS2DW12_SHELL_INCLUDED

#ifdef CONFIG_LIS2DW12_TRIGGER

#include "lis2dw12_reg.h"

extern int lis2dw12_trig_cnt;

void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig);
#endif

#endif
