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

#include <zephyr/drivers/gpio.h>

#define PIN_NUM_MAX 16

struct gpio_nrfx_data {
    /* gpio_driver_data needs to be first */
    struct gpio_driver_data common;
    sys_slist_t callbacks;
    uint32_t counters[PIN_NUM_MAX];
};


static int cmd_gpio_interrupt(const struct shell *sh,
                           size_t argc, char **argv)
{
    int pin = -1;

    if (argc > 1) {
        pin = strtol(argv[1], NULL, 10);
    }

    /* LIS2DW12 pins */
    static const struct gpio_dt_spec lis2dw12_gpio1 =
            GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(lis2dw12), irq_gpios, 0);
    static const struct gpio_dt_spec lis2dw12_gpio2 =
            GPIO_DT_SPEC_GET_BY_IDX(DT_NODELABEL(lis2dw12), irq_gpios, 1);

    struct gpio_nrfx_data * gpio1_data = lis2dw12_gpio1.port->data;
    struct gpio_nrfx_data * gpio2_data = lis2dw12_gpio2.port->data;

    if (pin == -1 || pin == lis2dw12_gpio1.pin) {
		shell_print(sh, "LIS2DW12 INT1 (pin %d): %u", lis2dw12_gpio1.pin,
            gpio1_data->counters[lis2dw12_gpio1.pin]);
	}
	if (pin == -1 || pin == lis2dw12_gpio2.pin) {
		shell_print(sh, "LIS2DW12 INT2 (pin %d): %u", lis2dw12_gpio2.pin,
            gpio2_data->counters[lis2dw12_gpio2.pin]);
    }

    return 0;
}


SHELL_CMD_REGISTER(gpio_interrupt, NULL, "List GPIO interrupts counters",
		cmd_gpio_interrupt);
