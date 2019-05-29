#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.8.2)

include(../../cmake/boilerplate.cmake)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END
zephyr_include_directories(src)

<<<<<<< HEAD
include_directories(src/orientation_detector)
target_sources(app PRIVATE src/orientation_detector/orientation_detector.c)
include_directories(src/cloud_codec)
target_sources(app PRIVATE src/cloud_codec/cloud_codec.c)
=======
# Include application events and configuration headers
zephyr_library_include_directories(
  src/orientation_detector
  src/ui
  )

# Application sources
add_subdirectory(src/orientation_detector)
add_subdirectory(src/ui)
>>>>>>> ef114f2... applications: asset_tracker: Add user interface module
