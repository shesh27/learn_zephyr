/* vl53l0x.c - Driver for ST VL53L0X time of flight sensor */

#define DT_DRV_COMPAT st_vl53l0x

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <init.h>
#include <drivers/gpio.h>
#include <sys/__assert.h>
#include <zephyr/types.h>
#include <device.h>
#include <logging/log.h>

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

LOG_MODULE_REGISTER(VL53L0X, CONFIG_SENSOR_LOG_LEVEL);

/* All the values used in this driver are coming from ST datasheet and examples.
 * It can be found here:
 *   http://www.st.com/en/embedded-software/stsw-img005.html
 * There are also examples of use in the L4 cube FW:
 *   http://www.st.com/en/embedded-software/stm32cubel4.html
 */
#define VL53L0X_REG_WHO_AM_I   0xC0
#define VL53L0X_CHIP_ID        0xEEAA
#define VL53L0X_SETUP_SIGNAL_LIMIT         (0.1*65536)
#define VL53L0X_SETUP_SIGMA_LIMIT          (60*65536)
#define VL53L0X_SETUP_MAX_TIME_FOR_RANGING     33000
#define VL53L0X_SETUP_PRE_RANGE_VCSEL_PERIOD   18
#define VL53L0X_SETUP_FINAL_RANGE_VCSEL_PERIOD 14

struct vl53l0x_data {
	const struct device *i2c;
	VL53L0X_Dev_t vl53l0x;
	VL53L0X_RangingMeasurementData_t RangingMeasurementData;
};

static int vl53l0x_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct vl53l0x_data *drv_data = dev->data;
	VL53L0X_Error ret;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL
			|| chan == SENSOR_CHAN_DISTANCE
			|| chan == SENSOR_CHAN_PROX);

	ret = VL53L0X_PerformSingleRangingMeasurement(&drv_data->vl53l0x,
					&drv_data->RangingMeasurementData);
	if (ret < 0) {
		LOG_ERR("Could not perform measurment (error=%d)", ret);
		return -EINVAL;
	}

	return 0;
}


static int vl53l0x_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct vl53l0x_data *drv_data = dev->data;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_DISTANCE
			|| chan == SENSOR_CHAN_PROX);

	if (chan == SENSOR_CHAN_PROX) {
		if (drv_data->RangingMeasurementData.RangeMilliMeter <=
		    CONFIG_VL53L0X_PROXIMITY_THRESHOLD) {
			val->val1 = 1;
		} else {
			val->val1 = 0;
		}
		val->val2 = 0;
	} else {
		val->val1 = drv_data->RangingMeasurementData.RangeMilliMeter / 1000;
		val->val2 = (drv_data->RangingMeasurementData.RangeMilliMeter % 1000) * 1000;
	}

	return 0;
}

static const struct sensor_driver_api vl53l0x_api_funcs = {
	.sample_fetch = vl53l0x_sample_fetch,
	.channel_get = vl53l0x_channel_get,
};

static int vl53l0x_setup_single_shot(const struct device *dev)
{
	struct vl53l0x_data *drv_data = dev->data;
	int ret;
	uint8_t VhvSettings;
	uint8_t PhaseCal;
	uint32_t refSpadCount;
	uint8_t isApertureSpads;

	ret = VL53L0X_StaticInit(&drv_data->vl53l0x);
	if (ret) {
		LOG_ERR("VL53L0X_StaticInit failed");
		goto exit;
	}

	ret = VL53L0X_PerformRefCalibration(&drv_data->vl53l0x,
					    &VhvSettings,
					    &PhaseCal);
	if (ret) {
		LOG_ERR("VL53L0X_PerformRefCalibration failed");
		goto exit;
	}

	ret = VL53L0X_PerformRefSpadManagement(&drv_data->vl53l0x,
					       (uint32_t *)&refSpadCount,
					       &isApertureSpads);
	if (ret) {
		LOG_ERR("VL53L0X_PerformRefSpadManagement failed");
		goto exit;
	}

	ret = VL53L0X_SetDeviceMode(&drv_data->vl53l0x,
				    VL53L0X_DEVICEMODE_SINGLE_RANGING);
	if (ret) {
		LOG_ERR("VL53L0X_SetDeviceMode failed");
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckEnable(&drv_data->vl53l0x,
					  VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					  1);
	if (ret) {
		LOG_ERR("VL53L0X_SetLimitCheckEnable sigma failed");
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckEnable(&drv_data->vl53l0x,
				VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
				1);
	if (ret) {
		LOG_ERR("VL53L0X_SetLimitCheckEnable signal rate failed");
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckValue(&drv_data->vl53l0x,
				VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,
				VL53L0X_SETUP_SIGNAL_LIMIT);

	if (ret) {
		LOG_ERR("VL53L0X_SetLimitCheckValue signal rate failed");
		goto exit;
	}

	ret = VL53L0X_SetLimitCheckValue(&drv_data->vl53l0x,
					 VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,
					 VL53L0X_SETUP_SIGMA_LIMIT);
	if (ret) {
		LOG_ERR("VL53L0X_SetLimitCheckValue sigma failed");
		goto exit;
	}

	ret = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&drv_data->vl53l0x,
					    VL53L0X_SETUP_MAX_TIME_FOR_RANGING);
	if (ret) {
		LOG_ERR(
		"VL53L0X_SetMeasurementTimingBudgetMicroSeconds failed");
		goto exit;
	}

	ret = VL53L0X_SetVcselPulsePeriod(&drv_data->vl53l0x,
					  VL53L0X_VCSEL_PERIOD_PRE_RANGE,
					  VL53L0X_SETUP_PRE_RANGE_VCSEL_PERIOD);
	if (ret) {
		LOG_ERR("VL53L0X_SetVcselPulsePeriod pre range failed");
		goto exit;
	}

	ret = VL53L0X_SetVcselPulsePeriod(&drv_data->vl53l0x,
					VL53L0X_VCSEL_PERIOD_FINAL_RANGE,
					VL53L0X_SETUP_FINAL_RANGE_VCSEL_PERIOD);
	if (ret) {
		LOG_ERR("VL53L0X_SetVcselPulsePeriod final range failed");
		goto exit;
	}

exit:
	return ret;
}


static int vl53l0x_init(const struct device *dev)
{
	struct vl53l0x_data *drv_data = dev->data;
	VL53L0X_Error ret;
	uint16_t vl53l0x_id = 0U;
	VL53L0X_DeviceInfo_t vl53l0x_dev_info;

	LOG_DBG("enter in %s", __func__);

#if DT_INST_NODE_HAS_PROP(0, xshut_gpios)
	const struct device *gpio;

	/* configure and set VL53L0X_XSHUT_Pin */
	gpio = device_get_binding(DT_INST_GPIO_LABEL(0, xshut_gpios));
	if (gpio == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
		DT_INST_GPIO_LABEL(0, xshut_gpios));
		return -EINVAL;
	}

	if (gpio_pin_configure(gpio,
			      DT_INST_GPIO_PIN(0, xshut_gpios),
			      GPIO_OUTPUT | GPIO_PULL_UP) < 0) {
		LOG_ERR("Could not configure GPIO %s %d).",
			DT_INST_GPIO_LABEL(0, xshut_gpios),
			DT_INST_GPIO_PIN(0, xshut_gpios));
		return -EINVAL;
	}

	gpio_pin_set(gpio, DT_INST_GPIO_PIN(0, xshut_gpios), 1);
	k_sleep(K_MSEC(100));
#endif

	drv_data->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (drv_data->i2c == NULL) {
		LOG_ERR("Could not get pointer to %s device.",
			DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	drv_data->vl53l0x.i2c = drv_data->i2c;
	drv_data->vl53l0x.I2cDevAddr = DT_INST_REG_ADDR(0);

	/* Get info from sensor */
	(void)memset(&vl53l0x_dev_info, 0, sizeof(VL53L0X_DeviceInfo_t));

	ret = VL53L0X_GetDeviceInfo(&drv_data->vl53l0x, &vl53l0x_dev_info);
	if (ret < 0) {
		LOG_ERR("Could not get info from device.");
		return -ENODEV;
	}

	LOG_DBG("VL53L0X_GetDeviceInfo = %d", ret);
	LOG_DBG("   Device Name : %s", vl53l0x_dev_info.Name);
	LOG_DBG("   Device Type : %s", vl53l0x_dev_info.Type);
	LOG_DBG("   Device ID : %s", vl53l0x_dev_info.ProductId);
	LOG_DBG("   ProductRevisionMajor : %d",
		    vl53l0x_dev_info.ProductRevisionMajor);
	LOG_DBG("   ProductRevisionMinor : %d",
		    vl53l0x_dev_info.ProductRevisionMinor);

	ret = VL53L0X_RdWord(&drv_data->vl53l0x,
			     VL53L0X_REG_WHO_AM_I,
			     (uint16_t *) &vl53l0x_id);
	if ((ret < 0) || (vl53l0x_id != VL53L0X_CHIP_ID)) {
		LOG_ERR("Issue on device identification");
		return -ENOTSUP;
	}

	/* sensor init */
	ret = VL53L0X_DataInit(&drv_data->vl53l0x);
	if (ret < 0) {
		LOG_ERR("VL53L0X_DataInit return error (%d)", ret);
		return -ENOTSUP;
	}

	ret = vl53l0x_setup_single_shot(dev);
	if (ret < 0) {
		return -ENOTSUP;
	}

	return 0;
}


static struct vl53l0x_data vl53l0x_driver;

DEVICE_DT_INST_DEFINE(0, vl53l0x_init, NULL, &vl53l0x_driver,
		    NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,
		    &vl53l0x_api_funcs);
