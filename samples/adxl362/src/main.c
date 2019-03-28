/*
 * Copyright (c) 2018 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct sensor_value accel[3];

	struct device *adxl362_dev = device_get_binding(DT_ADI_ADXL362_0_LABEL);
	if (adxl362_dev == NULL) {
		printf("Could not get %s device\n", DT_ADI_ADXL362_0_LABEL);
		return;
	}

	struct device *adxl372_dev = device_get_binding(DT_ADI_ADXL372_0_LABEL);
	if (adxl372_dev == NULL) {
		printf("Could not get %s device\n", DT_ADI_ADXL372_0_LABEL);
		return;
	}

	printk("Get data\n");

	while (1) {
		sensor_sample_fetch(adxl362_dev);

		sensor_channel_get(adxl362_dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
		sensor_channel_get(adxl362_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
		sensor_channel_get(adxl362_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);

		printf("ADXL362: X %f - Y %f - Z %f\n",
			sensor_value_to_double(&accel[0]),
			sensor_value_to_double(&accel[1]),
			sensor_value_to_double(&accel[2]));

		sensor_sample_fetch(adxl372_dev);

		sensor_channel_get(adxl372_dev, SENSOR_CHAN_ACCEL_X, &accel[0]);
		sensor_channel_get(adxl372_dev, SENSOR_CHAN_ACCEL_Y, &accel[1]);
		sensor_channel_get(adxl372_dev, SENSOR_CHAN_ACCEL_Z, &accel[2]);

		printf("ADXL372: X %f - Y %f - Z %f\n\n",
			sensor_value_to_double(&accel[0]),
			sensor_value_to_double(&accel[1]),
			sensor_value_to_double(&accel[2]));
		k_sleep(2000);
	}
}
