/*
 * Copyright (c) 2024 VoxelBotics
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "config.h"
#include "usb_uart.h"

LOG_MODULE_REGISTER(app);

int main(void)
{
	if (usb_uart_enable()) {
		return 0;
	}

	LOG_INF("Start thingy9151lite_nrf5340_cpuapp_app v%d.%d-%d-%s on %s",
		CONFIG_VERSION_MAJOR,
		CONFIG_VERSION_MINOR,
		CONFIG_BUILD_NUMBER,
		CONFIG_BUILD_DEBUG ? "debug" : "release",
		CONFIG_BOARD);

	return 0;
}
