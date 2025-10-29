/**
 * @file sensor.c
 * @author sok
 * @brief 
 * @version 0.1
 * @date 2025-10-21
 * 
 * @copyright Copyright (c) 2025
 * @description 共计3种类型传感器，通过PCA9546时分复用，转3个通道twi采集传感器数据
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "sensor.h"

#define LOG_SENSOR_NAME TM_SENSOR
LOG_MODULE_REGISTER(LOG_SENSOR_NAME);

#define I2C_NODE        DT_NODELABEL(i2c0)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

#define SHT40_NODE      DT_NODELABEL(mysensor1)
#define BH1750_NODE     DT_NODELABEL(mysensor2)
#define MAX44009_NODE   DT_NODELABEL(mysensor3)
#define PCA9546_NODE    DT_NODELABEL(myswitch)
static const struct i2c_dt_spec dev_sht40    = I2C_DT_SPEC_GET(SHT40_NODE);
static const struct i2c_dt_spec dev_bh1750   = I2C_DT_SPEC_GET(BH1750_NODE);
static const struct i2c_dt_spec dev_max44009 = I2C_DT_SPEC_GET(MAX44009_NODE);
static const struct i2c_dt_spec dev_pca9546  = I2C_DT_SPEC_GET(PCA9546_NODE);

/**
 * @brief SHT40 T Accuracy = 0.2℃ all command have six respond
 */
static uint8_t sht40_data_buf[6];

/** @note 3个传感器和通道切换芯片的读写函数-------------------------------------- */
static void pca9546_channel_select(uint8_t chn){
  int ret;
  uint8_t chn0 = 0x04;
  uint8_t chn1 = 0x01;
  uint8_t chn2 = 0x02;    
  switch(chn)
  {
    case 0: //固定内部SHT40传感器
      ret = i2c_write_dt(&dev_pca9546, &chn0, 1);
      break;
    case 1:
      ret = i2c_write_dt(&dev_pca9546, &chn1, 1);
      break;
    case 2:
      ret = i2c_write_dt(&dev_pca9546, &chn2, 1);
      break;
    default:
      ret = i2c_write_dt(&dev_pca9546, 0x00, 1);
      break;
    break;
  }
}
static int sht40_write_Byte(uint8_t cmd){
  int ret;
  ret = i2c_write_dt(&dev_sht40, &cmd, 1);
  if(ret != 0)
  {
    LOG_WRN("IIC write SHT40 err reson is: 0x%c\n", ret);
  }
  return ret;
}
static int sht40_ReadNumBytes(uint8_t *DataBuf, uint8_t number){
  int ret;
	ret = i2c_read_dt(&dev_sht40, DataBuf, number);
  if(ret != 0)
  {
    LOG_WRN("IIC write SHT40 numBytes err reson is: 0x%c\n", ret);
  }  
	return ret;
}
static int  bh1750_write_Byte(uint8_t cmd){
  int ret;
  ret = i2c_write_dt(&dev_bh1750, &cmd, 1);
  if(ret != 0)
  {
    LOG_WRN("IIC write bh1750 err reson is: 0x%c\n", ret);
  }
  return ret;
}static int bh1750_WriteRead_NumBytes(uint8_t cmd, uint8_t *DataBuf, uint8_t number){
  int ret;
  ret = i2c_write_read_dt(&dev_bh1750,&cmd,1,DataBuf,number); 
  {
    LOG_WRN("IIC write bh1750 err reson is: 0x%c\n", ret);
  }
  return ret;
}
static int max44009_write_Bytes(uint8_t reg){
 int ret;
  ret = i2c_write_dt(&dev_max44009, &reg, 1);
  if(ret != 0)
  {
    LOG_WRN("IIC write MAX44009_NODE err reson is: 0x%c\n", ret);
  }
  return ret;
}
/** @note 3个传感器和通道切换芯片的读写函数-------------------------------------- */
uint8_t CheckChn_Sensor_is(uint8_t chn){
  uint8_t sensor_is;
  int ret_code;
  pca9546_channel_select(chn);
  k_msleep(1);
  ret_code = sht40_write_Byte(SHT40_HIGH_REPEAT);
  if(ret_code != 0)
  {
    LOG_ERR("channel %d is not sht40\n", chn);
    sensor_is = none;
  }
  else
  {
    sensor_is = sht40;
    return sensor_is;
  }
  ret_code = bh1750_write_Byte(BH1750_PDOWN);
  if(ret_code != 0)
  {
    LOG_ERR("channel %d is not bh1750\n", chn);
    sensor_is = none;
  }
  else
  {
    sensor_is = bh1750;
    return sensor_is;
  }
  ret_code = max44009_write_Bytes(MAX_INIT_ENABLE);
  if(ret_code != 0)
  {
    LOG_ERR("channel %d is not max44009\n", chn);
    sensor_is = none;
  }
  else
  {
    sensor_is = max44009;
    return sensor_is;
  }






  return sensor_is;
}
void ReadSensor_Data(uint8_t *databuf , sensor_TypeDef type){
  if(type == sht40)
  {
    k_msleep(10);
    sht40_ReadNumBytes(sht40_data_buf,6);
  }
}










