/*
 * Copyright (c) 2024 VoxelBotics
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

int main(void)
{
	LOG_INF("Start thingy9151lite_nrf5340_cpuapp_app on %s", CONFIG_BOARD);

	return 0;
}
