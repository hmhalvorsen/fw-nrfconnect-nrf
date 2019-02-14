/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <at_cmd_parser.h>
#include <device.h>
#include <errno.h>
#include <lte_li.h>
#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(lte_li);

#define LI_CMD_EXT_SIG_QUALITY "AT+CESQ"
#define LI_CMD_CURRENT_BAND "AT%XCBAND"
#define LI_CMD_CURRENT_OP "AT+COPS?"
#define LI_CMD_PDP_CONTEXT "AT+CGDCONT?"

#define LI_RSSI_VALID_PARAM 5
#define LI_RSSI_PARAM_COUNT 6

#define LI_BAND_VALID_PARAM 0
#define LI_BAND_PARAM_COUNT 1

#define LI_OPERATOR_VALID_PARAM 2
#define LI_OPERATOR_PARAM_COUNT 4

#define LI_IP_ADDRESS_VALID_PARAM 3
#define LI_IP_ADDRESS_PARAM_COUNT 6

#define LI_CMD_SIZE(x) (strlen(x) - 1)

struct lte_link_data {
	const char *cmd;
	u8_t valid_param;
	u8_t param_count;
};

static struct lte_link_data rssi_data = {
	.cmd = LI_CMD_EXT_SIG_QUALITY,
	.valid_param = LI_RSSI_VALID_PARAM,
	.param_count = LI_RSSI_PARAM_COUNT,
};

static struct lte_link_data band_data = {
	.cmd = LI_CMD_CURRENT_BAND,
	.valid_param = LI_BAND_VALID_PARAM,
	.param_count = LI_BAND_PARAM_COUNT,
};

static struct lte_link_data operator_data = {
	.cmd = LI_CMD_CURRENT_OP,
	.valid_param = LI_OPERATOR_VALID_PARAM,
	.param_count = LI_OPERATOR_PARAM_COUNT,
};

static struct lte_link_data ip_data = {
	.cmd = LI_CMD_PDP_CONTEXT,
	.valid_param = LI_IP_ADDRESS_VALID_PARAM,
	.param_count = LI_IP_ADDRESS_PARAM_COUNT,
};

static struct lte_link_data *lte_data[] = {
	[LTE_LINK_STATUS_RSSI] = &rssi_data,
	[LTE_LINK_STATUS_BAND] = &band_data,
	[LTE_LINK_STATUS_OPERATOR] = &operator_data,
	[LTE_LINK_STATUS_IP_ADDRESS] = &ip_data,
};

/**
 * @brief Shared parameter list used to store responses from all AT commands.
 */
static struct at_param_list m_param_list;

static int at_cmd_send(int fd, const char *cmd, size_t size, char *resp_buffer)
{
	int len;

	LOG_DBG("send: %s\n", cmd);
	len = send(fd, cmd, size, 0);
	LOG_DBG("len: %d\n", len);
	if (len != size) {
		LOG_DBG("send: failed\n");
		return -EIO;
	}

	len = recv(fd, resp_buffer, CONFIG_LTE_LI_BUFFER_SIZE, 0);

	if (len < 0) {
		return -EFAULT;
	}

	return 0;
}

int lte_li_link_status_cmd_send(const char *cmd, char *response_buffer)
{
	int err;
	int at_socket_fd;
	size_t size = strlen(cmd);

	at_socket_fd = socket(AF_LTE, 0, NPROTO_AT);
	if (at_socket_fd == -1) {
		return -EFAULT;
	}

	err = at_cmd_send(at_socket_fd, cmd, size, response_buffer);
	close(at_socket_fd);

	return err;
}

int lte_li_link_status_recv(struct lte_link_data *lte_data)
{
	int err;
	char data_buffer[CONFIG_LTE_LI_BUFFER_SIZE];
	u32_t valid_params;

	err = lte_li_link_status_cmd_send(lte_data->cmd, data_buffer);
	if (err) {
		return err;
	}

	err = at_parser_max_params_from_str(
		&data_buffer[LI_CMD_SIZE(lte_data->cmd)], &m_param_list,
		lte_data->param_count);

	if (err != 0) {
		return err;
	}

	valid_params = at_params_get_valid_count(&m_param_list);
	if (valid_params != lte_data->param_count) {
		return -EAGAIN;
	}

	return err;
}

int lte_li_link_status_get_short(char *data_buffer, size_t valid_param,
					size_t max_len)
{
	int err;
	u16_t param_value;

	err = at_params_get_short(&m_param_list, valid_param,
					  &param_value);

	if (err != 0) {
		return err;
	}

	return snprintf(data_buffer, max_len,
					"%d", param_value);
}

int lte_li_link_status_get_string(char *data_buffer, size_t valid_param,
					size_t max_len)
{
	return at_params_get_string(&m_param_list, valid_param,
					   data_buffer, max_len);
}

int lte_li_link_status_update(enum lte_link_status link_status,
				enum lte_link_data_type data_type,
				char *data_buffer,
				size_t max_len)
{
	int err;
	int len = 0;

	if (data_buffer == NULL || max_len == 0) {
		return -EINVAL;
	}

	err = lte_li_link_status_recv(lte_data[link_status]);

	if (err != 0) {
		return err;
	}

	if (data_type == LTE_LINK_DATA_TYPE_SHORT) {
		len = lte_li_link_status_get_short(data_buffer,
				lte_data[link_status]->valid_param,
				max_len);
	} else if (data_type == LTE_LINK_DATA_TYPE_STRING) {
		len = lte_li_link_status_get_string(data_buffer,
				lte_data[link_status]->valid_param,
				max_len);
	}

	return len <= 0 ? -ENOTSUP : len;
}

int lte_li_init(void)
{
	int err = at_params_list_init(&m_param_list,
				CONFIG_LTE_LI_MAX_AT_PARAMS_RSP);

	return err ? -EFAULT : 0;
}

