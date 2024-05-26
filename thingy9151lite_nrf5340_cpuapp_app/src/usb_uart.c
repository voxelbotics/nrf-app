/*
 * Copyright (c) 2024 VoxelBotics
 */

#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/uart.h>
#include "config.h"
#include "usb_uart.h"

USBD_CONFIGURATION_DEFINE(config_1,
			  USB_SCD_SELF_POWERED,
			  200);

USBD_DESC_LANG_DEFINE(lang);
USBD_DESC_MANUFACTURER_DEFINE(mfr, "ZEPHYR");
USBD_DESC_PRODUCT_DEFINE(product, "Zephyr USBD ACM console");
USBD_DESC_SERIAL_NUMBER_DEFINE(sn, "0123456789ABCDEF");

USBD_DEVICE_DEFINE(usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0x0001);

int usb_uart_enable(void)
{
	int err;

	err = usbd_add_descriptor(&usbd, &lang);
	if (err) {
		return err;
	}

	err = usbd_add_descriptor(&usbd, &mfr);
	if (err) {
		return err;
	}

	err = usbd_add_descriptor(&usbd, &product);
	if (err) {
		return err;
	}

	err = usbd_add_descriptor(&usbd, &sn);
	if (err) {
		return err;
	}

	err = usbd_add_configuration(&usbd, &config_1);
	if (err) {
		return err;
	}

	err = usbd_register_class(&usbd, "cdc_acm_0", 1);
	if (err) {
		return err;
	}

	err = usbd_init(&usbd);
	if (err) {
		return err;
	}

	err = usbd_enable(&usbd);
	if (err) {
		return err;
	}

	return 0;
}
