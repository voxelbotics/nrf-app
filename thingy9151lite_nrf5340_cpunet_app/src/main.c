/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/sys/printk.h>
#include "config.h"

int main(void)
{
	printk("Start thingy9151lite_nrf5340_cpunet_app v%d.%d-%d-%s on %s\n",
	       CONFIG_VERSION_MAJOR,
	       CONFIG_VERSION_MINOR,
	       CONFIG_BUILD_NUMBER,
	       CONFIG_BUILD_DEBUG ? "debug" : "release",
	       CONFIG_BOARD);

	printk("Starting nRF RPC bluetooth host\n");

	return 0;
}
