/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <at_cmd_parser.h>
#include <ctype.h>
#include <device.h>
#include <errno.h>
#include <modem_info.h>
#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(modem_info);

#define INVALID_DESCRIPTOR	-1
#define THREAD_STACK_SIZE	(CONFIG_MODEM_INFO_SOCKET_BUF_SIZE + 512)
#define THREAD_PRIORITY		K_PRIO_PREEMPT(CONFIG_MODEM_INFO_THREAD_PRIO)

#define AT_CMD_CESQ		"AT+CESQ"
#define AT_CMD_CESQ_ON		"AT%CESQ=1"
#define AT_CMD_CESQ_OFF		"AT%CESQ=0"
#define AT_CMD_CESQ_RESP	"%CESQ"
#define AT_CMD_CURRENT_BAND	"AT%XCBAND"
#define AT_CMD_SUPPORTED_BAND	"AT%XCBAND=?"
#define AT_CMD_CURRENT_MODE	"AT+CEMODE?"
#define AT_CMD_CURRENT_OP	"AT+COPS?"
#define AT_CMD_NETWORK_STATUS	"AT+CEREG?"
#define AT_CMD_PDP_CONTEXT	"AT+CGDCONT?"
#define AT_CMD_UICC_STATE	"AT%XSIM?"
#define AT_CMD_VBAT		"AT%XVBAT"
#define AT_CMD_TEMP		"AT%XTEMP"
#define AT_CMD_FW_VERSION	"AT+CGMR"
#define AT_CMD_CRSM		"AT+CRSM"
#define AT_CMD_ICCID		"AT+CRSM=176,12258,0,0,10"
#define AT_CMD_SYSTEMMODE	"AT%XSYSTEMMODE?"
#define AT_CMD_SUCCESS_SIZE	5

#define RSRP_DATA_NAME		"signalStrength"
#define CUR_BAND_DATA_NAME	"curBand"
#define SUP_BAND_DATA_NAME	"supBands"
#define UE_MODE_DATA_NAME	"ueMode"
#define OPERATOR_DATA_NAME	"curOp"
#define MCC_DATA_NAME		"mcc"
#define MNC_DATA_NAME		"mnc"
#define AREA_CODE_DATA_NAME	"areaCode"
#define CELLID_DATA_NAME	"cellID"
#define IP_ADDRESS_DATA_NAME	"ipAdr"
#define UICC_DATA_NAME		"uiccState"
#define BATTERY_DATA_NAME	"btryVolt"
#define TEMPERATURE_DATA_NAME	"temp"
#define MODEM_FW_DATA_NAME	"mFW"
#define ICCID_DATA_NAME		"iccid"
#define LTE_MODE_DATA_NAME	"lteMode"
#define NBIOT_MODE_DATA_NAME	"nbiotMode"
#define GPS_MODE_DATA_NAME	"gpsMode"

#define RSRP_PARAM_INDEX	0
#define RSRP_PARAM_COUNT	2
#define RSRP_OFFSET_VAL		141

#define BAND_PARAM_INDEX	0 /* Index of desired parameter */
#define BAND_PARAM_COUNT	1 /* Number of parameters */

#define MODE_PARAM_INDEX	0
#define MODE_PARAM_COUNT	1

#define OPERATOR_PARAM_INDEX	2
#define OPERATOR_PARAM_COUNT	4

#define CELLID_PARAM_INDEX	3
#define CELLID_PARAM_COUNT	5

#define AREA_CODE_PARAM_INDEX	2
#define AREA_CODE_PARAM_COUNT	5

#define IP_ADDRESS_PARAM_INDEX	3
#define IP_ADDRESS_PARAM_COUNT	6

#define UICC_PARAM_INDEX	0
#define UICC_PARAM_COUNT	1

#define VBAT_PARAM_INDEX	0
#define VBAT_PARAM_COUNT	1

#define TEMP_PARAM_INDEX	1
#define TEMP_PARAM_COUNT	2

#define MODEM_FW_PARAM_INDEX	0
#define MODEM_FW_PARAM_COUNT	1

#define ICCID_PARAM_INDEX	2
#define ICCID_PARAM_COUNT	3

#define LTE_MODE_PARAM_INDEX	0
#define NBIOT_MODE_PARAM_INDEX	1
#define GPS_MODE_PARAM_INDEX	2
#define SYSTEMMODE_PARAM_COUNT	4

static const char success[] = "OK";

struct modem_info_data {
	const char *cmd;
	const char *data_name;
	u8_t param_index;
	u8_t param_count;
	enum at_param_type data_type;
};

static const struct modem_info_data rsrp_data = {
	.cmd		= AT_CMD_CESQ,
	.data_name	= RSRP_DATA_NAME,
	.param_index	= RSRP_PARAM_INDEX,
	.param_count	= RSRP_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data band_data = {
	.cmd		= AT_CMD_CURRENT_BAND,
	.data_name	= CUR_BAND_DATA_NAME,
	.param_index	= BAND_PARAM_INDEX,
	.param_count	= BAND_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data band_sup_data = {
	.cmd		= AT_CMD_SUPPORTED_BAND,
	.data_name	= SUP_BAND_DATA_NAME,
	.param_index	= BAND_PARAM_INDEX,
	.param_count	= BAND_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data mode_data = {
	.cmd		= AT_CMD_CURRENT_MODE,
	.data_name	= UE_MODE_DATA_NAME,
	.param_index	= MODE_PARAM_INDEX,
	.param_count	= MODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data operator_data = {
	.cmd		= AT_CMD_CURRENT_OP,
	.data_name	= OPERATOR_DATA_NAME,
	.param_index	= OPERATOR_PARAM_INDEX,
	.param_count	= OPERATOR_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data mcc_data = {
	.cmd		= AT_CMD_CURRENT_OP,
	.data_name	= MCC_DATA_NAME,
	.param_index	= OPERATOR_PARAM_INDEX,
	.param_count	= OPERATOR_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data mnc_data = {
	.cmd		= AT_CMD_CURRENT_OP,
	.data_name	= MNC_DATA_NAME,
	.param_index	= OPERATOR_PARAM_INDEX,
	.param_count	= OPERATOR_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data cellid_data = {
	.cmd		= AT_CMD_NETWORK_STATUS,
	.data_name	= CELLID_DATA_NAME,
	.param_index	= CELLID_PARAM_INDEX,
	.param_count	= CELLID_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data area_data = {
	.cmd		= AT_CMD_NETWORK_STATUS,
	.data_name	= AREA_CODE_DATA_NAME,
	.param_index	= AREA_CODE_PARAM_INDEX,
	.param_count	= AREA_CODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data ip_data = {
	.cmd		= AT_CMD_PDP_CONTEXT,
	.data_name	= IP_ADDRESS_DATA_NAME,
	.param_index	= IP_ADDRESS_PARAM_INDEX,
	.param_count	= IP_ADDRESS_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data uicc_data = {
	.cmd		= AT_CMD_UICC_STATE,
	.data_name	= UICC_DATA_NAME,
	.param_index	= UICC_PARAM_INDEX,
	.param_count	= UICC_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data battery_data = {
	.cmd		= AT_CMD_VBAT,
	.data_name	= BATTERY_DATA_NAME,
	.param_index	= VBAT_PARAM_INDEX,
	.param_count	= VBAT_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data temp_data = {
	.cmd		= AT_CMD_TEMP,
	.data_name	= TEMPERATURE_DATA_NAME,
	.param_index	= TEMP_PARAM_INDEX,
	.param_count	= TEMP_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data fw_data = {
	.cmd		= AT_CMD_FW_VERSION,
	.data_name	= MODEM_FW_DATA_NAME,
	.param_index	= MODEM_FW_PARAM_INDEX,
	.param_count	= MODEM_FW_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data iccid_data = {
	.cmd		= AT_CMD_ICCID,
	.data_name	= ICCID_DATA_NAME,
	.param_index	= ICCID_PARAM_INDEX,
	.param_count	= ICCID_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_STRING,
};

static const struct modem_info_data lte_mode_data = {
	.cmd		= AT_CMD_SYSTEMMODE,
	.data_name	= LTE_MODE_DATA_NAME,
	.param_index	= LTE_MODE_PARAM_INDEX,
	.param_count	= SYSTEMMODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data nbiot_mode_data = {
	.cmd		= AT_CMD_SYSTEMMODE,
	.data_name	= NBIOT_MODE_DATA_NAME,
	.param_index	= NBIOT_MODE_PARAM_INDEX,
	.param_count	= SYSTEMMODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data gps_mode_data = {
	.cmd		= AT_CMD_SYSTEMMODE,
	.data_name	= GPS_MODE_DATA_NAME,
	.param_index	= GPS_MODE_PARAM_INDEX,
	.param_count	= SYSTEMMODE_PARAM_COUNT,
	.data_type	= AT_PARAM_TYPE_NUM_SHORT,
};

static const struct modem_info_data *const modem_data[] = {
	[MODEM_INFO_RSRP]	= &rsrp_data,
	[MODEM_INFO_CUR_BAND]	= &band_data,
	[MODEM_INFO_SUP_BAND]	= &band_sup_data,
	[MODEM_INFO_UE_MODE]	= &mode_data,
	[MODEM_INFO_OPERATOR]	= &operator_data,
	[MODEM_INFO_MCC]	= &mcc_data,
	[MODEM_INFO_MNC]	= &mnc_data,
	[MODEM_INFO_AREA_CODE]	= &area_data,
	[MODEM_INFO_CELLID]	= &cellid_data,
	[MODEM_INFO_IP_ADDRESS]	= &ip_data,
	[MODEM_INFO_UICC]	= &uicc_data,
	[MODEM_INFO_BATTERY]	= &battery_data,
	[MODEM_INFO_TEMP]	= &temp_data,
	[MODEM_INFO_FW_VERSION]	= &fw_data,
	[MODEM_INFO_ICCID]	= &iccid_data,
	[MODEM_INFO_LTE_MODE]	= &lte_mode_data,
	[MODEM_INFO_NBIOT_MODE] = &nbiot_mode_data,
	[MODEM_INFO_GPS_MODE]   = &gps_mode_data,
};

static rsrp_cb_t modem_info_rsrp_cb;

static struct at_param_list m_param_list;
static struct k_thread socket_thread;
static K_THREAD_STACK_DEFINE(socket_thread_stack, THREAD_STACK_SIZE);
static struct k_mutex socket_mutex;
static int at_socket_fd = INVALID_DESCRIPTOR;
static struct pollfd fds;
static int nfds;

static int at_cmd_send(int fd, const char *cmd, size_t size, char *resp_buffer)
{
	int len;
	size_t ret = 0;

	k_mutex_lock(&socket_mutex, K_FOREVER);

	LOG_DBG("send: %s\n", cmd);
	len = send(fd, cmd, size, 0);
	LOG_DBG("len: %d\n", len);
	if (len != size) {
		LOG_DBG("send: failed\n");
		ret = -EIO;
	}

	if (resp_buffer != NULL && ret == 0) {
		len = recv(fd, resp_buffer, CONFIG_MODEM_INFO_BUFFER_SIZE, 0);

		if (memcmp(success,
			   &resp_buffer[len-AT_CMD_SUCCESS_SIZE],
			   strlen(success)) != 0) {
			LOG_DBG("recv: %s", resp_buffer);
			ret = -EIO;
		}
	}

	k_mutex_unlock(&socket_mutex);

	return ret;
}

static bool is_cesq_notification(char *buf, size_t len)
{
	return strstr(buf, AT_CMD_CESQ_RESP) ? true : false;
}

static void flip_iccid_string(char *buf)
{
	u8_t current_char;
	u8_t next_char;

	for (size_t i = 0; i < strlen(buf); i = i + 2) {
		current_char = buf[i];
		next_char = buf[i + 1];

		buf[i] = next_char;
		buf[i + 1] = current_char;
	}
}

static int modem_info_remove_cmd(char *buf)
{
	size_t cmd_length = 0;

	while (!isspace(*buf)) {
		buf++;
		cmd_length++;
	}

	return cmd_length;
}

static int modem_info_parse(const struct modem_info_data *modem_data, char *buf)
{
	int err;
	u32_t param_index;

	err = at_parser_max_params_from_str(buf, &m_param_list,
					    modem_data->param_count);

	if (err != 0) {
		return err;
	}

	param_index = at_params_valid_count_get(&m_param_list);
	if (param_index != modem_data->param_count) {
		return -EAGAIN;
	}

	return err;
}

enum at_param_type modem_info_type_get(enum modem_info info)
{
	if (info >= MODEM_INFO_COUNT) {
		return -EINVAL;
	}

	return modem_data[info]->data_type;
}

int modem_info_name_get(enum modem_info info, char *name)
{
	int len;

	if (name == NULL) {
		return -EINVAL;
	}

	len = strlen(modem_data[info]->data_name);

	if (len <= 0) {
		return -EINVAL;
	}

	memcpy(name,
		modem_data[info]->data_name,
		len);

	return len;
}

int modem_info_short_get(enum modem_info info, u16_t *buf)
{
	int err;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	size_t cmd_length = 0;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (modem_data[info]->data_type == AT_PARAM_TYPE_STRING) {
		return -EINVAL;
	}

	err = at_cmd_send(at_socket_fd,
			modem_data[info]->cmd,
			strlen(modem_data[info]->cmd),
			recv_buf);

	if (err) {
		return err;
	}

	if (info != MODEM_INFO_FW_VERSION) {
		cmd_length = modem_info_remove_cmd(recv_buf);
	}

	err = modem_info_parse(modem_data[info], &recv_buf[cmd_length]);

	if (err) {
		return err;
	}

	err = at_params_short_get(&m_param_list,
				  modem_data[info]->param_index,
				  buf);

	if (err) {
		return err;
	}

	return sizeof(u16_t);
}

int modem_info_string_get(enum modem_info info, char *buf)
{
	int err;
	int len = 0;
	char recv_buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	u16_t param_value;
	size_t cmd_length = 0;

	if (buf == NULL) {
		return -EINVAL;
	}

	err = at_cmd_send(at_socket_fd,
			modem_data[info]->cmd,
			strlen(modem_data[info]->cmd),
			recv_buf);

	if (err) {
		return err;
	}

	if (info != MODEM_INFO_FW_VERSION) {
		cmd_length = modem_info_remove_cmd(recv_buf);
	}

	err = modem_info_parse(modem_data[info], &recv_buf[cmd_length]);

	if (err) {
		LOG_DBG("Unable to parse data: %d\n", err);
		return err;
	}

	if (modem_data[info]->data_type == AT_PARAM_TYPE_NUM_SHORT) {
		err = at_params_short_get(&m_param_list,
					  modem_data[info]->param_index,
					  &param_value);
		if (err) {
			LOG_DBG("Unable to obtain short: %d\n", err);
			return err;
		}

		len = snprintf(buf, MODEM_INFO_MAX_RESPONSE_SIZE,
				"%d", param_value);
	} else if (modem_data[info]->data_type == AT_PARAM_TYPE_STRING) {
		len = at_params_string_get(&m_param_list,
					   modem_data[info]->param_index,
					   buf,
					   MODEM_INFO_MAX_RESPONSE_SIZE);
	}

	if (info == MODEM_INFO_ICCID) {
		flip_iccid_string(buf);
	}

	return len <= 0 ? -ENOTSUP : len;
}

static void modem_info_rsrp_subscribe_thread(void *arg1, void *arg2, void *arg3)
{
	char buf[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	u16_t param_value;
	int err;
	int r_bytes;
	size_t cmd_length;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	err = at_cmd_send(at_socket_fd,
			AT_CMD_CESQ_ON,
			strlen(AT_CMD_CESQ_ON),
			NULL);

	if (err) {
		LOG_ERR("AT cmd error: %d\n", err);
	}

	while (1) {
		k_mutex_lock(&socket_mutex, K_FOREVER);
		r_bytes = recv(at_socket_fd, buf, sizeof(buf), 0);
		k_mutex_unlock(&socket_mutex);

		if (!is_cesq_notification(buf, r_bytes)) {
			continue;
		}

		cmd_length = modem_info_remove_cmd(buf);
		modem_info_parse(modem_data[MODEM_INFO_RSRP],
				 &buf[cmd_length]);
		at_params_short_get(&m_param_list,
				    modem_data[MODEM_INFO_RSRP]->param_index,
				    &param_value);
		modem_info_rsrp_cb(param_value);
	}
}

int modem_info_rsrp_register(rsrp_cb_t cb)
{
	modem_info_rsrp_cb = cb;

	k_thread_create(&socket_thread, socket_thread_stack,
			K_THREAD_STACK_SIZEOF(socket_thread_stack),
			modem_info_rsrp_subscribe_thread,
			NULL, NULL, NULL,
			THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

int modem_info_init(void)
{
	/* Init at_cmd_parser storage module */
	int err = at_params_list_init(&m_param_list,
				CONFIG_MODEM_INFO_MAX_AT_PARAMS_RSP);

	/* Occupy socket for AT command transmission */
	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	fds.fd = at_socket_fd;
	fds.events = ZSOCK_POLLIN;
	nfds += 1;

	if (at_socket_fd == INVALID_DESCRIPTOR) {
		LOG_ERR("Creating at_socket failed\n");
		return -EFAULT;
	}

	/* Init thread for RSRP subscription */
	k_mutex_init(&socket_mutex);

	return err;
}
