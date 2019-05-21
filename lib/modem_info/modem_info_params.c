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

LOG_MODULE_REGISTER(modem_info_params);

int modem_info_params_init(struct modem_param_info *modem)
{
	if (modem == NULL) {
		return -EINVAL;
	}

	modem->network.cur_band.type	= MODEM_INFO_CUR_BAND;
	modem->network.sup_band.type	= MODEM_INFO_SUP_BAND;
	modem->network.area_code.type	= MODEM_INFO_AREA_CODE;
	modem->network.operator.type	= MODEM_INFO_OPERATOR;
	modem->network.mcc.type		= MODEM_INFO_MCC;
	modem->network.mnc.type		= MODEM_INFO_MNC;
	modem->network.cellid_hex.type	= MODEM_INFO_CELLID;
	modem->network.ip_address.type	= MODEM_INFO_IP_ADDRESS;
	modem->network.ue_mode.type	= MODEM_INFO_UE_MODE;
	modem->network.lte_mode.type	= MODEM_INFO_LTE_MODE;
	modem->network.nbiot_mode.type	= MODEM_INFO_NBIOT_MODE;
	modem->network.gps_mode.type	= MODEM_INFO_GPS_MODE;

	modem->sim.uicc.type		= MODEM_INFO_UICC;
	modem->sim.iccid.type		= MODEM_INFO_ICCID;

	modem->device.modem_fw.type	= MODEM_INFO_FW_VERSION;
	modem->device.battery.type	= MODEM_INFO_BATTERY;
	modem->device.board		= CONFIG_BOARD;

	return 0;
}

static int area_code_parse(struct lte_param *area_code)
{
	if (area_code == NULL) {
		return -EINVAL;
	}

	area_code->string[2] = '\0';
	/* Parses the string, interpreting its content as an 	*/
	/* integral number with base 16. (Hexadecimal)		*/
	area_code->value = (double)strtol(area_code->string, NULL, 16);

	return 0;
}

static int mcc_mnc_parse(struct lte_param *operator,
			 struct lte_param *mcc,
			 struct lte_param *mnc)
{
	if (operator == NULL || mcc == NULL || mnc == NULL) {
		return -EINVAL;
	}

	memcpy(mcc->string, operator->string, 3);

	if (sizeof(operator->string) - 1 < 6) {
		mnc->string[0] = '0';
		memcpy(&mnc->string[1], &operator->string[3], 2);
	} else {
		memcpy(&mnc->string, &operator->string[3], 3);
	}

	/* Parses the string, interpreting its content as an	*/
	/* integral number with base 10. (Decimal)		*/

	mcc->value = (double)strtol(mcc->string, NULL, 10);
	mnc->value = (double)strtol(mnc->string, NULL, 10);

	return 0;
}

static int cellid_to_dec(struct lte_param *cellID, double *cellID_dec)
{
	/* Parses the string, interpreting its content as an	*/
	/* integral number with base 16. (Hexadecimal)		*/

	*cellID_dec = (double)strtol(cellID->string, NULL, 16);

	return 0;
}

static int modem_data_get(struct lte_param *param)
{
	enum at_param_type data_type;
	int ret;

	data_type = modem_info_type_get(param->type);

	if (data_type < 0) {
		return -EINVAL;
	}

	if (data_type == AT_PARAM_TYPE_STRING) {
		ret = modem_info_string_get(param->type, param->string);

		if (ret < 0) {
			LOG_DBG("Link data not obtained: %d\n", ret);
			return ret;
		}
	} else if (data_type == AT_PARAM_TYPE_NUM_SHORT) {
		ret = modem_info_short_get(param->type, &param->value);

		if (ret < 0) {
			LOG_DBG("Link data not obtained: %d\n", ret);
			return ret;
		}
	}

	return 0;
}

int modem_info_params_get(struct modem_param_info *modem)
{
	int ret;

	if (modem == NULL) {
		return -EINVAL;
	}

	ret = modem_data_get(&modem->network.cur_band);
	ret += modem_data_get(&modem->network.sup_band);
	ret += modem_data_get(&modem->network.ip_address);
	ret += modem_data_get(&modem->network.ue_mode);
	ret += modem_data_get(&modem->network.operator);
	ret += modem_data_get(&modem->network.cellid_hex);
	ret += modem_data_get(&modem->network.area_code);
	ret += modem_data_get(&modem->network.lte_mode);
	ret += modem_data_get(&modem->network.nbiot_mode);
	ret += modem_data_get(&modem->network.gps_mode);

	ret += mcc_mnc_parse(&modem->network.operator,
			     &modem->network.mcc,
			     &modem->network.mnc);
	ret += cellid_to_dec(&modem->network.cellid_hex,
			     &modem->network.cellid_dec);
	ret += area_code_parse(&modem->network.area_code);

	if (ret) {
		LOG_DBG("Network data not obtained: %d\n", ret);
		return ret;
	}

	ret += modem_data_get(&modem->sim.uicc);
	ret += modem_data_get(&modem->sim.iccid);

	if (ret) {
		LOG_DBG("Sim data not obtained: %d\n", ret);
		return ret;
	}

	ret += modem_data_get(&modem->device.modem_fw);
	ret += modem_data_get(&modem->device.battery);

	if (ret) {
		LOG_DBG("Device data not obtained: %d\n", ret);
		return ret;
	}

	return 0;
}
