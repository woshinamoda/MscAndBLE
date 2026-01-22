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

//user set period
#define RSSI_READ_PERIOD      500     //unit 10ms
#define BAT_LEV_PERIOD        500     //unit 10ms
#define CHECK_CHN_SENSOR      500     //unit 10ms


// ble adv name
#define DEVICE_NAME       "YK-TM"
#define DEVICE_NAME_LEN   (sizeof(DEVICE_NAME) - 1)
#define ADC_INTERVAL_MIN    800        //min time = 1000ms
#define ADC_INTERVAL_MAX    810        //max time = 1500ms

// define user gpio is
#define BAT_ADC                NRF_GPIO_PIN_MAP(0,2)
#define VCHECK                 NRF_GPIO_PIN_MAP(0,31)
#define CHG                    NRF_GPIO_PIN_MAP(0,27)
#define BUZZ                   NRF_GPIO_PIN_MAP(0,19)
#define FLASH_CS               NRF_GPIO_PIN_MAP(0,6)
#define FLASH_SCK              NRF_GPIO_PIN_MAP(0,8)
#define FLASH_MOSI             NRF_GPIO_PIN_MAP(0,5)
#define FLASH_MISO             NRF_GPIO_PIN_MAP(0,4)
#define LCD_CS                 NRF_GPIO_PIN_MAP(0,13)
#define LCD_WR                 NRF_GPIO_PIN_MAP(0,15)
#define LCD_DATA               NRF_GPIO_PIN_MAP(0,17)
#define PCA_SDA                NRF_GPIO_PIN_MAP(0,11)
#define PCA_SCL                NRF_GPIO_PIN_MAP(0,12)
#define PCA_RESET              NRF_GPIO_PIN_MAP(1,9)
#define CH1_EN                 NRF_GPIO_PIN_MAP(1,8)
#define CH2_EN                 NRF_GPIO_PIN_MAP(0,7)
// #define PCA_A0                 NRF_GPIO_PIN_MAP(0,8)
// #define PCA_A1                 NRF_GPIO_PIN_MAP(1,8)
// #define PCA_A2                 NRF_GPIO_PIN_MAP(1,9)
#define IRQ                    NRF_GPIO_PIN_MAP(0,22)
// #define COEX_GRANT             NRF_GPIO_PIN_MAP(1,5)
// #define COEX_REQ               NRF_GPIO_PIN_MAP(1,2)
// #define COEX_STATUS            NRF_GPIO_PIN_MAP(1,1)
#define QSPI_D3                NRF_GPIO_PIN_MAP(0,20)
#define QSPI_D2                NRF_GPIO_PIN_MAP(0,21)
#define QSPI_D1                NRF_GPIO_PIN_MAP(0,24)
#define QSPI_D0                NRF_GPIO_PIN_MAP(1,0)
#define QSPI_CS                NRF_GPIO_PIN_MAP(0,23)
#define QSPI_CLK               NRF_GPIO_PIN_MAP(0,25)
#define MODULE_EN              NRF_GPIO_PIN_MAP(1,4)
#define TXD_4G                 NRF_GPIO_PIN_MAP(1,7)
#define RXD_4G                 NRF_GPIO_PIN_MAP(1,6)
#define DTR_4G                 NRF_GPIO_PIN_MAP(1,5)
// #define IS_WIFI                NRF_GPIO_PIN_MAP(0,19)
// #define IS_FLASH               NRF_GPIO_PIN_MAP(0,17)
#define BUTTON                 NRF_GPIO_PIN_MAP(0,29)
#define BQ_CE                  NRF_GPIO_PIN_MAP(1,15)


#define GPIO_SET(pin,state)   ((state) ? nrf_gpio_pin_set(pin) : nrf_gpio_pin_clear(pin))           

typedef enum
{
  nosensor  = 0x00,   //悬空
  sht40     = 0x01,   //温湿度1
  bh1750    = 0x02,   //光照强度
  max44009  = 0x02    //光照强度
}sensor_TypeDef;

typedef struct
{
  bool      warm_icon_sta;
  bool      bt_sta;
  uint8_t   rssi;
  bool      storage_read_sta;   //是否开始读取数据（用于控制蓝牙发送）
  bool      storage_sta;        //是否开启存储（用于图标）
  bool      charging_sta;
  uint8_t   bat_level;          //5个等级,用于电量图标显示
  uint8_t   bat_precent;        //0 - 100%，用于电池电量服务更新
  uint8_t   samp_interval;      //采样间隔1-200min
  uint8_t   display_chn;        //当前显示第几个通道
  bool      start_send_flag;    //在收到status指令后，激活该旗标。断开蓝牙后重置该旗标
}yongker_TM_initTypedef;

typedef struct
{
  sensor_TypeDef    channel_type;   
  bool              temp_type_is_C;     //温度显示单位
  int16_t           temp_celsius;       //摄氏度-精确到小数点1位
  int16_t           temp_fahrenheit;    //华氏度-精确到小数点1位
  uint16_t          humidity;           //湿度
  uint16_t          klux;               //光照强度
  /* 每个通道都有对应报警阈值设置 */
  int16_t           h_temp;
  int16_t           l_temp;
  uint16_t          h_hum;
  uint16_t          l_hum;
  uint16_t          h_lux;
  uint16_t          l_lux;
  /* 通道对应存储了多少条数据，除了idx本身固有累计，其余在收到传送读取数据后重置 */
  bool              storage_over;       //存储超过20000条 
  uint16_t          storage_idx;        //0 - 19999条
  /* 读取(qspi flash数据)时专用 */
  bool              storage_read_ok;    //该通道读取完成  
  uint16_t          storage_read_idx;   //已经读取的数量
  /* ble发送(读取数据)专用 */
  uint8_t           sending_cnt;        //累计读15次，压入一包发送。用于计数发送累计次数
  bool              sending_sta;        //true:正在发送数据。 false：蓝牙数据还在FIFO中，或者fifo满了，还在堆积，没有发送，正在等待发送
  bool              sending_retry;      //冲发送旗标，默认false：本次/上一包数据发送成功   true：上一包发送失败，需要重发送
}yongker_tm_channelDef;
extern yongker_tm_channelDef   channel_0;
extern yongker_tm_channelDef   channel_1;
extern yongker_tm_channelDef   channel_2;
extern yongker_TM_initTypedef  yk_tm;

typedef struct 
{
  bool press_flag;
  bool short_flag;
  bool long_flag;
  uint16_t press_cnt;
}Button_InitTypeDef;
typedef struct 
{
  bool toogle_flag;
  uint8_t active_cnt;
}Vcheck_InitTypeDef;
/**
 * @brief 固定2.5秒读取一次rssi，并重置绘制信号强度变量
 * @note 只在蓝牙连接时执行
 */
void read_yonkerTM_BleRssi();
/**
 * @brief 串口指令处理函数
 * @see 不放到回调中执行，在工作队列中轮询到执行
 */
void yk_tm_order_cb();
/**
 * @brief 返回收到指令的命令
 * 
 * @param code 
 * @param len 
 */
void reback_order_Status(uint8_t *code, uint16_t len);
/**
 * @brief 判断数据是否等于发送间隔，然后定时发送
 * 
 * @param Tcompare 用于对比的时间间隔
 */
void send_yktm_Data(uint8_t Tcompare);
#endif