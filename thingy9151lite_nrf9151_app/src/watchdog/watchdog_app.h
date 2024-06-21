/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**@file
 *
 * @brief   Watchdog module for Asset Tracker v2
 */

#ifndef WATCHDOG_APP_H__
#define WATCHDOG_APP_H__

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

enum watchdog_evt_type {
	WATCHDOG_EVT_START,
	WATCHDOG_EVT_TIMEOUT_INSTALLED,
	WATCHDOG_EVT_FEED,
	WATCHDOG_EVT_STOP,
	WATCHDOG_EVT_RESET
};

struct watchdog_evt {
	enum watchdog_evt_type type;
	uint32_t timeout;
};

/** @brief Reset reason enumeration */
enum watchdog_reset_reason
{
	WDT_CONTROL_RESET_REASON_USER = 0,

	WDT_CONTROL_RESET_REASON_TOTAL
};

/** @brief Watchdog library event handler.
 *
 *  @param[in] evt The event and any associated parameters.
 */
typedef void (*watchdog_evt_handler_t)(const struct watchdog_evt *evt);

/** @brief Initialize and start application watchdog module.
 *
 *  @return Zero on success, otherwise a negative error code is returned.
 */
int watchdog_init_and_start(void);

/** @brief Register handler to receive watchdog callback events.
 *
 *  @note The library only allows for one event handler to be registered
 *           at a time. A passed in event handler in this function will
 *           overwrite the previously set event handler.
 *
 *  @param evt_handler Event handler. Handler is de-registered if parameter is NULL.
 */
void watchdog_register_handler(watchdog_evt_handler_t evt_handler);

/** @brief Stop watchdog feed.
 *
 *  @param reason_id Reset reason ID that caused device reset.
 *  @param user_data unused. Added for compatibility with task wdt.
 */
void watchdog_stop_feed(int reason_id, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* WATCHDOG_APP_H__ */
