/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/dt-bindings/regulator/npm1300.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/shell/shell.h>

#include "pmic_regulator.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmic_regulator, CONFIG_REGULATOR_LOG_LEVEL);

/* Mutex to prevent concurrent access to regulators. */
static K_MUTEX_DEFINE(mutex);

/* "Parent" device, is used to check that the npm1300_regulators device is available on init */
static const struct device *pmic_regulators_parent = DEVICE_DT_GET(DT_NODELABEL(npm1300_regulators));

/* Array of regulators that control device power rails */
static const struct device *pmic_regulator_array[PMIC_REGULATOR_TOTAL] = {
	[PMIC_REGULATOR_BUCK1] = DEVICE_DT_GET(DT_NODELABEL(npm1300_buck1)),
	[PMIC_REGULATOR_LDSW1] = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm1300_ldsw1)),
	[PMIC_REGULATOR_LDSW2] = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(npm1300_ldsw2)),
};

/* Human readable names for regulators */
static const char * pmic_regulator_name_array[PMIC_REGULATOR_TOTAL] = {
	[PMIC_REGULATOR_BUCK1] = "BUCK1",
	[PMIC_REGULATOR_LDSW1] = "LDSW1",
	[PMIC_REGULATOR_LDSW2] = "LDSW2",
};

static pmic_regulator_id_t pmic_regulator_id_by_name(const char * name)
{
	pmic_regulator_id_t ret = PMIC_REGULATOR_TOTAL;
	if (name == NULL)
	{
		return ret;
	}
	for (uint8_t i = 0; i < PMIC_REGULATOR_TOTAL; i++) {
		const char *reg_name = pmic_regulator_name_array[i];
		if (strcmp(name, reg_name) == 0) {
			ret = i;
			break;
		}
	}
	return ret;
}

static uint32_t pmic_regulator_get_voltage(pmic_regulator_id_t regulator_id)
{
	uint32_t uv = 0;
	int err = 0;

	k_mutex_lock(&mutex, K_FOREVER);

	const struct device *reg = pmic_regulator_array[regulator_id];
	const char *name = pmic_regulator_name_array[regulator_id];

	if (!device_is_ready(reg)) {
		LOG_ERR("Regulator %s device is not ready.", name);
		goto exit;
	}

	err = regulator_get_voltage(reg, &uv);
	if (err) {
		LOG_ERR("Cannot read regulator %s voltage. Error code: %d", name, err);
	}
exit:
	k_mutex_unlock(&mutex);
	return uv;
}

bool pmic_regulator_is_enabled(pmic_regulator_id_t regulator_id)
{
	bool status = false;

	k_mutex_lock(&mutex, K_FOREVER);

	const struct device *reg = pmic_regulator_array[regulator_id];
	const char *name = pmic_regulator_name_array[regulator_id];

	if (!device_is_ready(reg)) {
		LOG_ERR("Regulator %s device is not ready.", name);
		status = false;
		goto exit;
	}

	status = regulator_is_enabled(reg);
exit:
	k_mutex_unlock(&mutex);
	return status;
}

int pmic_regulator_enable(pmic_regulator_id_t regulator_id)
{
	int err = 0;

	k_mutex_lock(&mutex, K_FOREVER);

	const struct device *reg = pmic_regulator_array[regulator_id];
	const char *name = pmic_regulator_name_array[regulator_id];

	if (!device_is_ready(reg)) {
		LOG_ERR("Regulator %s device is not ready.", name);
		err = -EIO;
		goto exit;
	}

	if (regulator_is_enabled(reg)) {
		LOG_ERR("Regulator %s is already enabled.", name);
		err = -EBUSY;
		goto exit;
	}

	err = regulator_enable(reg);
	if (err) {
		LOG_ERR("Cannot enable regulator %s. Error code: %d", name, err);
		goto exit;
	}

	if (!regulator_is_enabled(reg)) {
		LOG_ERR("Regulator: %s is not enabled.", name);
		err = -EAGAIN;
		goto exit;
	}

	LOG_INF("Regulator %s enabled", name);
exit:
	k_mutex_unlock(&mutex);
	return err;
}

void pmic_regulator_disable(pmic_regulator_id_t regulator_id)
{
	int err = 0;

	k_mutex_lock(&mutex, K_FOREVER);

	const struct device *reg = pmic_regulator_array[regulator_id];
	const char *name = pmic_regulator_name_array[regulator_id];

	if (!device_is_ready(reg)) {
		LOG_ERR("Regulator %s device is not ready.", name);
		goto exit;
	}

	if (!regulator_is_enabled(reg)) {
		LOG_ERR("Regulator %s is already disabled.", name);
		goto exit;
	}

	err = regulator_disable(reg);
	if (err) {
		LOG_ERR("Cannot disable regulator %s. Error code: %d", name, err);
		goto exit;
	}

	if (regulator_is_enabled(reg)) {
		LOG_ERR("Regulator: %s is not disabled.", name);
		goto exit;
	}

	LOG_INF("Regulator %s disabled", name);
exit:
	k_mutex_unlock(&mutex);
}

void pmic_regulator_deinit(void)
{
	if (!device_is_ready(pmic_regulators_parent)) {
		LOG_ERR("Parent regulators device not ready.");
	}

	if (pmic_regulator_is_enabled(PMIC_REGULATOR_LDSW1)) {
		pmic_regulator_disable(PMIC_REGULATOR_LDSW1);
	}
	if (pmic_regulator_is_enabled(PMIC_REGULATOR_LDSW2)) {
		pmic_regulator_disable(PMIC_REGULATOR_LDSW2);
	}
	if (pmic_regulator_is_enabled(PMIC_REGULATOR_BUCK1)) {
		pmic_regulator_disable(PMIC_REGULATOR_BUCK1);
	}
}

int pmic_regulator_init(void)
{
	int err = 0;
	if (!device_is_ready(pmic_regulators_parent)) {
		LOG_ERR("Parent regulators device not ready.");
		return -EBUSY;
	}

	pmic_regulator_deinit();

	err = pmic_regulator_enable(PMIC_REGULATOR_BUCK1);
	if (err) {
		LOG_ERR("Cannot enable regulator %s. Error code: %d",
			pmic_regulator_name_array[PMIC_REGULATOR_BUCK1], err);
		goto exit;
	}

	err = pmic_regulator_enable(PMIC_REGULATOR_LDSW2);
	if (err) {
		LOG_ERR("Cannot enable regulator %s. Error code: %d",
			pmic_regulator_name_array[PMIC_REGULATOR_LDSW2], err);
		goto exit;
	}

	err = pmic_regulator_enable(PMIC_REGULATOR_LDSW1);
	if (err) {
		LOG_ERR("Cannot enable regulator %s. Error code: %d",
			pmic_regulator_name_array[PMIC_REGULATOR_LDSW1], err);
	}

exit:
	return err;
}

/* PMIC regulator status command. */
static int pmic_regulator_status_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	for (int8_t id = PMIC_REGULATOR_BUCK1; id < PMIC_REGULATOR_TOTAL; id++) {
		const char *name = pmic_regulator_name_array[id];
		uint32_t uv = pmic_regulator_get_voltage(id);
		bool status = pmic_regulator_is_enabled(id);

		shell_print(sh, "\tRegulator: %s status: %s, voltage: %0.2f V",
			name, status ? "enabled" : "disabled", (double)uv / 1000000);
	}

	return 0;
}

/* PMIC regulator enable command. */
static int pmic_regulator_enable_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	pmic_regulator_id_t id = pmic_regulator_id_by_name(argv[1]);

	if (id >= PMIC_REGULATOR_TOTAL) {
		LOG_ERR("Cannot find regulator %s.", argv[1]);
		return 0;
	}

	pmic_regulator_enable(id);

	return 0;
}

/* PMIC regulator enable command. */
static int pmic_regulator_disable_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	pmic_regulator_id_t id = pmic_regulator_id_by_name(argv[1]);

	if (id >= PMIC_REGULATOR_TOTAL) {
		LOG_ERR("Cannot find regulator %s.", argv[1]);
		return 0;
	}

	pmic_regulator_disable(id);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_pmic_regulator,
	SHELL_CMD(status, NULL, "Read regulators status", pmic_regulator_status_cmd),
	SHELL_CMD_ARG(enable, NULL, "Enable <regulator>", pmic_regulator_enable_cmd, 2, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable <regulator>", pmic_regulator_disable_cmd, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(pmic_regulator, &sub_pmic_regulator, "PMIC regulator control commands", NULL);
