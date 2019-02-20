/**
 * @file lte_lc.h
 *
 * @brief Public APIs for the LTE Link Control driver.
 */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ZEPHYR_INCLUDE_MODEM_INFO_H_
#define ZEPHYR_INCLUDE_MODEM_INFO_H_

/* The largest expected parameter response size */
#define MODEM_INFO_MAX_RESPONSE_SIZE 32

/**@brief RSRP event handler function protoype. */
typedef void (*rsrp_cb_t)(char *rsrp_value, size_t len);

/**@brief LTE link status data. */
enum modem_status {
	MODEM_STATUS_RSRP,	/**< Signal strength */
	MODEM_STATUS_BAND,	/**< Current band */
	MODEM_STATUS_MODE,	/**< Mode */
	MODEM_STATUS_OPERATOR,	/**< Operator name */
	MODEM_STATUS_CELLID,	/**< Cell ID */
	MODEM_STATUS_IP_ADDRESS,/**< IP address */
	MODEM_STATUS_UICC,	/**< UICC state */
	MODEM_STATUS_BATTERY,	/**< Battery voltage */
	MODEM_STATUS_TEMP,	/**< Temperature */
	MODEM_STATUS_FW_VERSION,/**< FW Version */
	MODEM_STATUS_COUNT,	/**< Number of legal elements in enum */
};

/**@brief LTE link data type. */
enum modem_data_type {
	MODEM_DATA_TYPE_INT,	/**< Int */
	MODEM_DATA_TYPE_SHORT,	/**< Short */
	MODEM_DATA_TYPE_STRING,	/**< String */
};

/** @brief Function for initializing the link information driver
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int modem_info_init(void);

/** @brief Function for initializing the subscription of RSRP values
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int modem_info_rsrp_sub_init(rsrp_cb_t cb);

/** @brief Function for requesting the current LTE status of any predefined
 * status value.
 *
 * @return Length of received data or (negative) error code otherwise.
 */
int modem_info_update(enum modem_status status,	char *buffer);

/** @brief Function for requesting the string name of a modem status data type
 *
 * @return Length of received data or (negative) error code otherwise.
 */
int modem_info_get_name(enum modem_status status, char *data_name);

/** @brief Function for requesting the data type of the current modem status
 *
 * @return Length of received data or (negative) error code otherwise.
 */
enum modem_data_type modem_info_get_type(enum modem_status status);

#endif /* ZEPHYR_INCLUDE_MODEM_INFO_H_ */
