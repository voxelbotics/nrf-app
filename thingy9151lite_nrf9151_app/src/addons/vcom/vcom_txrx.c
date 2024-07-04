/*
 * VCOM UART TXRX Test routines
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/random/random.h>
#include "vcom.h"

#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vcom_txrx, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_ppp_uart)

#define MSG_SIZE 256

static uint8_t tx_buf[MSG_SIZE];
static uint8_t rx_buf[MSG_SIZE];
static int rx_pos;
static int tx_pos;

K_TIMER_DEFINE(timer, NULL, NULL);

const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

/* receive buffer used in UART ISR callback */

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
static void serial_cb(const struct device *dev, void *user_data)
{
	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (uart_irq_tx_ready(dev)) {
		int tx_len = 0;
		if (tx_pos < MSG_SIZE) {
			tx_len = uart_fifo_fill(dev, &tx_buf[tx_pos],
					MSG_SIZE - tx_pos);
			tx_pos += tx_len;
		} else {
			uart_irq_tx_disable(dev);
		}
	}

	if (uart_irq_rx_ready(dev)) {
		int rx_len = 0;
		if (rx_pos < MSG_SIZE) {
			rx_len = uart_fifo_read(dev, &rx_buf[rx_pos],
					MSG_SIZE - rx_pos);
			rx_pos += rx_len;

		} else {
			uart_irq_rx_disable(dev);
		}
	}
}

static void vcom_uart_flush(const struct device *dev)
{
	uint8_t c;

	while (uart_fifo_read(dev, &c, 1) > 0) {
		continue;
	}
}

int vcom_txrx(void) {

	char rand_buf[MSG_SIZE/2 - 1];

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not found!");
		return 0;
	}

	/* configure interrupt and callback to receive data */
	int ret = uart_irq_callback_user_data_set(uart_dev, serial_cb, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			LOG_ERR("Interrupt-driven UART API support not enabled\n");
		} else if (ret == -ENOSYS) {
			LOG_ERR("UART device does not support interrupt-driven API\n");
		} else {
			LOG_ERR("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}

	sys_rand_get(rand_buf, MSG_SIZE / 2 - 1);
	bin2hex(&rand_buf[0], MSG_SIZE / 2 - 1, &tx_buf[0], MSG_SIZE);

	rx_pos = 0;
	tx_pos = 0;

	vcom_uart_flush(uart_dev);

	memset(rx_buf, 0, MSG_SIZE);

	uart_irq_rx_enable(uart_dev);
	uart_irq_tx_enable(uart_dev);

	k_timer_start(&timer, K_SECONDS(360), K_NO_WAIT);
	/* Wait for transmit to end */
	while ((tx_pos < MSG_SIZE)) {
		if (k_timer_status_get(&timer))
			break;
		k_yield();
	}

	k_timer_start(&timer, K_SECONDS(10), K_NO_WAIT);
	/* Wait some more time for the receive process to end */
	while ((rx_pos < MSG_SIZE)) {
		if (k_timer_status_get(&timer))
			break;
		k_yield();
	}

	uart_irq_tx_disable(uart_dev);
	uart_irq_rx_disable(uart_dev);

	LOG_DBG("PTRs %d %d", rx_pos, tx_pos);
	LOG_HEXDUMP_DBG(tx_buf, MSG_SIZE, "data tx");
	LOG_HEXDUMP_DBG(rx_buf, MSG_SIZE, "data rx");

	if (memcmp(rx_buf, tx_buf, MSG_SIZE))
		return 1;

	return 0;
}
