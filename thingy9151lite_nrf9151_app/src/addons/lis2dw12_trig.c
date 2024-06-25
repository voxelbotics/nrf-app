#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "lis2dw12_trig.h"

LOG_MODULE_REGISTER(lis2dw12, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

#ifdef CONFIG_LIS2DW12_TRIGGER

int lis2dw12_trig_cnt;

void lis2dw12_trigger_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	lis2dw12_trig_cnt++;
}

#endif

