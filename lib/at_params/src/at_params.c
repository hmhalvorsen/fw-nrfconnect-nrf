/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <at_params.h>
#include <kernel.h>

#define AT_PARAMS_CALLOC k_calloc
#define AT_PARAMS_MALLOC k_malloc
#define AT_PARAMS_FREE k_free

/* Internal function. Parameter cannot be null. */
static void at_param_init(struct at_param *const param)
{
	memset(param, 0, sizeof(struct at_param));
}


/* Internal function. Parameter cannot be null. */
static void at_param_clear(struct at_param *const param)
{
	if (param == NULL){
		return;
	}

	if (param->type == AT_PARAM_TYPE_STRING) {
		AT_PARAMS_FREE(param->value.str_val);
	} else if (param->type == AT_PARAM_TYPE_NUM_INT) {
		param->value.int_val = 0;
	} else if (param->type == AT_PARAM_TYPE_NUM_SHORT) {
		param->value.short_val = 0;
	}
}


/* Internal function. Parameters cannot be null. */
static struct at_param *at_params_get(const struct at_param_list *const list,
				 size_t index)
{
	if (index >= list->param_count) {
		return NULL;
	}

	struct at_param *param = list->params;

	return &param[index];
}


/* Internal function. Parameter cannot be null. */
static size_t at_param_size(const struct at_param *const param)
{
	if (param->type == AT_PARAM_TYPE_NUM_SHORT) {
		return sizeof(u16_t);
	} else if (param->type == AT_PARAM_TYPE_NUM_INT) {
		return sizeof(u32_t);
	} else if (param->type == AT_PARAM_TYPE_STRING) {
		return strlen(param->value.str_val);
	}

	return 0;
}


int at_params_list_init(struct at_param_list *const list, size_t max_params_count)
{
	if (list == NULL) {
		return -EINVAL;
	}

	if (list->params != NULL) {
		return -EACCES;
	}

	list->params = AT_PARAMS_CALLOC(max_params_count,
				sizeof(struct at_param));
	if (list->params == NULL) {
		return -ENOMEM;
	}

	list->param_count = max_params_count;

	for (size_t param_idx = 0; param_idx < list->param_count;
			++param_idx) {
		struct at_param *params = list->params;

		at_param_init(&params[param_idx]);
	}

	return 0;
}


void at_params_list_clear(struct at_param_list *const list)
{
	if (list == NULL || list->params == NULL) {
		return;
	}

	for (size_t param_idx = 0; param_idx < list->param_count;
		 ++param_idx) {
		struct at_param *params = list->params;

		at_param_clear(&params[param_idx]);
		at_param_init(&params[param_idx]);
	}
}


void at_params_list_free(struct at_param_list *const list)
{
	if (list == NULL || list->params == NULL) {
		return;
	}

	at_params_list_clear(list);

	list->param_count = 0;
	AT_PARAMS_FREE(list->params);
	list->params = NULL;
}


int at_params_clear(struct at_param_list *const list, size_t index)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}


	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);
	at_param_init(param);
	return 0;
}


int at_params_put_short(const struct at_param_list *const list, size_t index,
			u16_t value)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);

	param->type = AT_PARAM_TYPE_NUM_SHORT;
	param->value.short_val = (value & USHRT_MAX);
	return 0;
}


int at_params_put_int(const struct at_param_list *const list, size_t index,
						u32_t value)
{
	if (list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	at_param_clear(param);

	param->type = AT_PARAM_TYPE_NUM_INT;
	param->value.int_val = value;
	return 0;
}


int at_params_put_string(const struct at_param_list *const list, size_t index,
			 const char *const str, size_t str_len)
{
	if (list == NULL || list->params == NULL || str == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	char *param_value = (char *)AT_PARAMS_MALLOC(str_len + 1);

	if (param_value == NULL) {
		return -ENOMEM;
	}

	memcpy(param_value, str, str_len);
	param_value[str_len] = '\0';

	at_param_clear(param);
	param->type = AT_PARAM_TYPE_STRING;
	param->value.str_val =	param_value;

	return 0;
}


int at_params_get_size(const struct at_param_list *const list, size_t index,
					size_t *const len)
{
	if (list == NULL || list->params == NULL || len == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	*len = at_param_size(param);
	return 0;
}


int at_params_get_short(const struct at_param_list *const list, size_t index,
			u16_t *const value)
{
	if (list == NULL || list->params == NULL || value == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_NUM_SHORT) {
		return -EINVAL;
	}

	*value = param->value.short_val;
	return 0;
}


int at_params_get_int(const struct at_param_list *const list, size_t index,
					u32_t *const value)
{
	if (list == NULL || list->params == NULL || value == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_NUM_INT) {
		return -EINVAL;
	}

	*value = param->value.int_val;
	return 0;
}


int at_params_get_string(const struct at_param_list *const list, size_t index,
			char *const value, size_t len)
{
	if (list == NULL || list->params == NULL || value == NULL) {
		return -EINVAL;
	}

	struct at_param *param = at_params_get(list, index);

	if (param == NULL) {
		return -EINVAL;
	}

	if (param->type != AT_PARAM_TYPE_STRING) {
		return -EINVAL;
	}

	size_t param_len = at_param_size(param);

	if (len < param_len) {
		return -ENOMEM;
	}

	memcpy(value, param->value.str_val, param_len);
	return param_len;
}


u32_t at_params_get_valid_count(const struct at_param_list *const list)
{
	if (list == NULL || list->params == NULL) {
		return 0;
	}

	size_t valid_param_idx = 0;
	struct at_param *param = at_params_get(list, valid_param_idx);

	while (param != NULL && param->type != AT_PARAM_TYPE_EMPTY) {
		valid_param_idx += 1;
		param = at_params_get(list, valid_param_idx);
	}

	return valid_param_idx;
}
