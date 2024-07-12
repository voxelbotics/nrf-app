#
# Copyright (c) 2024 Emcraft Systems
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/gpio_shell.c
)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/vcom)
