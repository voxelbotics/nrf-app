/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/watchdog.h>

#include "watchdog_app.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(watchdog, CONFIG_WATCHDOG_LOG_LEVEL);

/* Priority and stack size for watchdog work queue */
#define WATCHDOG_STACK_SIZE 1024
#define WATCHDOG_THREAD_PRIORITY K_HIGHEST_APPLICATION_THREAD_PRIO

struct watchdog_config_storage {
	const struct device *wdt;
};

struct watchdog_data_storage {
	int channel_id;
	atomic_t initialized; /* Flag indicating that watchdog is initialized. */
	atomic_t stop_feed; /* Flag to stop watchdog feed. */
	struct k_work_delayable feed_work;
	struct k_work enable_work;
	struct k_work disable_work;
};

/* watchdog work queue stack */
static K_THREAD_STACK_DEFINE(watchdog_stack_area, WATCHDOG_STACK_SIZE);
/* Separate queue for watchdog tasks */
static struct k_work_q watchdog_work_q;

static watchdog_evt_handler_t app_evt_handler;

static const struct watchdog_config_storage watchdog_config = {
#if CONFIG_WATCHDOG_APPLICATION_USE_INTERNAL_WDT
	.wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0)),
#elif CONFIG_WATCHDOG_APPLICATION_USE_NPM1300_WDT
	.wdt = DEVICE_DT_GET(DT_NODELABEL(npm1300_wdt)),
#endif
};

static struct watchdog_data_storage watchdog_data;

static void watchdog_notify_event(const struct watchdog_evt *evt)
{
	__ASSERT(evt != NULL, "Library event not found");

	if (app_evt_handler != NULL) {
		app_evt_handler(evt);
	}
}

void watchdog_stop_feed(int reason_id, void *user_data)
{
	ARG_UNUSED(user_data);
	ARG_UNUSED(reason_id);

	if (!(bool)atomic_get(&(watchdog_data.initialized))) {
		LOG_ERR("Watchdog disabled, stopping feed has no effect");
		return;
	}
	/* Tell watchdog feed worker to stop rescheduling. */
	atomic_set(&(watchdog_data.stop_feed), 1);
}

/* Set up watchdog timeout */
static int watchdog_timeout_install(const struct watchdog_config_storage *config,
				struct watchdog_data_storage *data)
{
	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(data != NULL);

	static const struct wdt_timeout_cfg watchdog_settings = {
		.window = {
			.min = 0,
			.max = CONFIG_WATCHDOG_APPLICATION_TIMEOUT_MS,
		},
		.callback = NULL,
		.flags = WDT_FLAG_RESET_SOC
	};
	struct watchdog_evt evt = {
		.type = WATCHDOG_EVT_TIMEOUT_INSTALLED,
		.timeout = CONFIG_WATCHDOG_APPLICATION_TIMEOUT_MS
	};

	data->channel_id = wdt_install_timeout(config->wdt, &watchdog_settings);
	if (data->channel_id < 0) {
		LOG_ERR("Cannot install watchdog timeout. Error code: %d", data->channel_id);
		return -EFAULT;
	}

	watchdog_notify_event(&evt);

	LOG_DBG("Watchdog timeout installed. Timeout: %d", CONFIG_WATCHDOG_APPLICATION_TIMEOUT_MS);
	return 0;
}

static int watchdog_start(const struct watchdog_config_storage *config)
{
	__ASSERT_NO_MSG(config != NULL);

	int err = wdt_setup(config->wdt, WDT_OPT_PAUSE_HALTED_BY_DBG);

	if (err) {
		LOG_ERR("Cannot start watchdog! Error code: %d", err);
	} else {
		LOG_DBG("Watchdog started");
	}
	return err;
}

static int watchdog_disable(const struct watchdog_config_storage *config)
{
	int err;

	__ASSERT_NO_MSG(config != NULL);

	if (!device_is_ready(config->wdt)) {
		LOG_ERR("Watchdog device not ready");
		return -ENODEV;
	}

	err = wdt_disable(config->wdt);
	if (err) {
		LOG_ERR("Cannot stop watchdog. Error code: %d", err);
		return err;
	}

	return 0;
}

/* Set up timeout for watchdog and start it */
static int watchdog_enable(const struct watchdog_config_storage *config,
			struct watchdog_data_storage *data)
{
	int err;

	__ASSERT_NO_MSG(config != NULL);
	__ASSERT_NO_MSG(data != NULL);

	if (!device_is_ready(config->wdt)) {
		LOG_ERR("Watchdog device not ready");
		return -ENODEV;
	}

	err = watchdog_timeout_install(config, data);
	if (err) {
		return err;
	}

	err = watchdog_start(config);
	if (err) {
		return err;
	}

	return 0;
}

static void watchdog_feed_worker(struct k_work *work_desc)
{
	if (!(bool)atomic_get(&(watchdog_data.initialized))) {
		LOG_ERR("Watchdog disabled");
		return;
	}

	struct watchdog_evt evt = {
		.type = WATCHDOG_EVT_FEED,
	};

	int err = wdt_feed(watchdog_config.wdt, watchdog_data.channel_id);
	LOG_DBG("Feeding watchdog");
	if (err) {
		LOG_ERR("Cannot feed watchdog. Error code: %d", err);
	}

	if (!(bool)atomic_get(&(watchdog_data.stop_feed))) {
		k_work_reschedule_for_queue(&watchdog_work_q, &watchdog_data.feed_work,
					K_MSEC(CONFIG_WATCHDOG_APPLICATION_FEED_PERIOD_MS));
	} else {
		LOG_INF("Watchdog feed stopped");
	}

	watchdog_notify_event(&evt);
}

/* Watchdog enable work handler. */
static void watchdog_enable_worker(struct k_work *work_desc)
{
	if ((bool)atomic_get(&(watchdog_data.initialized))) {
		LOG_ERR("Watchdog already enabled");
		return;
	}

	struct watchdog_evt evt = {
		.type = WATCHDOG_EVT_START
	};

	int err = watchdog_enable(&watchdog_config, &watchdog_data);
	if (err) {
		LOG_ERR("Failed to enable watchdog. Error code: %d", err);
		return;
	} else {
		LOG_INF("Watchdog enabled");
		atomic_set(&(watchdog_data.initialized), 1);
		atomic_set(&(watchdog_data.stop_feed), 0);
	}

	err = k_work_schedule_for_queue(&watchdog_work_q, &(watchdog_data.feed_work), K_NO_WAIT);
	if (err < 0) {
		LOG_ERR("Cannot schedule watchdog feed work. Error code: %d", err);
	} else {
		LOG_DBG("Watchdog feed enabled. Timeout: %d", CONFIG_WATCHDOG_APPLICATION_FEED_PERIOD_MS);
	}

	watchdog_notify_event(&evt);
}

/* Watchdog disable work handler. */
static void watchdog_disable_worker(struct k_work *work_desc)
{
	if (!(bool)atomic_get(&(watchdog_data.initialized))) {
		LOG_ERR("Watchdog already disabled");
		return;
	}

	struct watchdog_evt evt = {
		.type = WATCHDOG_EVT_STOP
	};

	int err = watchdog_disable(&watchdog_config);
	if (err) {
		LOG_ERR("Failed to disable watchdog. Error code: %d", err);
		return;
	} else {
		LOG_INF("Watchdog disabled");
		atomic_set(&(watchdog_data.initialized), 0);
	}

	err = k_work_cancel_delayable(&(watchdog_data.feed_work));
	if (err < 0) {
		LOG_ERR("Cannot stop watchdog feed work. Error code: %d", err);
	} else {
		LOG_DBG("Watchdog feed disabled.");
	}

	watchdog_notify_event(&evt);
}

int watchdog_init_and_start(void)
{
	k_work_init_delayable(&(watchdog_data.feed_work), watchdog_feed_worker);
	k_work_init(&(watchdog_data.enable_work), watchdog_enable_worker);
	k_work_init(&(watchdog_data.disable_work), watchdog_disable_worker);
	k_work_queue_start(&watchdog_work_q, watchdog_stack_area,
			K_THREAD_STACK_SIZEOF(watchdog_stack_area),
			WATCHDOG_THREAD_PRIORITY, NULL);

	int err = k_work_submit_to_queue(&watchdog_work_q, &(watchdog_data.enable_work));
	if (err < 0) {
		LOG_ERR("Cannot submit WDT enable work. Error code: %d", err);
		return err;
	}
	return 0;
}

void watchdog_register_handler(watchdog_evt_handler_t evt_handler)
{
	if (evt_handler == NULL) {
		app_evt_handler = NULL;
		LOG_DBG("Previously registered handler %p de-registered", app_evt_handler);
		return;
	}

	LOG_DBG("Registering handler %p", evt_handler);
	app_evt_handler = evt_handler;

	/* If the application watchdog already has been initialized and started prior to an
	 * external module registering a handler, the newly registered handler is notified that
	 * the library has been started and that a watchdog timeout has been installed.
	 */
	if ((bool)atomic_get(&(watchdog_data.initialized))) {
		struct watchdog_evt evt = {
			.type = WATCHDOG_EVT_START,
		};

		watchdog_notify_event(&evt);

		evt.type = WATCHDOG_EVT_TIMEOUT_INSTALLED;
		evt.timeout = CONFIG_WATCHDOG_APPLICATION_TIMEOUT_MS;

		watchdog_notify_event(&evt);
	}
}

/* Watchdog enable command. */
static int watchdog_enable_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Enabling watchdog");
	k_work_submit_to_queue(&watchdog_work_q, &(watchdog_data.enable_work));

	return 0;
}

/* Watchdog disable command. */
static int watchdog_control_disable_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Disabling watchdog");
	k_work_submit_to_queue(&watchdog_work_q, &(watchdog_data.disable_work));

	return 0;
}

/* Watchdog reset command. */
static int watchdog_reset_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("Reset command received, stopping watchdog feed");
	watchdog_stop_feed(WDT_CONTROL_RESET_REASON_USER, NULL);

	return 0;
}

/* Watchdog status command. */
static int watchdog_status_cmd(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "\tWatchdog status: %s", (bool)atomic_get(&(watchdog_data.initialized)) ? "enabled" : "disabled");
	shell_print(sh, "\tWatchdog stop requested: %s", (bool)atomic_get(&(watchdog_data.stop_feed)) ? "yes" : "no");
	shell_print(sh, "\tWatchdog feed period: %d ms", CONFIG_WATCHDOG_APPLICATION_FEED_PERIOD_MS);
	shell_print(sh, "\tWatchdog timeout: %d ms", CONFIG_WATCHDOG_APPLICATION_TIMEOUT_MS);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_watchdog,
	SHELL_CMD(enable, NULL, "Start watchdog", watchdog_enable_cmd),
	SHELL_CMD(disable, NULL, "Stop watchdog", watchdog_control_disable_cmd),
	SHELL_CMD(status, NULL, "Print watchdog status", watchdog_status_cmd),
	SHELL_CMD(reset, NULL, "Reset the device", watchdog_reset_cmd),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(watchdog, &sub_watchdog, "Watchdog control commands", NULL);
