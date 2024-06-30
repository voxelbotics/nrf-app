/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**@file
 *
 * @brief   PMIC regulator module
 */

#ifndef PMIC_REGULATOR_H__
#define PMIC_REGULATOR_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Regulators ID enumeration
 */
typedef enum {
	PMIC_REGULATOR_BUCK1,
	PMIC_REGULATOR_LDSW1,
	PMIC_REGULATOR_LDSW2,

	PMIC_REGULATOR_TOTAL
} pmic_regulator_id_t;

/** @brief Initialize PMIC voltage regulators.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int pmic_regulator_init(void);

/** @brief Deinitialize PMIC voltage regulators.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
void pmic_regulator_deinit(void);

/** @brief Enable a specific regulator.
 *
 *  @param regulator_id Regulator ID.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int pmic_regulator_enable(pmic_regulator_id_t regulator_id);

/** @brief Disable a specific regulator.
 *
 *  @param regulator_id Regulator ID.
 */
void pmic_regulator_disable(pmic_regulator_id_t regulator_id);

/** @brief Check if regulator is enabled.
 *
 *  @param regulator_id Regulator ID.
 *
 *  @return true if regulator is enabled, false otherwise.
 */
bool pmic_regulator_is_enabled(pmic_regulator_id_t regulator_id);

#ifdef __cplusplus
}
#endif

#endif /* PMIC_REGULATOR_H__ */
