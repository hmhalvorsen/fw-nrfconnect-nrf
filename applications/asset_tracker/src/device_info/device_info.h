/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */
/**@file
 *
 * @brief   Device info module.
 *
 * Module that obtains device info data.
 */

#define DEVICE_INFO_STRING_SIZE 128
/** @brief Function for requesting the current device status. The
 * data will be added to the string buffer with JSON formatting.
 *
 * @return Length of string buffer or (negative) error code otherwise.
 */
int device_info_get_json_string(char *string_buffer);
