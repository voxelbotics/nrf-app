/*
 * Copyright (c) 2024 VoxelBotics
 */

#ifndef WDT_CONTROL_H_
#define WDT_CONTROL_H_

#include <zephyr/kernel.h>

/** @brief Reset reason enumeration */
typedef enum
{
	WDT_CONTROL_RESET_REASON_USER = 0,

	WDT_CONTROL_RESET_REASON_TOTAL
} wdt_control_reset_reason_t;

/** @brief Initialize and start hardware watchdog module.
 */
void wdt_control_init(void);

/** @brief Stop watchdog feed.
 *
 *  @param reason_id Reset reason ID that caused device reset.
 *  @param user_data unused. Added for compatibility with task wdt.
 */
void wdt_control_stop(int reason_id, void *user_data);

#endif /* WDT_CONTROL_H_ */
