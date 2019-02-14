#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(CONFIG_LTE_LINK_INFO)
    zephyr_library()
    zephyr_library_sources(lte_li.c)
endif()
