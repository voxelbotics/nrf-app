/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**@file
 *
 * @brief PMIC interrupt module
 */

#ifndef PMIC_IRQ_H__
#define PMIC_IRQ_H__

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Enable PMIC interrupts and configure IRQ handler.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int pmic_irq_enable(void);

/** @brief Disable PMIC interrupts and remove IRQ handler.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int pmic_irq_disable(void);

#ifdef __cplusplus
}
#endif

#endif /* PMIC_IRQ_H__ */
