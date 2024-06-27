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

#include "lis2dw12_trig.h"

LOG_MODULE_REGISTER(lis2dw12, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

static inline float out_ev(struct sensor_value *val)
{
	return (val->val1 + (float)val->val2 / 1000000);
}

/* set sampling frequency */
static void lis2dw12_set_odr(const struct device *dev, uint32_t odr)
{
	struct sensor_value odr_attr;

	odr_attr.val1 = odr;
	odr_attr.val2 = 0;

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
		SENSOR_ATTR_SAMPLING_FREQUENCY, &odr_attr) < 0) {
			LOG_DBG("Cannot set sampling frequency for LIS2DW12 gyro");
	}

}

/* set interrupt pin */
static void lis2dw12_set_pin(const struct device *dev, int pin) {
	struct sensor_value attr;

	attr.val1 = CONFIGURE_INT_PIN;
	attr.val2 = pin;

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
			    SENSOR_ATTR_CONFIGURATION, &attr) < 0) {
		LOG_DBG("Cannot set pin for LIS2DW12 gyro");
	}

}

/* set full scale */
static void lis2dw12_set_scale(const struct device *dev)
{
	struct sensor_value fs_attr;
	sensor_g_to_ms2(16, &fs_attr);

	if (sensor_attr_set(dev, SENSOR_CHAN_ACCEL_XYZ,
		    SENSOR_ATTR_FULL_SCALE, &fs_attr) < 0) {
		LOG_DBG("Cannot set scale for LIS2DW12 gyro");
	}
}

/* set trigger handler */
static void lis2dw12_set_trigger(const struct device *dev)
{
#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	/* reset counter */
	lis2dw12_trig_cnt = 0;
	sensor_trigger_set(dev, &trig, lis2dw12_trigger_handler);
#endif
}

/* unset trigger handler */
static void lis2dw12_unset_trigger(const struct device *dev)
{
#ifdef CONFIG_LIS2DW12_TRIGGER
	struct sensor_trigger trig;

	trig.type = SENSOR_TRIG_DATA_READY;
	trig.chan = SENSOR_CHAN_ACCEL_XYZ;

	/* reset counter */
	lis2dw12_trig_cnt = 0;
	sensor_trigger_set(dev, &trig, NULL);
#endif
}

void lis2dw12_configure_tap_handler(const struct device *dev)
{
	lis2dw12_set_pin(dev, 1);
	lis2dw12_set_scale(dev);

#ifdef CONFIG_LIS2DW12_TAP
	struct sensor_trigger tap_trigger = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(dev, &tap_trigger,
			       lis2dw12_trigger_handler) < 0) {
		LOG_DBG("Cannot set tap handler for LIS2DW12");
	}
#endif
}

static void lis2dw12_configure_dataready_handler(const struct device *dev, int pin)
{
	lis2dw12_set_pin(dev, pin);
	lis2dw12_set_odr(dev, 12);
	lis2dw12_set_scale(dev);
	lis2dw12_set_trigger(dev);
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
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);
	uint32_t odr;

	if (argc < 2) {
		shell_print(sh, "Example: lis2dw12_set 12 - set ODR to 12.5 Hz");
		return 0;
	}
	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready.", dev->name);
		return 0;
	}

	/* ODR: LIS2DW12 will change 1 to 1.6 Hz, 12 to 12.5 Hz */
	odr = strtol(argv[1], NULL, 10);

	shell_print(sh, "Setting sampling rate to %u Hz", odr);

	lis2dw12_set_odr(dev, odr);
	lis2dw12_set_scale(dev);
	lis2dw12_set_trigger(dev);

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

/* set accelerometer interrupts*/
static int cmd_lis2dw12_init(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready", dev->name);
		return 0;
	}

	shell_print(sh, "Setting data-ready handler on INT2");
	lis2dw12_configure_dataready_handler(dev, 2);

	return 0;
}

/* unset accelerometer interrupts*/
static int cmd_lis2dw12_uninit(const struct shell *sh, size_t argc, char **argv)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(st_lis2dw12);

	if (!device_is_ready(dev)) {
		shell_print(sh, "%s: device not ready", dev->name);
		return 0;
	}

	shell_print(sh, "Unsetting data-ready handler on INT2");
	lis2dw12_unset_trigger(dev);

	return 0;
}

SHELL_CMD_REGISTER(lis2dw12_get, NULL, "Print IMU data", 
		cmd_lis2dw12_get);

SHELL_CMD_REGISTER(lis2dw12_set, NULL, "Set LIS2DW12 IMU sampling frequency (default is 12)",
		cmd_lis2dw12_set);

SHELL_CMD_REGISTER(lis2dw12_pm, NULL, "Set LIS2DW12 IMU power mode (1 - high performance, 2..4 low power mode)",
		cmd_lis2dw12_pm_set);

SHELL_CMD_REGISTER(lis2dw12_init, NULL, "Set LIS2DW12 data-ready handler on INT2",
		cmd_lis2dw12_init);

SHELL_CMD_REGISTER(lis2dw12_uninit, NULL, "Set LIS2DW12 data-ready handler on INT2",
		cmd_lis2dw12_uninit);
