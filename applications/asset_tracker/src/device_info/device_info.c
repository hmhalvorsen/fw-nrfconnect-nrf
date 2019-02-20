/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <nrf_cloud.h>
#include <cJSON.h>
#include <modem_info.h>
#include <device_info.h>

struct modem_info {
	enum nrf_cloud_sensor type;
	enum modem_status status;
};

static struct modem_info modem_info_rspr = {
	.type = NRF_CLOUD_LTE_LINK_RSRP,
	.status = MODEM_STATUS_RSRP,
};

static struct modem_info modem_info_band = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_BAND,
};

static struct modem_info modem_info_mode = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_MODE,
};

static struct modem_info modem_info_operator = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_OPERATOR,
};

static struct modem_info modem_info_cellid = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_CELLID,
};

static struct modem_info modem_info_ip_address = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_IP_ADDRESS,
};

static struct modem_info modem_info_uicc = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_UICC,
};

static struct modem_info modem_info_battery = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_BATTERY,
};

static struct modem_info modem_info_fw = {
	.type = NRF_CLOUD_DEVICE_INFO,
	.status = MODEM_STATUS_FW_VERSION,
};

static struct modem_info *modem_information[] = {
	&modem_info_rspr,
	&modem_info_band,
	&modem_info_mode,
	&modem_info_operator,
	&modem_info_cellid,
	&modem_info_ip_address,
	&modem_info_uicc,
	&modem_info_battery,
	&modem_info_fw,
};

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}

static int json_add_num(cJSON *parent, const char *str, double num)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(num);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_num);
}

static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

int device_info_get_json_string(char *string_buffer)
{
	size_t len = 0;
	size_t total_len = 0;
	int ret = 0;
	char data_buffer[MODEM_INFO_MAX_RESPONSE_SIZE] = {0};
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE] = {0};
	cJSON *data_obj = cJSON_CreateObject();
	enum modem_data_type data_type;

	if (data_obj == NULL) {
		return -ENOMEM;
	}

	for (size_t i = 0; i < ARRAY_SIZE(modem_information); i++) {
		if (modem_information[i]->type != NRF_CLOUD_DEVICE_INFO) {
			continue;
		}
		len = modem_info_update(modem_information[i]->status,
					data_buffer);
		if (len < 0) {
			printk("LTE link data not obtained: %d\n", len);
			continue;
		}
		total_len += len;

		len = modem_info_get_name(modem_information[i]->status,
					data_name);
		if (len < 0) {
			printk("Data name not obtained: %d\n", len);
			continue;
		}

		data_type = modem_info_get_type(modem_information[i]->status);

		if (data_type == MODEM_DATA_TYPE_STRING) {
			ret += json_add_str(data_obj, data_name, data_buffer);
		} else if (data_type == MODEM_DATA_TYPE_SHORT) {
			ret += json_add_num(data_obj, data_name,
					atoi(data_buffer));
		}

		memset(data_name, 0, MODEM_INFO_MAX_RESPONSE_SIZE);
	}

	if (ret != 0) {
		cJSON_Delete(data_obj);
		return -ENOMEM;
	}

	memcpy(string_buffer,
		cJSON_PrintUnformatted(data_obj),
		DEVICE_INFO_STRING_SIZE);

	cJSON_Delete(data_obj);

	return total_len;
}
