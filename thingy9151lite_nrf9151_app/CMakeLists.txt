#
# Copyright (c) 2024 VoxelBotics
#

cmake_minimum_required(VERSION 3.20.0)

# set(DTC_OVERLAY_FILE app.overlay)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(imd-nrf)

# NORDIC SDK APP START
target_sources(app PRIVATE
	src/main.c
	src/lis2dw12.c
	src/lis2dw12_shell.c
	src/lps22hb_shell.c
	src/lps22hh.c
	src/lps22hh_shell.c
)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)
