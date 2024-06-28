/*
 * LPS22HB init
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include "lps22hb_trig.h"

LOG_MODULE_REGISTER(lps22hb, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

#ifdef CONFIG_LPS22HB_TRIGGER

int lps22hb_trig_cnt;

void lps22hb_handler(const struct device *dev,
				     const struct sensor_trigger *trig)
{
	struct sensor_value pressure;

	printk("LPS22HB: pressure threshold interrupt\n");
	if (sensor_sample_fetch(dev) < 0) {
		return;
	}

	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);

	lps22hb_trig_cnt++;
}

#endif /* CONFIG_LPS22HB_TRIGGER */

void lps22hb_init()
{
	if (IS_ENABLED(CONFIG_LPS22HB_TRIGGER)) {
		const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hb_press);
		int ret;

		/* Set ODR to 10 Hz */
		struct sensor_value attr = {
			.val1 = 10,
			.val2 = 0,
		};

		struct sensor_value threshold_attr = {
			.val1 = LPS22HB_CMD_SET_THRESHOLD,
			.val2 = 16, /* 0.1 kPa */
		};

		/* Set trigger */
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_THRESHOLD,
			.chan = SENSOR_CHAN_ALL,
		};

		if (!device_is_ready(dev)) {		
			return;
		}

		if (sensor_trigger_set(dev, &trig, lps22hb_handler) < 0) {
			LOG_ERR("Cannot configure trigger");
			return;
		}

		ret = sensor_attr_set(dev, SENSOR_CHAN_ALL, SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);

		if (ret != 0) {
			LOG_ERR("Cannot set sampling frequency");
			return;
		}

		ret = sensor_attr_set(dev, SENSOR_CHAN_ALL,
			SENSOR_ATTR_CONFIGURATION, &threshold_attr);

		if (ret != 0) {
			LOG_ERR("Cannot set pressure threshold");
			return;
		}
	}
}

