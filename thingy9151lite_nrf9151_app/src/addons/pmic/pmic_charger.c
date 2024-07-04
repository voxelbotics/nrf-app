/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/npm1300_charger.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/shell/shell.h>

#include "pmic_charger.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmic_charger, CONFIG_CHARGER_LOG_LEVEL);

/* Mutex to prevent concurrent access to the charger. */
static K_MUTEX_DEFINE(mutex);

/* Charger device. */
static const struct device *charger = DEVICE_DT_GET(DT_NODELABEL(npm1300_charger));

int pmic_charger_update(void)
{
	int err = 0;
	k_mutex_lock(&mutex, K_FOREVER);

	if (!device_is_ready(charger)) {
		LOG_ERR("Charger device is not ready");
		goto exit;
	}

	err = sensor_sample_fetch(charger);
	if (err) {
		LOG_ERR("Cannot fetch charger samples. Error code: %d", err);
	}

exit:
	k_mutex_unlock(&mutex);
	return err;
}

double pmic_charger_get_voltage(void)
{
	double ret = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_VOLTAGE, &val);
	ret = sensor_value_to_double(&val);

	k_mutex_unlock(&mutex);
	return ret;
}

double pmic_charger_get_current(void)
{
	double ret = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_AVG_CURRENT, &val);
	ret = -sensor_value_to_double(&val);

	k_mutex_unlock(&mutex);
	return ret;
}

double pmic_charger_get_temp(void)
{
	double ret = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	sensor_channel_get(charger, SENSOR_CHAN_GAUGE_TEMP, &val);
	ret = sensor_value_to_double(&val);

	k_mutex_unlock(&mutex);
	return ret;
}

uint32_t pmic_charger_get_status(void)
{
	uint32_t ret = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	sensor_channel_get(charger, SENSOR_CHAN_NPM1300_CHARGER_STATUS, &val);
	ret = (uint32_t)val.val1;

	k_mutex_unlock(&mutex);
	return ret;
}

uint32_t pmic_charger_get_error(void)
{
	uint32_t ret = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	sensor_channel_get(charger, SENSOR_CHAN_NPM1300_CHARGER_ERROR, &val);
	ret = (uint32_t)val.val1;

	k_mutex_unlock(&mutex);
	return ret;
}

int pmic_charger_clear_error(void)
{
	int err = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	val.val1 = 1;
	err = sensor_attr_set(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, SENSOR_ATTR_CONFIGURATION, &val);
	if (err) {
		LOG_ERR("Cannot clear charging error. Error code: %d", err);
	}

	k_mutex_unlock(&mutex);
	return err;
}

bool pmic_charger_is_enabled(void)
{
	bool ret = false;
	int err = 0;
	struct sensor_value val;
	k_mutex_lock(&mutex, K_FOREVER);

	err = sensor_attr_get(charger, SENSOR_CHAN_GAUGE_DESIRED_CHARGING_CURRENT, SENSOR_ATTR_CONFIGURATION, &val);
	if (ret) {
		LOG_ERR("Cannot get charging status. Error code: %d", err);
	} else {
		ret = (bool)val.val1;
	}

	k_mutex_unlock(&mutex);
	return ret;
}

static int pmic_charger_status_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	int err = pmic_charger_update();
	if (err) {
		LOG_ERR("Cannot update charger status. Error code: %d", err);
		return 0;
	}
	shell_print(sh, "\tBattery charging: %s", pmic_charger_is_enabled() ? "enabled" : "disabled");
	shell_print(sh, "\tBattery voltage: %.2f V", pmic_charger_get_voltage());
	shell_print(sh, "\tBattery current: %.2f mA", pmic_charger_get_current() * 1000);
	shell_print(sh, "\tBattery temp: %.2f", pmic_charger_get_temp());
	shell_print(sh, "\tBattery charger status register: %d", pmic_charger_get_status());
	shell_print(sh, "\tBattery charger error register: %d", pmic_charger_get_error());

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pmic_charger,
	SHELL_CMD(status, NULL, "Read charger status", pmic_charger_status_cmd),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(pmic_charger, &sub_pmic_charger, "PMIC charger control commands", NULL);
