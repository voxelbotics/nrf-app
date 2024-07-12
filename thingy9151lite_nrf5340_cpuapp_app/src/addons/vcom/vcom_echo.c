/*
 * VCOM UART Echo Service
 *
 * Copyright (c) 2024 Emcraft Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vcom_echo, CONFIG_APPLICATION_MODULE_LOG_LEVEL);

/* change this to any other UART peripheral if desired */
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_ppp_uart)

#define MSG_SIZE 8

K_FIFO_DEFINE(uart_fifo);

struct uart_pkt {
	void * reserved;
	char c;
};

static struct uart_pkt rx_buf[MSG_SIZE];

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static struct k_work work;

/* receive buffer used in UART ISR callback */

/*
 * Read characters from UART until line end is detected. Afterwards push the
 * data to the message queue.
 */
static void serial_cb(const struct device *dev, void *user_data)
{
	int rx_buf_pos;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}


	rx_buf_pos = 0;
	/* read until FIFO empty */
	while ((rx_buf_pos < MSG_SIZE) && (uart_fifo_read(uart_dev,
					&rx_buf[rx_buf_pos].c, 1) == 1)) {
		k_fifo_put(&uart_fifo, &rx_buf[rx_buf_pos]);
		rx_buf_pos++;
	}

	uart_irq_rx_disable(uart_dev);
	k_work_submit(&work);
}

static void flush_uart_fifo(struct k_work *item)
{
	struct uart_pkt * pkt;

	if (k_fifo_is_empty(&uart_fifo))
		return;

	/* wait for input from the user */
	while ((pkt = k_fifo_get(&uart_fifo, K_NO_WAIT)) != NULL) {
		uart_poll_out(uart_dev, pkt->c);
	}

	k_usleep(100);
	uart_irq_rx_enable(uart_dev);
}

void vcom_echosrv_stop(void)
{
	uart_irq_rx_disable(uart_dev);
	k_work_cancel(&work);
}

int vcom_echosrv_start(void)
{
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not found!");
		return 0;
	}

	k_work_init(&work, flush_uart_fifo);
	k_fifo_init(&uart_fifo);

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

	uart_irq_rx_enable(uart_dev);

	return 0;
}
