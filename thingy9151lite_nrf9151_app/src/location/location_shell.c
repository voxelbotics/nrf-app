/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef CONFIG_GETOPT
#include <zephyr/posix/unistd.h>
#endif
#include <getopt.h>
#include <net/nrf_cloud.h>
#if defined(CONFIG_NRF_CLOUD_REST)
#include <net/nrf_cloud_rest.h>
#endif
#include <modem/location.h>
#include "location_shell.h"

enum location_shell_command {
	LOCATION_CMD_NONE = 0,
	LOCATION_CMD_GET,
	LOCATION_CMD_CANCEL
};
extern struct k_work_q mosh_common_work_q;

/* Work for sending acquired location to nRF Cloud */
struct gnss_location_work_data {
	struct k_work work;

	enum nrf_cloud_gnss_type format;

	/* Data from location event */
	struct location_event_data loc_evt_data;
};
static struct gnss_location_work_data gnss_location_work_data;

/* Whether cloud location (cellular and Wi-Fi positioning) response is requested from the cloud.
 * Or whether it is not requested and MoSh indicates to Location library that positioning
 * result is unknown.
 */
static bool cloud_resp_enabled;

/******************************************************************************/
static const char location_usage_str[] =
	"Usage: location <subcommand> [options]\n"
	"\n"
	"Subcommands:\n"
	"  get:      Requests the current position or starts periodic position updates.\n"
	"  cancel:   Cancel/stop on going request. No options\n";

static const char location_get_usage_str[] =
	"Usage: location get [--mode <mode>] [--method <method>]\n"
	"[--timeout <secs>] [--interval <secs>]\n"
	"[--gnss_accuracy <acc>] [--gnss_num_fixes <number of fixes>]\n"
	"[--gnss_timeout <timeout in secs>] [--gnss_visibility]\n"
	"[--gnss_priority] [--gnss_cloud_nmea] [--gnss_cloud_pvt]\n"
	"[--cellular_timeout <timeout in secs>] [--cellular_service <service_string>]\n"
	"[--cellular_cell_count <cell count>]\n"
	"[--wifi_timeout <timeout in secs>] [--wifi_service <service_string>]\n"
	"[--cloud_resp_disabled]\n"
	"\n"
	"Options:\n"
	"  -m, --method, [str]         Location method: 'gnss', 'cellular' or 'wifi'. Multiple\n"
	"                              '--method' parameters may be given to indicate list of\n"
	"                              methods in priority order.\n"
	"  --mode, [str]               Location request mode: 'fallback' (default) or 'all'.\n"
	"  --interval, [int]           Position update interval in seconds\n"
	"                              (default: 0 = single position)\n"
	"  -t, --timeout, [float]      Timeout for the entire location request in seconds.\n"
	"                              Zero means timeout is disabled.\n"
	"  --gnss_accuracy, [str]      Used GNSS accuracy: 'low', 'normal' or 'high'\n"
	"  --gnss_num_fixes, [int]     Number of consecutive fix attempts (if gnss_accuracy\n"
	"                              set to 'high', default: 3)\n"
	"  --gnss_timeout, [float]     GNSS timeout in seconds. Zero means timeout is disabled.\n"
	"  --gnss_visibility,          Enables GNSS obstructed visibility detection\n"
	"  --gnss_priority,            Enables GNSS priority mode\n"
	"  --gnss_cloud_nmea,          Send acquired GNSS location to nRF Cloud formatted as NMEA\n"
	"  --gnss_cloud_pvt,           Send acquired GNSS location to nRF Cloud formatted as PVT\n"
	"  --cellular_timeout, [float] Cellular timeout in seconds.\n"
	"                              Zero means timeout is disabled.\n"
	"  --cellular_service, [str]   Used cellular positioning service:\n"
	"                              'any' (default), 'nrf' or 'here'\n"
	"  --cellular_cell_count, [int]\n"
	"                              Requested number of cells\n"
	"  --wifi_timeout, [float]     Wi-Fi timeout in seconds. Zero means timeout is disabled.\n"
	"  --wifi_service, [str]       Used Wi-Fi positioning service:\n"
	"                              'any' (default), 'nrf' or 'here'\n"
	"  --cloud_resp_disabled,      Do not wait for location response from cloud.\n"
	"                              Valid if CONFIG_LOCATION_SERVICE_EXTERNAL is set.\n";

/******************************************************************************/

/* Following are not having short options */
enum {
	LOCATION_SHELL_OPT_INTERVAL         = 1001,
	LOCATION_SHELL_OPT_MODE,
	LOCATION_SHELL_OPT_GNSS_ACCURACY,
	LOCATION_SHELL_OPT_GNSS_TIMEOUT,
	LOCATION_SHELL_OPT_GNSS_NUM_FIXES,
	LOCATION_SHELL_OPT_GNSS_VISIBILITY,
	LOCATION_SHELL_OPT_GNSS_PRIORITY_MODE,
	LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_NMEA,
	LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_PVT,
	LOCATION_SHELL_OPT_CELLULAR_TIMEOUT,
	LOCATION_SHELL_OPT_CELLULAR_SERVICE,
	LOCATION_SHELL_OPT_CELLULAR_CELL_COUNT,
	LOCATION_SHELL_OPT_CLOUD_RESP_DISABLED,
	LOCATION_SHELL_OPT_WIFI_TIMEOUT,
	LOCATION_SHELL_OPT_WIFI_SERVICE,
};

/* Specifying the expected options */
static struct option long_options[] = {
	{ "method", required_argument, 0, 'm' },
	{ "mode", required_argument, 0, LOCATION_SHELL_OPT_MODE },
	{ "interval", required_argument, 0, LOCATION_SHELL_OPT_INTERVAL },
	{ "timeout", required_argument, 0, 't' },
	{ "gnss_accuracy", required_argument, 0, LOCATION_SHELL_OPT_GNSS_ACCURACY },
	{ "gnss_timeout", required_argument, 0, LOCATION_SHELL_OPT_GNSS_TIMEOUT },
	{ "gnss_num_fixes", required_argument, 0, LOCATION_SHELL_OPT_GNSS_NUM_FIXES },
	{ "gnss_visibility", no_argument, 0, LOCATION_SHELL_OPT_GNSS_VISIBILITY },
	{ "gnss_priority", no_argument, 0, LOCATION_SHELL_OPT_GNSS_PRIORITY_MODE },
	{ "gnss_cloud_nmea", no_argument, 0, LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_NMEA },
	{ "gnss_cloud_pvt", no_argument, 0, LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_PVT },
	{ "cellular_timeout", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_TIMEOUT },
	{ "cellular_service", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_SERVICE },
	{ "cellular_cell_count", required_argument, 0, LOCATION_SHELL_OPT_CELLULAR_CELL_COUNT },
	{ "cloud_resp_disabled", no_argument, 0, LOCATION_SHELL_OPT_CLOUD_RESP_DISABLED },
	{ "wifi_timeout", required_argument, 0, LOCATION_SHELL_OPT_WIFI_TIMEOUT },
	{ "wifi_service", required_argument, 0, LOCATION_SHELL_OPT_WIFI_SERVICE },
	{ 0, 0, 0, 0 }
};

static bool gnss_location_to_cloud;
static enum nrf_cloud_gnss_type gnss_location_to_cloud_format;

/******************************************************************************/

static void location_shell_print_usage(const struct shell *shell,
				       enum location_shell_command command)
{
	switch (command) {
	case LOCATION_CMD_GET:
		shell_print(shell, "%s", location_get_usage_str);
		break;

	default:
		shell_print(shell, "%s", location_usage_str);
		break;
	}
}

#define MOSH_LOC_SERVICE_NONE 0xFF

static enum location_service location_shell_string_to_service(const char *service_str)
{
	enum location_service service = MOSH_LOC_SERVICE_NONE;

	if (strcmp(service_str, "any") == 0) {
		service = LOCATION_SERVICE_ANY;
	} else if (strcmp(service_str, "nrf") == 0) {
		service = LOCATION_SERVICE_NRF_CLOUD;
	} else if (strcmp(service_str, "here") == 0) {
		service = LOCATION_SERVICE_HERE;
	}

	return service;
}


void location_ctrl_event_handler(const struct location_event_data *event_data)
{
	switch (event_data->id) {
	case LOCATION_EVT_LOCATION:
		printk("Location:\n");
		printk(
			"  used method: %s (%d)\n",
			location_method_str(event_data->method),
			event_data->method);
		printk("  latitude: %.06f\n", event_data->location.latitude);
		printk("  longitude: %.06f\n", event_data->location.longitude);
		printk("  accuracy: %.01f m\n", event_data->location.accuracy);
		if (event_data->location.datetime.valid) {
			printk(
				"  date: %04d-%02d-%02d\n",
				event_data->location.datetime.year,
				event_data->location.datetime.month,
				event_data->location.datetime.day);
			printk(
				"  time: %02d:%02d:%02d.%03d UTC\n",
				event_data->location.datetime.hour,
				event_data->location.datetime.minute,
				event_data->location.datetime.second,
				event_data->location.datetime.ms);
#if defined(CONFIG_LOCATION_DATA_DETAILS)
			/* Print details for each satellite */
			for (uint32_t i = 0; i < NRF_MODEM_GNSS_MAX_SATELLITES; i++) {
				if (event_data->location.details.gnss.pvt_data.sv[i].sv == 0) {
					break;
				}

				const struct nrf_modem_gnss_sv *sv_data = &event_data->location.details.gnss.pvt_data.sv[i];

				printk("PRN: %3d, C/N0: %4.1f, in fix: %d, unhealthy: %d\n",
					sv_data->sv,
					sv_data->cn0 / 10.0,
					sv_data->flags & NRF_MODEM_GNSS_SV_FLAG_USED_IN_FIX ? 1 : 0,
					sv_data->flags & NRF_MODEM_GNSS_SV_FLAG_UNHEALTHY ? 1 : 0);
			}
#endif
		}
		break;

	case LOCATION_EVT_TIMEOUT:
		printk("Location request timed out\n");
		break;

	case LOCATION_EVT_ERROR:
		printk("Location request failed\n");
		break;
#if defined(CONFIG_LOCATION_SERVICE_EXTERNAL)
#if defined(CONFIG_NRF_CLOUD_AGNSS)
	case LOCATION_EVT_GNSS_ASSISTANCE_REQUEST:
		printk(
			"A-GNSS request from Location library \n"
			"(ephe: 0x%08x alm: 0x%08x flags: 0x%02x)\n",
			(uint32_t)event_data->agnss_request.system[0].sv_mask_ephe,
			(uint32_t)event_data->agnss_request.system[0].sv_mask_alm,
			event_data->agnss_request.data_flags);
		location_srv_ext_agnss_handle(&event_data->agnss_request);
		break;
#endif
#if defined(CONFIG_LOCATION_METHOD_CELLULAR) || defined(CONFIG_LOCATION_METHOD_WIFI)
	case LOCATION_EVT_CLOUD_LOCATION_EXT_REQUEST:
#if defined(CONFIG_LOCATION_METHOD_CELLULAR)
		if (event_data->cloud_location_request.cell_data != NULL) {
			printk(
				"Cloud positioning request from Location library \n"
				"(neighbor cells: %d GCI cells: %d)\n",
				event_data->cloud_location_request.cell_data->ncells_count,
				event_data->cloud_location_request.cell_data->gci_cells_count);
		}
#endif
#if defined(CONFIG_LOCATION_METHOD_WIFI)
		if (event_data->cloud_location_request.wifi_data != NULL) {
			printk(
				"Cloud positioning request from Location library (access points: %d)\n",
				event_data->cloud_location_request.wifi_data->cnt);
		}
#endif
		location_srv_ext_cloud_location_handle(
			&event_data->cloud_location_request, cloud_resp_enabled);
		break;
#endif
#endif /* defined(CONFIG_LOCATION_SERVICE_EXTERNAL) */
	case LOCATION_EVT_RESULT_UNKNOWN:
		printk("Location request completed, but the result is not known\n");
		break;
	default:
		printk("Unknown event from location library, id %d\n", event_data->id);
		break;
	}
}

void location_ctrl_init()
{
	int ret;

	ret = location_init(location_ctrl_event_handler);
	if (ret) {
		printk("Initializing the Location library failed, err: %d\n", ret);
	}
}

/******************************************************************************/

int location_shell(const struct shell *shell, size_t argc, char **argv)
{
	enum location_shell_command command = LOCATION_CMD_NONE;

	int interval = 0;
	bool interval_set = false;

	float timeout = 0;
	bool timeout_set = false;

	float gnss_timeout = 0;
	bool gnss_timeout_set = false;

	enum location_accuracy gnss_accuracy = 0;
	bool gnss_accuracy_set = false;

	int gnss_num_fixes = 0;
	bool gnss_num_fixes_set = false;

	bool gnss_visibility = false;

	bool gnss_priority_mode = false;

	float cellular_timeout = 0;
	bool cellular_timeout_set = false;
	enum location_service cellular_service = LOCATION_SERVICE_ANY;
	int cellular_cell_count = 0;
	bool cellular_cell_count_set = false;

	float wifi_timeout = 0;
	bool wifi_timeout_set = false;
	enum location_service wifi_service = LOCATION_SERVICE_ANY;

	enum location_method method_list[CONFIG_LOCATION_METHODS_LIST_SIZE] = { 0 };
	int method_count = 0;

	enum location_req_mode req_mode = LOCATION_REQ_MODE_FALLBACK;

	cloud_resp_enabled = true;

	int opt;
	int ret = 0;
	int long_index = 0;

	gnss_location_to_cloud_format = NRF_CLOUD_GNSS_TYPE_PVT;

	if (argc < 2) {
		goto show_usage;
	}

	/* command = argv[0] = "location" */
	/* sub-command = argv[1] */
	if (strcmp(argv[1], "get") == 0) {
		command = LOCATION_CMD_GET;
	} else if (strcmp(argv[1], "cancel") == 0) {
		command = LOCATION_CMD_CANCEL;
	} else {
		shell_print(shell, "Unsupported command=%s\n", argv[1]);
		ret = -EINVAL;
		goto show_usage;
	}

	/* Reset getopt due to possible previous failures */
	optreset = 1;

	/* We start from subcmd arguments */
	optind = 2;

	gnss_location_to_cloud = false;

	while ((opt = getopt_long(argc, argv, "m:t:", long_options, &long_index)) != -1) {
		switch (opt) {
		case LOCATION_SHELL_OPT_GNSS_TIMEOUT:
			gnss_timeout = atof(optarg);
			gnss_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_NUM_FIXES:
			gnss_num_fixes = atoi(optarg);
			gnss_num_fixes_set = true;
			break;
		case LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_NMEA:
			gnss_location_to_cloud_format = NRF_CLOUD_GNSS_TYPE_NMEA;
		/* flow-through */
		case LOCATION_SHELL_OPT_GNSS_LOC_CLOUD_PVT:
			gnss_location_to_cloud = true;
			break;
		case LOCATION_SHELL_OPT_CELLULAR_TIMEOUT:
			cellular_timeout = atof(optarg);
			cellular_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_CELLULAR_SERVICE:
			cellular_service = location_shell_string_to_service(optarg);
			if (cellular_service == MOSH_LOC_SERVICE_NONE) {
				shell_print(shell, "Unknown cellular positioning service. See usage:");
				goto show_usage;
			}
			break;
		case LOCATION_SHELL_OPT_CELLULAR_CELL_COUNT:
			cellular_cell_count = atoi(optarg);
			cellular_cell_count_set = true;
			break;

		case LOCATION_SHELL_OPT_WIFI_TIMEOUT:
			wifi_timeout = atof(optarg);
			wifi_timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_WIFI_SERVICE:
			wifi_service = location_shell_string_to_service(optarg);
			if (wifi_service == MOSH_LOC_SERVICE_NONE) {
				shell_print(shell, "Unknown Wi-Fi positioning service. See usage:");
				goto show_usage;
			}
			break;

		case LOCATION_SHELL_OPT_INTERVAL:
			interval = atoi(optarg);
			interval_set = true;
			break;
		case 't':
			timeout = atof(optarg);
			timeout_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_ACCURACY:
			if (strcmp(optarg, "low") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_LOW;
			} else if (strcmp(optarg, "normal") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_NORMAL;
			} else if (strcmp(optarg, "high") == 0) {
				gnss_accuracy = LOCATION_ACCURACY_HIGH;
			} else {
				shell_print(shell, "Unknown GNSS accuracy. See usage:");
				goto show_usage;
			}
			gnss_accuracy_set = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_VISIBILITY:
			gnss_visibility = true;
			break;

		case LOCATION_SHELL_OPT_GNSS_PRIORITY_MODE:
			gnss_priority_mode = true;
			break;

		case LOCATION_SHELL_OPT_MODE:
			if (strcmp(optarg, "fallback") == 0) {
				req_mode = LOCATION_REQ_MODE_FALLBACK;
			} else if (strcmp(optarg, "all") == 0) {
				req_mode = LOCATION_REQ_MODE_ALL;
			} else {
				shell_print(shell,
					"Unknown location request mode (%s) was given. See usage:",
						optarg);
				goto show_usage;
			}
			break;

		case 'm':
			if (method_count >= CONFIG_LOCATION_METHODS_LIST_SIZE) {
				shell_print(shell,
					"Maximum number of location methods (%d) exceeded. "
					"Location method (%s) still given.",
					CONFIG_LOCATION_METHODS_LIST_SIZE, optarg);
				return -EINVAL;
			}

			if (strcmp(optarg, "cellular") == 0) {
				method_list[method_count] = LOCATION_METHOD_CELLULAR;
			} else if (strcmp(optarg, "gnss") == 0) {
				method_list[method_count] = LOCATION_METHOD_GNSS;
			} else if (strcmp(optarg, "wifi") == 0) {
				method_list[method_count] = LOCATION_METHOD_WIFI;
			} else {
				shell_print(shell, "Unknown method (%s) given. See usage:", optarg);
				goto show_usage;
			}
			method_count++;
			break;

		case '?':
			goto show_usage;

		default:
			shell_print(shell, "Unknown option. See usage:");
			goto show_usage;
		}
	}

	/* Handle location subcommands */
	switch (command) {
	case LOCATION_CMD_CANCEL:
		gnss_location_to_cloud = false;
		k_work_cancel(&gnss_location_work_data.work);
		ret = location_request_cancel();
		if (ret) {
			shell_print(shell, "Canceling location request failed, err: %d", ret);
			return -1;
		}
		shell_print(shell, "Location request cancelled");
		break;

	case LOCATION_CMD_GET: {
		struct location_config config = { 0 };
		struct location_config *real_config = &config;

		if (method_count == 0 && !interval_set && !timeout_set &&
		    req_mode == LOCATION_REQ_MODE_FALLBACK) {
			/* No methods or top level config given. Use default config. */
			real_config = NULL;
		}

		location_config_defaults_set(&config, method_count, method_list);

		for (uint8_t i = 0; i < method_count; i++) {
			if (config.methods[i].method == LOCATION_METHOD_GNSS) {
				if (gnss_timeout_set) {
					config.methods[i].gnss.timeout = (gnss_timeout == 0) ?
						SYS_FOREVER_MS : gnss_timeout * MSEC_PER_SEC;
				}
				if (gnss_accuracy_set) {
					config.methods[i].gnss.accuracy = gnss_accuracy;
				}
				if (gnss_num_fixes_set) {
					config.methods[i].gnss.num_consecutive_fixes =
						gnss_num_fixes;
				}
				config.methods[i].gnss.visibility_detection = gnss_visibility;
				config.methods[i].gnss.priority_mode = gnss_priority_mode;
			} else if (config.methods[i].method == LOCATION_METHOD_CELLULAR) {
				config.methods[i].cellular.service = cellular_service;
				if (cellular_timeout_set) {
					config.methods[i].cellular.timeout =
						(cellular_timeout == 0) ?
						SYS_FOREVER_MS : cellular_timeout * MSEC_PER_SEC;
				}
				if (cellular_cell_count_set) {
					config.methods[i].cellular.cell_count = cellular_cell_count;
				}
			} else if (config.methods[i].method == LOCATION_METHOD_WIFI) {
				config.methods[i].wifi.service = wifi_service;
				if (wifi_timeout_set) {
					config.methods[i].wifi.timeout = (wifi_timeout == 0) ?
						SYS_FOREVER_MS : wifi_timeout * MSEC_PER_SEC;
				}
			}
		}

		if (interval_set) {
			config.interval = interval;
		}
		if (timeout_set) {
			config.timeout = (timeout == 0) ?
				SYS_FOREVER_MS : timeout * MSEC_PER_SEC;
		}
		config.mode = req_mode;

		ret = location_request(real_config);
		if (ret) {
			shell_print(shell, "Requesting location failed, err: %d", ret);
			return -1;
		}
		shell_print(shell, "Started to get current location...");
		break;
	}
	default:
		shell_print(shell, "Unknown command. See usage:");
		goto show_usage;
	}

	return ret;
show_usage:
	/* Reset getopt for another users */
	optreset = 1;

	location_shell_print_usage(shell, command);
	return ret;
}

SHELL_CMD_REGISTER(location, NULL,
	"Commands for using the Location library.",
	location_shell);
