/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON.h>
#include <modem_info.h>
#include <at_params.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(modem_info_json);

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

static int json_add_bool(cJSON *parent, const char *str, int var)
{
	cJSON *json_bool;

	json_bool = cJSON_CreateBool(var);
	if (json_bool == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_bool);
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

static int json_add_data(struct lte_param *param, cJSON *json_obj)
{
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE];
	enum at_param_type data_type;
	size_t total_len = 0;
	size_t ret;

	memset(data_name, 0, MODEM_INFO_MAX_RESPONSE_SIZE);
	ret = modem_info_name_get(param->type,
				data_name);
	if (ret < 0) {
		LOG_DBG("Data name not obtained: %d\n", ret);
		return -EINVAL;
	}

	data_type = modem_info_type_get(param->type);

	if (data_type < 0) {
		return -EINVAL;
	}

	if (data_type == AT_PARAM_TYPE_STRING &&
	    param->type != MODEM_INFO_AREA_CODE) {
		total_len += strlen(param->string);
		ret += json_add_str(json_obj, data_name, param->string);
	} else {
		total_len += sizeof(u16_t);
		ret += json_add_num(json_obj, data_name, param->value);
	}

	if (ret < 0) {
		return ret;
	}

	return total_len;
}

static int network_data_add(struct network_param *network, cJSON *json_obj)
{
	char data_name[MODEM_INFO_MAX_RESPONSE_SIZE];
	size_t total_len;
	int ret;
	int len;

	if (network == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	total_len = json_add_data(&network->cur_band, json_obj);
	total_len += json_add_data(&network->sup_band, json_obj);
	total_len += json_add_data(&network->area_code, json_obj);
	total_len += json_add_data(&network->mcc, json_obj);
	total_len += json_add_data(&network->mnc, json_obj);
	total_len += json_add_data(&network->ip_address, json_obj);
	total_len += json_add_data(&network->ue_mode, json_obj);

	len = modem_info_name_get(network->cellid_hex.type, data_name);
	data_name[len] =  '\0';
	ret = json_add_num(json_obj, data_name, network->cellid_dec);

	if (ret) {
		LOG_DBG("Unable to add the cell ID.\n");
	} else {
		total_len += sizeof(double);
	}

	len = modem_info_name_get(network->lte_mode.type, data_name);
	data_name[len] =  '\0';
	ret = json_add_bool(json_obj, data_name, network->lte_mode.value);

	if (ret) {
		LOG_DBG("Unable to add the LTE-M system mode.\n");
		return ret;
	} else {
		total_len += sizeof(bool);
	}

	len = modem_info_name_get(network->nbiot_mode.type, data_name);
	data_name[len] =  '\0';
	ret = json_add_bool(json_obj, data_name, network->nbiot_mode.value);

	if (ret) {
		LOG_DBG("Unable to add the NB-IoT system mode.\n");
		return ret;
	} else {
		total_len += sizeof(bool);
	}

	len = modem_info_name_get(network->gps_mode.type, data_name);
	data_name[len] =  '\0';
	ret = json_add_bool(json_obj, data_name, network->gps_mode.value);

	if (ret) {
		LOG_DBG("Unable to add the GPS system mode.\n");
		return ret;
	} else {
		total_len += sizeof(bool);
	}

	return total_len;
}

static int sim_data_add(struct sim_param *sim, cJSON *json_obj)
{
	size_t total_len;

	if (sim == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	total_len = json_add_data(&sim->uicc, json_obj);
	total_len += json_add_data(&sim->iccid, json_obj);

	return total_len;
}

static int device_data_add(struct device_param *device, cJSON *json_obj)
{
	size_t total_len;

	if (device == NULL || json_obj == NULL) {
		return -EINVAL;
	}

	total_len = json_add_data(&device->modem_fw, json_obj);
	total_len += json_add_data(&device->battery, json_obj);
	total_len += json_add_str(json_obj, "board", device->board);
	total_len += json_add_str(json_obj, "appVersion", device->app_version);
	total_len += json_add_str(json_obj, "appName", device->app_name);

	return total_len;
}

int modem_info_json_string_encode(struct modem_param_info *modem, char *buf)
{
	size_t total_len = 0;
	int ret;

	cJSON *data_obj		= cJSON_CreateObject();
	cJSON *network_obj	= cJSON_CreateObject();
	cJSON *sim_obj		= cJSON_CreateObject();
	cJSON *device_obj	= cJSON_CreateObject();

	if (data_obj == NULL || network_obj == NULL ||
	    sim_obj == NULL || device_obj == NULL) {
		return -ENOMEM;
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_DEVICE_STRING_NETWORK)) {
		ret = network_data_add(&modem->network, network_obj);

		if (ret < 0) {
			return ret;
		}

		total_len += ret;
		json_add_obj(data_obj, "networkInfo", network_obj);
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_DEVICE_STRING_SIM)) {
		ret = sim_data_add(&modem->sim, sim_obj);

		if (ret < 0) {
			return ret;
		}

		total_len += ret;
		json_add_obj(data_obj, "simInfo", sim_obj);
	}

	if (IS_ENABLED(CONFIG_MODEM_INFO_DEVICE_STRING_DEVICE)) {
		ret = device_data_add(&modem->device, device_obj);

		if (ret < 0) {
			return ret;
		}

		total_len += ret;
		json_add_obj(data_obj, "deviceInfo", device_obj);
	}

	if (total_len > 0) {
		memcpy(buf,
			cJSON_PrintUnformatted(data_obj),
			MODEM_INFO_JSON_STRING_SIZE);
	}

	cJSON_Delete(data_obj);

	return total_len;
}
