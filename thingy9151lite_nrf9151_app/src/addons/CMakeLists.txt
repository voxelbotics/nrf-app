#
# Copyright (c) 2024 Emcraft Systems
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/sensors_shell.c
	${CMAKE_CURRENT_SOURCE_DIR}/lis2dw12_trig.c
	${CMAKE_CURRENT_SOURCE_DIR}/lis2dw12_shell.c
	${CMAKE_CURRENT_SOURCE_DIR}/lps22hb_trig.c
	${CMAKE_CURRENT_SOURCE_DIR}/lps22hb_shell.c
	${CMAKE_CURRENT_SOURCE_DIR}/lps22hh_trig.c
	${CMAKE_CURRENT_SOURCE_DIR}/lps22hh_shell.c
)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pmic)
