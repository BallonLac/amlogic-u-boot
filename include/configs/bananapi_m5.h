/*
 * include/configs/bananapi_m5.h
 *
 * (C) Copyright 2018 Hardkernel Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __BANANAPI_M5_H__
#define __BANANAPI_M5_H__

#define CONFIG_DEVICE_PRODUCT		"bananapi_m5"
#define BANANAPI_BOARD_UUID		"9098004a-a1dd-11e8-98d0-529269fb1459"

/* configs for CEC */
#define CONFIG_CEC_OSD_NAME		"BANANAPI-M5"
#define CONFIG_CEC_WAKEUP

#include "bananapi-g12-common.h"

#if defined(CONFIG_CMD_USB)
	 /* USB Host Hub Reset */
        #define CONFIG_USB_HUB_RST_N            GPIOEE(GPIOH_4)
        #define CONFIG_USB_HUB_RST_N_NAME       "GPIOH_4"
        /* USB Host Hub Enable */
        #define CONFIG_USB_HUB_CHIP_EN          GPIOEE(GPIOH_6)
        #define CONFIG_USB_HUB_CHIP_EN_NAME     "GPIOH_6"

	/* USB OTG Power Enable */
	#define CONFIG_USB_GPIO_PWR		-1
	#define CONFIG_USB_GPIO_PWR_NAME	""
#endif

#undef  CONFIG_VDDEE_INIT_VOLTAGE
#define CONFIG_VDDEE_INIT_VOLTAGE		880	/* VDDEE power up voltage */

/* only one i2c master initial?? */
/* i2c_2 GPIO_X10/X11  */
//#define CONFIG_SYS_I2C_AML_I2C1

/* i2c_2 GPIO_X17/X18  */
#define CONFIG_SYS_I2C_AML_I2C2

/* i2c_3 GPIO_A14/A15 */
//#define CONFIG_SYS_I2C_AML_I2C3

#endif