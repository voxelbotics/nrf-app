#
# Copyright (c) 2024 Emcraft Systems
#
# SPDX-License-Identifier: Apache-2.0
#

target_include_directories(app PRIVATE .)
target_sources_ifdef(CONFIG_PMIC_REGULATOR app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pmic_regulator.c)
