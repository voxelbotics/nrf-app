/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/mfd/npm1300.h>

#include "pmic_irq.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmic_irq, CONFIG_PMIC_LOG_LEVEL);

#define PMIC_IRQ_CHARGER2_INT_SET 0x10U /* INTENEVENTSBCHARGER2SET register */
#define PMIC_IRQ_CHARGER2_INT_CLR 0x11U /* INTENEVENTSBCHARGER2CLR register */
#define PMIC_IRQ_EVENTBATDETECTED_MSK 0x1U /* Register mask for "Battery Detected" event */
#define PMIC_IRQ_EVENTBATLOST_MSK 0x2U /* Register mask for "Battery Lost" event */

static K_MUTEX_DEFINE(mutex);

/* Get PMIC device node to configure interrupts and pins */
static const struct device *pmic_dev = DEVICE_DT_GET(DT_NODELABEL(npm1300_pmic));

/* gpio_callback struct to configure PMIC IRQ callback */
static struct gpio_callback pmic_irq_event_cb;
static atomic_t pmic_irq_initialized = 0;

static void pmic_irq_event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	if (pins & BIT(NPM1300_EVENT_BATTERY_DETECTED)) {
		LOG_INF("Battery detected");
	}
	if (pins & BIT(NPM1300_EVENT_BATTERY_REMOVED)) {
		LOG_ERR("Battery removed");
	}
}

int pmic_irq_disable(void)
{
	int err = 0;
	if (!(bool)atomic_get(&pmic_irq_initialized)) {
		LOG_ERR("PMIC interrupts already disabled");
		err = -EBUSY;
		return err;
	}

	k_mutex_lock(&mutex, K_FOREVER);

	if (!device_is_ready(pmic_dev)) {
		LOG_ERR("PMIC device not ready");
		err = -ENODEV;
		goto exit;
	}

	/* Remove callback from the PMIC driver */
	err = mfd_npm1300_remove_callback(pmic_dev, &pmic_irq_event_cb);
	if (err) {
		LOG_ERR("Cannot remove PMIC IRQ handler. Error code: %d", err);
		goto exit;
	}

	/* Disable interrupts */
	err = mfd_npm1300_reg_write(pmic_dev, 0,
				PMIC_IRQ_CHARGER2_INT_CLR,
				PMIC_IRQ_EVENTBATDETECTED_MSK |
				PMIC_IRQ_EVENTBATLOST_MSK);
	if (err) {
		LOG_ERR("Cannot disable PMIC interrupts. Error code: %d", err);
		goto exit;
	}

	atomic_set(&pmic_irq_initialized, 0);

	LOG_INF("PMIC interrupts disabled");

exit:
	k_mutex_unlock(&mutex);
	return 0;
}

int pmic_irq_enable(void)
{
	int err = 0;
	if ((bool)atomic_get(&pmic_irq_initialized)) {
		LOG_ERR("PMIC interrupts already enabled");
		err = -EBUSY;
		return err;
	}

	k_mutex_lock(&mutex, K_FOREVER);

	if (!device_is_ready(pmic_dev)) {
		LOG_ERR("PMIC device not ready");
		err = -ENODEV;
		goto exit;
	}

	/* Initialize the callback for BATLOST and BATDETECTED events */
	gpio_init_callback(&pmic_irq_event_cb, pmic_irq_event_callback,
			BIT(NPM1300_EVENT_BATTERY_DETECTED) |
			BIT(NPM1300_EVENT_BATTERY_REMOVED));

	/* Add the callback to the PMIC driver */
	err = mfd_npm1300_add_callback(pmic_dev, &pmic_irq_event_cb);
	if (err) {
		LOG_ERR("Cannot add callback to mfd_npm1300 driver. Error code: %d", err);
		goto exit;
	}

	atomic_set(&pmic_irq_initialized, 1);

	LOG_INF("PMIC interrupts enabled");

exit:
	k_mutex_unlock(&mutex);
	return err;
}
