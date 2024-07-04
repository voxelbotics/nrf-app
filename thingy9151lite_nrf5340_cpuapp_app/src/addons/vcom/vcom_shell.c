/*
 * VCOM Shell commands
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <zephyr/shell/shell.h>
#include "vcom.h"

static int cmd_echosrv_start(const struct shell *sh, size_t argc, char **argv)
{
	vcom_echosrv_start();
	return 0;
}

static int cmd_echosrv_stop(const struct shell *sh, size_t argc, char **argv)
{
	vcom_echosrv_stop();
	return 0;
}

static int cmd_vcom_txrx(const struct shell *sh, size_t argc, char **argv)
{
	int ret = vcom_txrx();
	shell_print(sh, "VCOM TXRX Test: %s", ret ? "FAILED" : "PASSED");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_vcom_echosrv,
	SHELL_CMD(start, NULL, "Start echo service", cmd_echosrv_start),
	SHELL_CMD(stop, NULL, "Stop echo service", cmd_echosrv_stop),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_vcom_test,
	SHELL_CMD(echosrv, &sub_vcom_echosrv, "echo service commands", NULL),
	SHELL_CMD(txrx, NULL, "Run tx/rx test", cmd_vcom_txrx),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_vcom,
	SHELL_CMD(test, &sub_vcom_test, "VCOM Test commands", NULL),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(vcom, &sub_vcom, "VCOM commands", NULL);
