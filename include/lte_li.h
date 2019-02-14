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

#ifndef ZEPHYR_INCLUDE_LTE_LINK_INFO_H_
#define ZEPHYR_INCLUDE_LTE_LINK_INFO_H_

/* The largest expected parameter response size */
#define LTE_LI_MAX_RESPONSE_SIZE 16

/**@brief LTE link status data. */
enum lte_link_status {
	LTE_LINK_STATUS_RSSI,      /**< Signal strength */
	LTE_LINK_STATUS_BAND,      /**< Current band */
	LTE_LINK_STATUS_OPERATOR,  /**< Operator name */
	LTE_LINK_STATUS_IP_ADDRESS,/**< IP address */
};

/**@brief LTE link data type. */
enum lte_link_data_type {
	LTE_LINK_DATA_TYPE_INT,		/**< Int */
	LTE_LINK_DATA_TYPE_SHORT,	/**< Short */
	LTE_LINK_DATA_TYPE_STRING,	/**< String */
};

enum lte_link_data_behavior {
	LTE_LINK_BEHAVIOR_STATIC,	/**< Static */
	LTE_LINK_BEHAVIOR_DYNAMIC,	/**< Dynamic */
};

/** @brief Function for initializing the link information driver
 *
 * @return Zero on success or (negative) error code otherwise.
 */
int lte_li_init(void);

/** @brief Function for requesting the current LTE status of any predefined
 * status value.
 *
 * @return Length of received data or (negative) error code otherwise.
 */
int lte_li_link_status_update(enum lte_link_status link_status,
				enum lte_link_data_type data_type,
				char *buffer,
				size_t max_len);

#endif /* ZEPHYR_INCLUDE_LTE_LINK_INFO_H_ */
