/*
 * Copyright (c) 2024 VoxelBotics
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "config.h"
#include "lps22hb_shell.h"
#include "lis2dw12_shell.h"

LOG_MODULE_REGISTER(app);

int main(void)
{
	LOG_INF("Start thingy9151lite_nrf9151_app v%d.%d-%d-%s on %s",
		CONFIG_VERSION_MAJOR,
		CONFIG_VERSION_MINOR,
		CONFIG_BUILD_NUMBER,
		CONFIG_BUILD_DEBUG ? "debug" : "release",
		CONFIG_BOARD);

	if (IS_ENABLED(CONFIG_LPS22HB_TRIGGER)) {
		const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hb_press);
		struct sensor_value attr;
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};

		if (sensor_trigger_set(dev, &trig, lps22hb_handler) < 0) {
			LOG_ERR("Cannot configure trigger\n");
		}

		attr.val1 = 10;
		attr.val2 = 0;

		sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	}

	if (IS_ENABLED(CONFIG_LIS2DW12_TRIGGER)) {
		const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);

		struct sensor_value odr_attr = {
			.val1 = 12,
			.val2 = 500000,
		};
		struct sensor_value fs_attr;
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ACCEL_XYZ,
		};
		
		LOG_DBG("Setting up handler for LIS2DW12");
		
		lis2dw12_set_power_mode(dev, LIS2DW12_HIGH_PERFORMANCE);

		sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);

		sensor_g_to_ms2(16, &fs_attr);

		if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
			LOG_ERR("Cannot set sampling frequency for LIS2DW12 gyro\n");
		}

		sensor_trigger_set(dev, &trig, lis2dw12_trigger_handler);
	}

	return 0;
}
