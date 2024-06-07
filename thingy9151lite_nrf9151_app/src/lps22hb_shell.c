/*
 * LPS22HH shell commands
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>
#include <ctype.h>
#include "lps22hh_shell.h"

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

static int cmd_lps22hb_get(const struct shell *sh, size_t argc, char **argv)
{
	struct sensor_value pressure, temp;
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hb_press);

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready.\n", dev->name);
		return 0;
	}

	if (sensor_sample_fetch(dev) < 0) {
		shell_print(sh, "Sensor sample update error\n");
		return 0;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
		shell_print(sh, "Cannot read LPS22HB pressure channel\n");
		return 0;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
		shell_print(sh, "Cannot read LPS22HB temperature channel\n");
		return 0;
	}
	
	/* display pressure */
	shell_print(sh, "Pressure: %.3f kPa\n", sensor_value_to_double(&pressure));

	/* display temperature */
	shell_print(sh, "Temperature: %.2f C\n", sensor_value_to_double(&temp));

	return 0;
}

static int cmd_lps22hb_set(const struct shell *sh, size_t argc, char **argv)
{

	int ret = 0;
	struct sensor_value attr;
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hb_press);

	if (argc < 1) {
		return 0;
	}
	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready.\n", dev->name);
		return 0;
	}
	/*
	   set barometer sampling rate to xx Hz
	   uart:~$ lps22hb_set 1
	   Setting sampling rate to 1 Hz
	*/
	attr.val1 = strtol(argv[1], NULL, 10);	
	attr.val2 = 0;

	shell_print(sh, "Setting sampling rate to %f Hz\n", out_ev(&attr));

	ret = sensor_attr_set(dev, SENSOR_CHAN_ALL,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &attr);

	if (ret != 0) {
		shell_print(sh, "Cannot configure sampling rate.\n");
		return ret;
	}

	return 0;
}

static int cmd_lps22hb_pm_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lps22hb_press);
	struct sensor_value attr;
	uint8_t mode = 0; 

	if (argc < 1) {
		return 0;
	}

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready.\n", dev->name);
		return 0;
	}

	if (strncmp(argv[2], "ln", 2) == 0) {
		mode = 1;
		shell_print(sh, "Setting low-noise mode.\n");
	} else {
		shell_print(sh, "Setting low-current mode.\n");
	}

	
	attr.val1 = mode;
	attr.val2 = 0;

	int ret = sensor_attr_set(dev, SENSOR_CHAN_ALL,
			SENSOR_ATTR_CONFIGURATION, &attr);

	if (ret != 0) {
		shell_print(sh, "Cannot configure power mode.\n");
		return ret;
	}

	return 0;
} 

SHELL_CMD_REGISTER(lps22hb_get, NULL, "Print LPS22HB data",
		cmd_lps22hb_get);

SHELL_CMD_REGISTER(lps22hb_set, NULL, "Set LPS22HB sampling frequency (1..200)",
		cmd_lps22hb_set);

SHELL_CMD_REGISTER(lps22hh_pm, NULL, "Set LPS22HB power mode (lc - low current, ln - low noise)",
		cmd_lps22hb_pm_set);
