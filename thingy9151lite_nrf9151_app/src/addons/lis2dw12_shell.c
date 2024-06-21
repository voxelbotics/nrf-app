/*
 * LIS2DW12 shell commands
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/debug/thread_analyzer.h>
#include <ctype.h>

#include "lis2dw12_shell.h"

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

static int cmd_lis2dw12_get(const struct shell *sh, size_t argc, char **argv)
{
	struct sensor_value x, y, z;
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready.\n", dev->name);
		return 0;
	}
	/* lis2dw12 accel */
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_X, &x);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Y, &y);
	sensor_channel_get(dev, SENSOR_CHAN_ACCEL_Z, &z);

	shell_print(sh, "accel x:%f ms/2 y:%f ms/2 z:%f ms/2\n",
			(double)out_ev(&x), (double)out_ev(&y), (double)out_ev(&z));

#ifdef CONFIG_LIS2DW12_TRIGGER
	shell_print(sh, "Trigger count: %d \n", lis2dw12_trig_cnt);
#endif
	return 0;
}

/* set accelerometer sampling rate */
static int cmd_lis2dw12_set(const struct shell *sh, size_t argc, char **argv)
{

	int ret = 0;
	struct sensor_value odr_attr, fs_attr;
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);

	if (argc < 2) {
		shell_print(sh, "Example: lis2dw12_set 12 - set ODR to 12.5 Hz");
		return 0;
	}
	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready.", dev->name);
		return 0;
	}

	/* ODR: LIS2DW12 will change 1 to 1.6 Hz, 12 to 12.5 Hz */
	odr_attr.val1 = strtol(argv[1], NULL, 10);
	odr_attr.val2 = 0;

	shell_print(sh, "Setting sampling rate to %u Hz", odr_attr.val1);

	ret = sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);
	if (ret != 0) {
		shell_print(sh, "Cannot set sampling frequency for accelerometer");
		return ret;
	}

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		shell_print(sh, "Cannot set sampling frequency for LIS2DW12 gyro");
		return 0;
	}

#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(dev, &trig, lis2dw12_trigger_handler);
#endif
	return 0;
}

/* set accelerometer power mode */
static int cmd_lis2dw12_pm_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);
	int mode;
	lis2dw12_mode_t pm;
	struct sensor_value attr;

	if (argc < 2) {
		shell_print(sh, "Example: lis2dw12_pm 1 - set High Performance mode");
		return 0;
	}

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready", dev->name);
		return 0;
	}
	mode = atoi(argv[1]);

	switch (mode) {
		case 1:
			pm = LIS2DW12_HIGH_PERFORMANCE;
			break;
		case 2:
			pm = LIS2DW12_CONT_LOW_PWR_2;
			break;
		case 3:
			pm = LIS2DW12_CONT_LOW_PWR_3;
			break;
		case 4:
			pm = LIS2DW12_CONT_LOW_PWR_4;
			break;
		default:
			pm = LIS2DW12_CONT_LOW_PWR_12bit;
	}
	
	shell_print(sh, "Setting power mode %d (val: %d)", mode, pm);
	attr.val1 = CONFIGURE_PM_MODE;
	attr.val2 = pm;
	
	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_CONFIGURATION, &attr) < 0) {
		shell_print(sh, "Cannot set power mode for LIS2DW12 gyro");
	}

	return 0;
} 

/* set accelerometer interrupt pin and switch interrupt handler to it */
static int cmd_lis2dw12_pin_set(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);
	int pin;
	struct sensor_value attr, fs_attr;
	struct sensor_value odr_attr = {
			.val1 = 12,
			.val2 = 0,
		};

	if (argc < 2) {
		shell_print(sh, "Example: lis2dw12_pin 1 - set pin #1");
		return 0;
	}

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready", dev->name);
		return 0;
	}
	pin = atoi(argv[1]);

	shell_print(sh, "Setting pin #%d", pin);
	
	attr.val1 = CONFIGURE_INT_PIN;
	attr.val2 = pin;

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_CONFIGURATION, &attr) < 0) {
		shell_print(sh, "Cannot set pin for LIS2DW12 gyro");
	}

	sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
		SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr);

	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
		    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		shell_print(sh, "Cannot set sampling frequency for LIS2DW12 gyro");
	}

	/* reset counter */
	lis2dw12_trig_cnt = 0;

#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;
	sensor_trigger_set(dev, &trig, lis2dw12_trigger_handler);
#endif
	
	return 0;
} 

SHELL_CMD_REGISTER(lis2dw12_get, NULL, "Print IMU data", 
		cmd_lis2dw12_get);

SHELL_CMD_REGISTER(lis2dw12_set, NULL, "Set LIS2DW12 IMU sampling frequency (default is 12)",
		cmd_lis2dw12_set);

SHELL_CMD_REGISTER(lis2dw12_pm, NULL, "Set LIS2DW12 IMU power mode (1 - high performance, 2..4 low power mode)",
		cmd_lis2dw12_pm_set);

SHELL_CMD_REGISTER(lis2dw12_pin, NULL, "Set LIS2DW12 INT pin (1 or 2)",
		cmd_lis2dw12_pin_set);
