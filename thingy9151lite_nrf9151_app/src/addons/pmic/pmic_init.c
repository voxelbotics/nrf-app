/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>

#if defined(CONFIG_PMIC_IRQ)
#include "pmic_irq.h"
#endif
#if defined(CONFIG_PMIC_REGULATOR)
#include "pmic_regulator.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmic_init, CONFIG_PMIC_LOG_LEVEL);

/* PMIC init function called by SYS_INIT.
 * It needs to be called before initializing
 * external devices powered by the PMIC.
 */
static int pmic_init(void)
{
	int err = 0;

#if defined(CONFIG_PMIC_REGULATOR)
	err = pmic_regulator_init();
	if (err) {
		LOG_DBG("pmic_regulator_init, error: %d", err);
	}
#endif

#if defined(CONFIG_PMIC_IRQ)
	err = pmic_irq_enable();
	if (err) {
		LOG_DBG("pmic_irq_init, error: %d", err);
	}
#endif

	return 0;
}

SYS_INIT(pmic_init, POST_KERNEL, CONFIG_PMIC_INIT_PRIORITY);
