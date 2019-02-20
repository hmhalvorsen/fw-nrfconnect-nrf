/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <at_cmd_parser.h>
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

#define AT_CMD_CESQ		"AT%CESQ"
#define AT_CMD_CESQ_ON		"AT%CESQ=1"
#define AT_CMD_CESQ_OFF		"AT%CESQ=0"
#define AT_CMD_CESQ_RESP	"%CESQ"
#define AT_CMD_CURRENT_BAND	"AT%XCBAND"
#define AT_CMD_CURRENT_MODE	"AT+CEMODE?"
#define AT_CMD_CURRENT_OP	"AT+COPS?"
#define AT_CMD_CELLID		"AT+CEREG?"
#define AT_CMD_PDP_CONTEXT	"AT+CGDCONT?"
#define AT_CMD_UICC_STATE	"AT%XSIM?"
#define AT_CMD_VBAT		"AT%XVBAT"
#define AT_CMD_TEMP		"AT%XTEMP"
#define AT_CMD_FW_VERSION	"AT+CGMR"

#define MI_RSRP_VALID_PARAM 0
#define MI_RSRP_PARAM_COUNT 2
#define MI_RSRP_OFFSET_VAL 141

#define MI_BAND_VALID_PARAM 0
#define MI_BAND_PARAM_COUNT 1

#define MI_MODE_VALID_PARAM 0
#define MI_MODE_PARAM_COUNT 1

#define MI_OPERATOR_VALID_PARAM 2
#define MI_OPERATOR_PARAM_COUNT 4

#define MI_CELLID_VALID_PARAM 3
#define MI_CELLID_PARAM_COUNT 5

#define MI_IP_ADDRESS_VALID_PARAM 3
#define MI_IP_ADDRESS_PARAM_COUNT 6

#define MI_UICC_VALID_PARAM 0
#define MI_UICC_PARAM_COUNT 1

#define MI_VBAT_VALID_PARAM 0
#define MI_VBAT_PARAM_COUNT 1

#define MI_TEMP_VALID_PARAM 1
#define MI_TEMP_PARAM_COUNT 2

#define MI_FW_VALID_PARAM 0
#define MI_FW_PARAM_COUNT 1

#define MI_CMD_SIZE(x) (strlen(x) - 1)

struct modem_info_data {
	const char *cmd;
	u8_t valid_param;
	u8_t param_count;
	enum modem_data_type data_type;
};

static struct modem_info_data rsrp_data = {
	.cmd = AT_CMD_CESQ,
	.valid_param = MI_RSRP_VALID_PARAM,
	.param_count = MI_RSRP_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_SHORT,
};

static struct modem_info_data band_data = {
	.cmd = AT_CMD_CURRENT_BAND,
	.valid_param = MI_BAND_VALID_PARAM,
	.param_count = MI_BAND_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_SHORT,
};

static struct modem_info_data mode_data = {
	.cmd = AT_CMD_CURRENT_MODE,
	.valid_param = MI_MODE_VALID_PARAM,
	.param_count = MI_MODE_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_SHORT,
};

static struct modem_info_data operator_data = {
	.cmd = AT_CMD_CURRENT_OP,
	.valid_param = MI_OPERATOR_VALID_PARAM,
	.param_count = MI_OPERATOR_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_STRING,
};

static struct modem_info_data cellid_data = {
	.cmd = AT_CMD_CELLID,
	.valid_param = MI_CELLID_VALID_PARAM,
	.param_count = MI_CELLID_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_STRING,
};

static struct modem_info_data ip_data = {
	.cmd = AT_CMD_PDP_CONTEXT,
	.valid_param = MI_IP_ADDRESS_VALID_PARAM,
	.param_count = MI_IP_ADDRESS_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_STRING,
};

static struct modem_info_data uicc_data = {
	.cmd = AT_CMD_UICC_STATE,
	.valid_param = MI_UICC_VALID_PARAM,
	.param_count = MI_UICC_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_SHORT,
};

static struct modem_info_data battery_data = {
	.cmd = AT_CMD_VBAT,
	.valid_param = MI_VBAT_VALID_PARAM,
	.param_count = MI_VBAT_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_SHORT,
};

static struct modem_info_data temp_data = {
	.cmd = AT_CMD_TEMP,
	.valid_param = MI_TEMP_VALID_PARAM,
	.param_count = MI_TEMP_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_SHORT,
};

static struct modem_info_data fw_data = {
	.cmd = AT_CMD_FW_VERSION,
	.valid_param = MI_FW_VALID_PARAM,
	.param_count = MI_FW_PARAM_COUNT,
	.data_type = MODEM_DATA_TYPE_STRING,
};

static struct modem_info_data *modem_data[] = {
	[MODEM_STATUS_RSRP] = &rsrp_data,
	[MODEM_STATUS_BAND] = &band_data,
	[MODEM_STATUS_MODE] = &mode_data,
	[MODEM_STATUS_OPERATOR] = &operator_data,
	[MODEM_STATUS_CELLID] = &cellid_data,
	[MODEM_STATUS_IP_ADDRESS] = &ip_data,
	[MODEM_STATUS_UICC] = &uicc_data,
	[MODEM_STATUS_BATTERY] = &battery_data,
	[MODEM_STATUS_TEMP] = &temp_data,
	[MODEM_STATUS_FW_VERSION] = &fw_data,
};

static const char * const modem_data_name[] = {
	[MODEM_STATUS_RSRP] = "RSRP",
	[MODEM_STATUS_BAND] = "BAND",
	[MODEM_STATUS_MODE] = "MODE",
	[MODEM_STATUS_OPERATOR] = "OPERATOR",
	[MODEM_STATUS_CELLID] = "CELLID",
	[MODEM_STATUS_IP_ADDRESS] = "IP ADDRESS",
	[MODEM_STATUS_UICC] = "SIM",
	[MODEM_STATUS_BATTERY] = "BATTERY",
	[MODEM_STATUS_TEMP] = "TEMP",
	[MODEM_STATUS_FW_VERSION] = "FW",
};

static rsrp_cb_t modem_info_rsrp_cb;

static struct at_param_list m_param_list;
static struct k_thread socket_thread;
static K_THREAD_STACK_DEFINE(socket_thread_stack, THREAD_STACK_SIZE);
static struct k_mutex socket_mutex;
static int at_socket_fd = INVALID_DESCRIPTOR;
static struct pollfd fds[1];
static int nfds;

static int at_cmd_send(int fd, const char *cmd, size_t size, char *resp_buffer)
{
	int len;

	k_mutex_lock(&socket_mutex, K_FOREVER);

	LOG_DBG("send: %s\n", cmd);
	len = send(fd, cmd, size, 0);
	LOG_DBG("len: %d\n", len);
	if (len != size) {
		LOG_DBG("send: failed\n");
		return -EIO;
	}

	if (resp_buffer != NULL) {
		len = recv(fd, resp_buffer, CONFIG_MODEM_INFO_BUFFER_SIZE, 0);
	}

	if (len < 0) {
		return -EFAULT;
	}

	k_mutex_unlock(&socket_mutex);

	return 0;
}

static bool is_cesq_notification(char *buf, size_t len)
{
	if ((len < MI_CMD_SIZE(AT_CMD_CESQ_RESP)) ||
	    (buf[4] != AT_CMD_CESQ_RESP[4])) {
		return false;
	}

	return strstr(buf, AT_CMD_CESQ_RESP) ? true : false;
}

enum modem_data_type modem_info_get_type(enum modem_status status)
{
	__ASSERT(status < MODEM_STATUS_COUNT, "Invalid argument.");

	return modem_data[status]->data_type;
}

int modem_info_get_name(enum modem_status status, char *data_name)
{
	int len;

	if (data_name == NULL) {
		return -EINVAL;
	}

	memcpy(data_name,
		modem_data_name[status],
		strlen(modem_data_name[status]));

	len = strlen(modem_data_name[status]);

	return len <= 0 ? -EINVAL : len;
}

int modem_info_get_short(char *data_buffer, size_t valid_param,
			size_t max_len, size_t offset_value)
{
	int err;
	u16_t param_value;

	err = at_params_get_short(&m_param_list, valid_param,
					  &param_value);

	if (err != 0) {
		return err;
	}

	return snprintf(data_buffer, max_len,
			"%d", param_value - offset_value);
}

int modem_info_get_string(char *data_buffer, size_t valid_param, size_t max_len)
{
	return at_params_get_string(&m_param_list, valid_param,
					   data_buffer, max_len);
}

int modem_info_parse(struct modem_info_data *modem_data, char *buf)
{
	int err;
	u32_t valid_params;

	err = at_parser_max_params_from_str(
		&buf[MI_CMD_SIZE(modem_data->cmd)], &m_param_list,
		modem_data->param_count);

	if (err != 0) {
		return err;
	}

	valid_params = at_params_get_valid_count(&m_param_list);
	if (valid_params != modem_data->param_count) {
		return -EAGAIN;
	}

	return err;
}

int modem_info_update(enum modem_status status,	char *data_buffer)
{
	int err;
	int len = 0;
	char response_buffer[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};

	if (data_buffer == NULL) {
		return -EINVAL;
	}

	err = at_cmd_send(at_socket_fd,
			modem_data[status]->cmd,
			strlen(modem_data[status]->cmd),
			response_buffer);

	if (err) {
		return err;
	}

	err = modem_info_parse(modem_data[status], response_buffer);

	if (err) {
		return err;
	}

	if (modem_data[status]->data_type == MODEM_DATA_TYPE_SHORT) {
		len = modem_info_get_short(data_buffer,
				modem_data[status]->valid_param,
				MODEM_INFO_MAX_RESPONSE_SIZE,
				0);
	} else if (modem_data[status]->data_type == MODEM_DATA_TYPE_STRING) {
		len = modem_info_get_string(data_buffer,
				modem_data[status]->valid_param,
				MODEM_INFO_MAX_RESPONSE_SIZE);
	}

	return len <= 0 ? -ENOTSUP : len;
}

static void modem_info_rsrp_subscribe_thread(void *arg1, void *arg2, void *arg3)
{
	char response_buffer[CONFIG_MODEM_INFO_BUFFER_SIZE] = {0};
	char rsrp_val[MODEM_INFO_MAX_RESPONSE_SIZE] = {0};
	int err;
	int r_bytes;
	int len;

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
		err = poll(fds, nfds, K_FOREVER);
		if (err < 0) {
			LOG_ERR("Poll error: %d\n", err);
			continue;
		} else if (err == 0) {
			LOG_DBG("Timeout");
			k_sleep(100);
			continue;
		}

		k_mutex_lock(&socket_mutex, K_FOREVER);
		r_bytes = recv(at_socket_fd, response_buffer,
				sizeof(response_buffer), MSG_DONTWAIT);
		k_mutex_unlock(&socket_mutex);

		if (is_cesq_notification(response_buffer, r_bytes)) {
			modem_info_parse(modem_data[MODEM_STATUS_RSRP],
					response_buffer);
			len = modem_info_get_short(rsrp_val,
				modem_data[MODEM_STATUS_RSRP]->valid_param,
				MODEM_INFO_MAX_RESPONSE_SIZE,
				MI_RSRP_OFFSET_VAL);
			modem_info_rsrp_cb(rsrp_val, len);
		}
	}
}

int modem_info_rsrp_sub_init(rsrp_cb_t cb)
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
	fds[0].fd = at_socket_fd;
	fds[0].events = ZSOCK_POLLIN;
	nfds += 1;

	if (at_socket_fd == INVALID_DESCRIPTOR) {
		LOG_ERR("Creating at_socket failed\n");
		return -EFAULT;
	}

	/* Init thread for RSRP subscription */
	k_mutex_init(&socket_mutex);

	return err ? -EFAULT : 0;
}

