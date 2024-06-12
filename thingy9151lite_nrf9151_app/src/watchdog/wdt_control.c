/*
 * Copyright (c) 2024 VoxelBotics
 */
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/watchdog.h>
#include "wdt_control.h"

LOG_MODULE_REGISTER(wdt_control, CONFIG_WDT_CONTROL_LOG_LEVEL);

/* Priority and stack size for wdt_control_work_q */
#define WDT_CONTROL_STACK_SIZE 768
#define WDT_CONTROL_PRIORITY K_LOWEST_APPLICATION_THREAD_PRIO

/* Structure to hold WDT parameters */
struct wdt_control_config
{
	const struct device *dev; /* WDT dev. Can be nPM1300 or internal wathcdog. */
	int channel_id; /* WDT channel ID. */
	atomic_t initialized; /* Flag indicating that WDT is initialized. */
	atomic_t stop; /* Flag to stop WDT feed. */
	struct k_work_delayable feed;
	struct k_work enable;
	struct k_work disable;
};

/* wdt_control_work_q stack */
static K_THREAD_STACK_DEFINE(wdt_control_stack_area, WDT_CONTROL_STACK_SIZE);
/* Separate queue for WDT tasks */
static struct k_work_q wdt_control_work_q;

/* WDT config instance */
static struct wdt_control_config wdt_config = {
#if CONFIG_WDT_CONTROL_USE_INTERNAL_WDT
	.dev = DEVICE_DT_GET(DT_NODELABEL(wdt0)),
#elif CONFIG_WDT_CONTROL_USE_NPM1300_WDT
	.dev = DEVICE_DT_GET(DT_NODELABEL(npm1300_wdt)),
#endif
};

void wdt_control_stop(int reason_id, void *user_data)
{
	ARG_UNUSED(user_data);

	if (!(bool)atomic_get(&(wdt_config.initialized))) {
		LOG_ERR("Watchdog disabled, stopping feed has no effect");
		return;
	}
	/* Tell WDT feed handler to stop rescheduling. */
	atomic_set(&(wdt_config.stop), 1);
}

/* Set WDT timeout and enable it */
static int wdt_control_enable(void)
{
	int err = 0;
	static const struct wdt_timeout_cfg wdt_settings = {
		.window = {
			.min = 0,
			.max = CONFIG_WDT_CONTROL_TIMEOUT_MS,
		},
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC
	};

	if ((bool)atomic_get(&(wdt_config.initialized))) {
		LOG_ERR("Watchdog already enabled");
		err = -EBUSY;
		goto exit;
	}

	if (!device_is_ready(wdt_config.dev)) {
		LOG_ERR("Watchdog device is not ready");
		err = -ENODEV;
		goto exit;
	}

	/* Set WDT timeout. Should return 0 for physical watchdog */
	wdt_config.channel_id =	wdt_install_timeout(wdt_config.dev, &wdt_settings);
	if (wdt_config.channel_id < 0) {
		LOG_ERR("Cannot install watchdog timer! Error code: %d", wdt_config.channel_id);
		err = -EFAULT;
		goto exit;
	}

	/*
	 * Start WDT.
	 * WDT_OPT_PAUSE_HALTED_BY_DBG is needed for reprogramming the device with enabled WDT.
	 */
	err = wdt_setup(wdt_config.dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
	if (err) {
		LOG_ERR("Cannot enable watchdog! Error code: %d", err);
	} else {
		LOG_INF("Watchdog enabled");
		atomic_set(&(wdt_config.initialized), 1);
	}

exit:
	return err;
}

static int wdt_control_disable(void)
{
	int err = 0;

	if (!(bool)atomic_get(&(wdt_config.initialized))) {
		LOG_ERR("Watchdog already disabled");
		err = -EBUSY;
		goto exit;
	}

	if (!device_is_ready(wdt_config.dev)) {
		LOG_ERR("Watchdog device is not ready");
		err = -ENODEV;
		goto exit;
	}

	err = wdt_disable(wdt_config.dev);
	if (err) {
		LOG_ERR("Cannot disable watchdog! Error code: %d", err);
	} else {
		LOG_INF("Watchdog disabled");
		atomic_set(&(wdt_config.initialized), 0);
	}

exit:
	return err;
}

/*
 * WDT feed handler.
 * Stops rescheduling WDT feeding if wdt_control reset command is received or CPU runs out of resources.
 */
static void wdt_control_feed_handler(struct k_work *work_desc)
{
	if (!(bool)atomic_get(&(wdt_config.initialized))) {
		LOG_ERR("Watchdog disabled");
		return;
	}

	int err = wdt_feed(wdt_config.dev, wdt_config.channel_id);
	LOG_DBG("Feeding watchdog");
	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
	}
	if (!(bool)atomic_get(&(wdt_config.stop))) {
		k_work_reschedule(&(wdt_config.feed), K_MSEC(CONFIG_WDT_CONTROL_FEED_PERIOD_MS));
	} else {
		LOG_INF("Watchdog feed stopped");
	}
}

/* WDT enable work handler. */
static void wdt_control_enable_handler(struct k_work *work_desc)
{
	int err = wdt_control_enable();
	if (err) {
		LOG_ERR("Cannot enable watchdog! Error code: %d", err);
		return;
	}

	k_work_schedule_for_queue(&wdt_control_work_q, &(wdt_config.feed), K_NO_WAIT);
}

/* WDT disable work handler. */
static void wdt_control_disable_handler(struct k_work *work_desc)
{
	int err = wdt_control_disable();
	if (err) {
		LOG_ERR("Cannot diable watchdog! Error code: %d", err);
		return;
	}
	k_work_cancel_delayable(&(wdt_config.feed));
}

void wdt_control_init(void)
{
	k_work_init_delayable(&(wdt_config.feed), wdt_control_feed_handler);
	k_work_init(&(wdt_config.enable), wdt_control_enable_handler);
	k_work_init(&(wdt_config.disable), wdt_control_disable_handler);
	k_work_queue_start(&wdt_control_work_q, wdt_control_stack_area,
						K_THREAD_STACK_SIZEOF(wdt_control_stack_area),
						WDT_CONTROL_PRIORITY, NULL);

	k_work_submit_to_queue(&wdt_control_work_q, &(wdt_config.enable));
}

/* WDT enable command. */
static int wdt_control_enable_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Enabling watchdog");
	k_work_submit_to_queue(&wdt_control_work_q, &(wdt_config.enable));

	return 0;
}

/* WDT disable command. */
static int wdt_control_disable_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Disabling watchdog");
	k_work_submit_to_queue(&wdt_control_work_q, &(wdt_config.disable));

	return 0;
}

/* WDT reset command. */
static int wdt_control_reset_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Reset command received, stopping watchdog feed");
	wdt_control_stop(WDT_CONTROL_RESET_REASON_USER, NULL);

	return 0;
}

/* WDT reset command. */
static int wdt_control_status_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	printf("\tWatchdog is: %s\r\n", (bool)atomic_get(&(wdt_config.initialized)) ? "enabled" : "disabled");
	printf("\tWatchdog feed period: %d ms\r\n", CONFIG_WDT_CONTROL_FEED_PERIOD_MS);
	printf("\tWatchdog timeout: %d ms\r\n", CONFIG_WDT_CONTROL_TIMEOUT_MS);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_wdt_control,
	SHELL_CMD(enable, NULL, "Start watchdog", wdt_control_enable_cmd),
	SHELL_CMD(disable, NULL, "Stop watchdog", wdt_control_disable_cmd),
	SHELL_CMD(status, NULL, "Print watchdog status", wdt_control_status_cmd),
	SHELL_CMD(reset, NULL, "Reset the device", wdt_control_reset_cmd),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(wdt_control, &sub_wdt_control, "Watchdog control commands", NULL);
