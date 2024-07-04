/*
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**@file
 *
 * @brief   PMIC charger module
 */

#ifndef PMIC_CHARGER_H__
#define PMIC_CHARGER_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Enumeration for PMIC charger status bits.
 */
enum pmic_charger_status
{
	PMIC_CHARGER_STATUS_BATT_DETECTED = 0,
	PMIC_CHARGER_STATUS_CHG_COMPLETED,
	PMIC_CHARGER_STATUS_TRICKLE_CHARGE,
	PMIC_CHARGER_STATUS_CC_CHARGE,
	PMIC_CHARGER_STATUS_CV_CHARGE,
	PMIC_CHARGER_STATUS_RECHARGE,
	PMIC_CHARGER_STATUS_HIGH_TEMP_PAUSE,
	PMIC_CHARGER_STATUS_SUPPLEMENT_MODE,

	PMIC_CHARGER_STATUS_TOTAL
};

/** @brief Enumeration for PMIC charger error bits.
 */
enum pmic_charger_error
{
	PMIC_CHARGER_ERROR_NTC_SENSOR = 0,
	PMIC_CHARGER_ERROR_VBAT_SENSOR,
	PMIC_CHARGER_ERROR_VBAT_LOW,
	PMIC_CHARGER_ERROR_TRICKLE,
	PMIC_CHARGER_ERROR_MEAS_TIMEOUT,
	PMIC_CHARGER_ERROR_CHARGE_TIMEOUT,
	PMIC_CHARGER_ERROR_TRICKLE_TIMEOUT,

	PMIC_CHARGER_ERROR_TOTAL
};

/** @brief Update PMIC charger parameters.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int pmic_charger_update(void);

/** @brief Get battery voltage from PMIC charger driver.
 *
 *  @return Battery voltage in Volts
 */
double pmic_charger_get_voltage(void);

/** @brief Get battery current from PMIC charger driver.
 *
 *  @return Battery current in mA.
 *
 *  @note Positive value indicates battery charging, negative - discharge.
 */
double pmic_charger_get_current(void);

/** @brief Get battery temperature from PMIC charger driver.
 *
 *  @return Battery temperature in C degrees.
 */
double pmic_charger_get_temp(void);

/** @brief Read PMIC charger status register.
 *
 *  @return Charger status register value.
 *
 *  @note See enum pmic_charger_status for specific status bits.
 */
uint32_t pmic_charger_get_status(void);

/** @brief Read PMIC charger error register.
 *
 *  @return Charger error register value.
 *
 *  @note See enum pmic_charger_error for specific status bits.
 */
uint32_t pmic_charger_get_error(void);

/** @brief Clear PMIC charger error register.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int pmic_charger_clear_error(void);

/** @brief Check if charging is enabled.
 *
 *  @return True if charging is enabled, false otherwise.
 */
bool pmic_charger_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* PMIC_CHARGER_H__ */
