#
# Copyright (c) 2024 Emcraft Systems
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(app PRIVATE .)

target_sources_ifdef(CONFIG_VCOM_TESTS app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/vcom_shell.c
	${CMAKE_CURRENT_SOURCE_DIR}/vcom_echo.c
	${CMAKE_CURRENT_SOURCE_DIR}/vcom_txrx.c
)
