/*
 * Copyright (c) 2024 VoxelBotics
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "config.h"
#include "lps22hb_shell.h"

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
			return 0;
		}

		attr.val1 = 10;
		attr.val2 = 0;

		sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);
	}
	return 0;
}
