/*
 * GPIO shell commands
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <ctype.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/shell/shell.h>

#include <zephyr/drivers/gpio.h>

#define PIN_NUM_MAX 16
#define ARG_MAX_LEN 16

struct gpio_nrfx_data {
  /* gpio_driver_data needs to be first */
  struct gpio_driver_data common;
  sys_slist_t callbacks;
  uint32_t counters[PIN_NUM_MAX];
};

static int cmd_gpio_interrupt(const struct shell *sh, size_t argc,
                              char **argv) {
  int pin, block;
  char *blockstr, *pinstr;
  char argvstr[ARG_MAX_LEN];

  static const struct device *gpio0 =
      DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio0));
  static const struct device *gpio1 =
      DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpio1));
  const struct device *gpio_dev;

  struct gpio_nrfx_data *gpio_data = NULL;

  if (argc > 1) {
    strncpy(argvstr, argv[1], ARG_MAX_LEN);
    blockstr = strtok(argvstr, ".");
    block = strtol(blockstr, NULL, 10);

    gpio_dev = (block == 0) ? gpio0 : gpio1;
    gpio_data = (gpio_dev == NULL) ? NULL : gpio_dev->data;

    if (gpio_data != NULL) {
      pinstr = strtok(NULL, ".");
      pin = strtol(pinstr, NULL, 10);
      shell_print(sh, "P%u.%u: %u", block, pin, gpio_data->counters[pin]);
    }

    return 0;
  }

  for (block = 0; block <= 1; block++) {
    for (pin = 0; pin < PIN_NUM_MAX; pin++) {
      gpio_dev = (block == 0) ? gpio0 : gpio1;
      gpio_data = (gpio_dev == NULL) ? NULL : gpio_dev->data;

      if (gpio_data != NULL) {
        shell_print(sh, "P%u.%u: %u", block, pin, gpio_data->counters[pin]);
      }
    }
  }

  return 0;
}

#define GPIO_BLOCK 0
#define GPIO_BLOCK_NAME gpio0
#define GPIO_PIN 2

static int cmd_trigger_irq(const struct shell *sh, size_t argc, char **argv) {
  int ret;
  static const struct device *gpio =
      DEVICE_DT_GET_OR_NULL(DT_NODELABEL(GPIO_BLOCK_NAME));

  struct gpio_dt_spec spec = {
      .port = gpio,
      .pin = GPIO_PIN,
  };

  if (!device_is_ready(gpio)) {
    shell_print(sh, "Error: device %s is not ready", gpio->name);
    return 0;
  }

  ret = gpio_pin_configure_dt(&spec, GPIO_OUTPUT);
  if (ret != 0) {
    shell_print(sh, "Error %d: failed to configure P%u.%u", ret, GPIO_BLOCK,
                spec.pin);
    return 0;
  }

  shell_print(sh, "GPIO send: P%u.%u", GPIO_BLOCK, GPIO_PIN);
  gpio_pin_set(spec.port, spec.pin, 0);
  k_msleep(100);
  gpio_pin_set(spec.port, spec.pin, 1);

  return 0;
}

SHELL_CMD_REGISTER(gpio_interrupt, NULL, "List GPIO interrupts counters",
                   cmd_gpio_interrupt);

SHELL_CMD_REGISTER(trigger_irq, NULL, "Trigger Inter-MCU IRQ", cmd_trigger_irq);
