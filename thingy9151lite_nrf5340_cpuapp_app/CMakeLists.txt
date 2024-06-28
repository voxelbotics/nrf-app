#
# Copyright (c) 2024 VoxelBotics
#

cmake_minimum_required(VERSION 3.20.0)

list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(imd-nrf)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/usb_uart.c
	src/bt_central_ht.c
)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)
