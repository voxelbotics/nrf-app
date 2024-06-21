#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "lis2dw12_shell.h"

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

void lis2dw12_init() {

	if (IS_ENABLED(CONFIG_LIS2DW12_TRIGGER)) {
		const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);

		struct sensor_value odr_attr = {
			.val1 = 12,
			.val2 = 0,
		};
		struct sensor_value fs_attr;
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ACCEL_XYZ,
		};

		if (!device_is_ready(dev)) {		
			return;
		}
		
		LOG_DBG("Setting up handler for LIS2DW12");
		
		sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);

		sensor_g_to_ms2(16, &fs_attr);

		if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
			LOG_ERR("Cannot set sampling frequency for LIS2DW12 gyro");
		}

		sensor_trigger_set(dev, &trig, lis2dw12_trigger_handler);
	}
}
