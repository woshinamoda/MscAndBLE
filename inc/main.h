/**
 * @file main.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-10-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef MAIN_H
#define MAIN_H

// zephyr 
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
//system
#include "stdint.h"
// Log 
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
// zephyr driver include
#include <zephyr/drivers/gpio.h>
// nordic hal driver include
#include <hal/nrf_gpio.h>
// nordic nrfx driver include
#include <nrfx_gpiote.h>

// ble adv name
#define DEVICE_NAME       "YK-TM"
#define DEVICE_NAME_LEN   (sizeof(DEVICE_NAME) - 1)
#define ADC_INTERVAL_MIN    800        //min time = 1000ms
#define ADC_INTERVAL_MAX    810        //max time = 1500ms

// define user gpio is
#define BAT_ADC                NRF_GPIO_PIN_MAP(0,2)
#define VCHECK                 NRF_GPIO_PIN_MAP(0,31)
#define CHG                    NRF_GPIO_PIN_MAP(0,29)
#define BUZZ                   NRF_GPIO_PIN_MAP(0,13)
#define FLASH_CS               NRF_GPIO_PIN_MAP(0,4)
#define FLASH_SCK              NRF_GPIO_PIN_MAP(0,5)
#define FLASH_MOSI             NRF_GPIO_PIN_MAP(0,6)
#define FLASH_MISO             NRF_GPIO_PIN_MAP(0,26)
#define LCD_CS                 NRF_GPIO_PIN_MAP(0,14)
#define LCD_WR                 NRF_GPIO_PIN_MAP(0,15)
#define LCD_DATA               NRF_GPIO_PIN_MAP(0,16)
#define PCA_SDA                NRF_GPIO_PIN_MAP(0,12)
#define PCA_SCL                NRF_GPIO_PIN_MAP(0,11)
#define PCA_RESET              NRF_GPIO_PIN_MAP(0,7)
#define PCA_A0                 NRF_GPIO_PIN_MAP(0,8)
#define PCA_A1                 NRF_GPIO_PIN_MAP(1,8)
#define PCA_A2                 NRF_GPIO_PIN_MAP(1,9)
#define IRQ                    NRF_GPIO_PIN_MAP(1,6)
#define COEX_GRANT             NRF_GPIO_PIN_MAP(1,5)
#define COEX_REQ               NRF_GPIO_PIN_MAP(1,2)
#define COEX_STATUS            NRF_GPIO_PIN_MAP(1,1)
#define QSPI_D3                NRF_GPIO_PIN_MAP(0,23)
#define QSPI_D2                NRF_GPIO_PIN_MAP(0,21)
#define QSPI_D1                NRF_GPIO_PIN_MAP(0,20)
#define QSPI_D0                NRF_GPIO_PIN_MAP(0,22)
#define QSPI_CS                NRF_GPIO_PIN_MAP(0,24)
#define QSPI_CLK               NRF_GPIO_PIN_MAP(1,0)
#define MODULE_EN              NRF_GPIO_PIN_MAP(1,7)
#define TXD_4G                 NRF_GPIO_PIN_MAP(1,4)
#define RXD_4G                 NRF_GPIO_PIN_MAP(1,3)
#define DTR_4G                 NRF_GPIO_PIN_MAP(0,25)
#define IS_WIFI                NRF_GPIO_PIN_MAP(0,19)
#define IS_FLASH               NRF_GPIO_PIN_MAP(0,17)

#define GPIO_SET(pin,state)   ((state) ? nrf_gpio_pin_set(pin) : nrf_gpio_pin_clear(pin))           

typedef struct
{
  bool      bt_sta;
  uint8_t   strength;
  bool      storage_sta;
  uint8_t   channel_num;
  uint8_t   channel_type;
  bool      charging_sta;
  uint8_t   bat_level;
  bool      temp_type;
  uint16_t  temp_celsius;       //精确到小数点1位
  uint16_t  temp_fahrenheit;    //精确到小数点1位
}yongker_TM_initTypedef;

extern yongker_TM_initTypedef yk_tm;




#endif