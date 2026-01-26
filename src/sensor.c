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
#include "math.h"

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
static uint8_t sensor_ReadBuf[20];

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
    //LOG_WRN("IIC write SHT40 err reson is: 0x%c\n", ret);
  }
  return ret;
}
static int sht40_ReadNumBytes(uint8_t *DataBuf, uint8_t number){
  int ret;
	ret = i2c_read_dt(&dev_sht40, DataBuf, number);
  if(ret != 0)
  {
    //LOG_WRN("IIC write SHT40 numBytes err reson is: 0x%c\n", ret);
  }  
	return ret;
}
static int bh1750_write_Byte(uint8_t cmd){
  int ret;
  ret = i2c_write_dt(&dev_bh1750, &cmd, 1);
  if(ret != 0)
  {
    //LOG_WRN("IIC write bh1750 err reson is: 0x%c\n", ret);
  }
  return ret;
}
static int bh1750_WriteRead_NumBytes(uint8_t *DataBuf, uint8_t number){
  int ret;
  ret = i2c_read_dt(&dev_bh1750, DataBuf, number);
  {
    //LOG_WRN("IIC write bh1750 err reson is: 0x%c\n", ret);
  }
  return ret;
}
static int max44009_write_Bytes(uint8_t reg, uint8_t data){
 int ret;
 uint8_t codebuf[2];
 codebuf[0] = reg;
 codebuf[1] = data;
  ret = i2c_write_dt(&dev_max44009, codebuf, 2);
  if(ret != 0)
  {
    //LOG_WRN("IIC write MAX44009_NODE err reson is: 0x%c\n", ret);
  }
  return ret;
}
static int max44009_WriteRead_NumBytes(uint8_t *DataBuf, uint8_t number){
  int ret;
  uint8_t reg0 = 0x03;
  uint8_t reg1 = 0x04;  
 ret = i2c_write_read(i2c_dev, 0x4A, &reg0, 1, &DataBuf[0], 1); 
 ret = i2c_write_read(i2c_dev, 0x4A, &reg1, 1, &DataBuf[1], 1);   
  return ret;
}
/** @note 3个传感器和通道切换芯片的读写函数-------------------------------------- */
void judege_sensor_warming(yongker_tm_channelDef *chn)
{
  bool warm_temp = false;
  bool warm_hum = false;
  /** 没传感器就不报警 */
  if(chn->channel_type == nosensor)
    yk_tm.warm_icon_sta = false;
  if(chn->channel_type == sht40)
  {
    if((chn->humidity >= chn->h_hum)||(chn->humidity <= chn->l_hum))
    { warm_hum = true; }  //湿度超阈值范围
    if((chn->temp_celsius >= chn->h_temp)||(chn->temp_celsius <= chn->l_temp))
    { warm_temp = true; } //温度超阈值范围
    if((warm_temp)||(warm_hum))
      yk_tm.warm_icon_sta = true; //只要一个超出范围，直接报警
    else
      yk_tm.warm_icon_sta = false;
  }
  if(chn->channel_type == bh1750)
  {
    if((chn->klux >= chn->h_lux)||(chn->klux <= chn->l_lux))
      { yk_tm.warm_icon_sta = true;}
    else
      { yk_tm.warm_icon_sta = false;} 
  }
   if(chn->channel_type == max44009)
  {
    if((chn->klux >= chn->h_lux)||(chn->klux <= chn->l_lux))
      { yk_tm.warm_icon_sta = true;}
    else
      { yk_tm.warm_icon_sta = false;} 
  } 
}
void read_sensor_data(yongker_tm_channelDef *chn)
{
  uint32_t temp_raw, humid_raw, plux;
  if(chn->channel_type == nosensor)
  {
    //printk("without sensor, cannot read data\n\r");
  }
  if(chn->channel_type == sht40)
  {
    /*jb芯片，最少要等10ms转换*/
    k_msleep(15);
    sht40_ReadNumBytes(sensor_ReadBuf, 6);
    temp_raw = ((uint32_t)sensor_ReadBuf[0] << 8) | sensor_ReadBuf[1];
    humid_raw = ((uint32_t)sensor_ReadBuf[3] << 8) | sensor_ReadBuf[4];
    chn->temp_celsius    = (-45.0 + 175.0 * temp_raw / 65535.0)*10;
    chn->temp_fahrenheit = (-49.0 + 315.0 * temp_raw / 65535.0)*10;
    chn->humidity        = (-6.0 + 125.0 * humid_raw / 65535.0)*10;
    if(chn->humidity > 100)
      chn->humidity = 100;
    chn->klux = 0;
  }
  if(chn->channel_type == bh1750)
  {
    bh1750_write_Byte(BH1750_CONTINU_L_MODE);
    k_msleep(25);
    bh1750_WriteRead_NumBytes(sensor_ReadBuf, 2);
    plux = (sensor_ReadBuf[0]<<8 | sensor_ReadBuf[1]);
    chn->temp_celsius    = 0;
    chn->temp_fahrenheit = 0;
    chn->humidity        = 0;
    //原本应该除以1000，转klux，现在是lux。改为除10，这样保留2位小数
    //另外手册要求除以120，实测彭云确实除了0.46，最大可以卡在120klux    
    chn->klux = plux * 100 / 96 / 10;
    if(chn->klux >= 9999)
      chn->klux = 9999;    
  }
  if(chn->channel_type == max44009)
  {
    uint32_t E, M;
    k_msleep(50);
    max44009_WriteRead_NumBytes(sensor_ReadBuf, 2);
    E = sensor_ReadBuf[0] >>4;
    M = (((sensor_ReadBuf[0] & 0x0f) << 4)|(sensor_ReadBuf[1] & 0x0f));
    //和1750一样lux转klux，然后除1000改10，保留2位小数
    plux = (1 << E) * M * 45 / 1000 / 10;
    chn->temp_celsius    = 0;
    chn->temp_fahrenheit = 0;
    chn->humidity        = 0;
    chn->klux = plux;
    if(chn->klux >= 9999)
      chn->klux = 9999;
  }
  k_msleep(10);
}
uint8_t CheckChn_Sensor_is(uint8_t chn){
  uint8_t sensor_is;
  int ret_code;
  pca9546_channel_select(chn);
  ret_code = sht40_write_Byte(SHT40_HIGH_REPEAT);
  if(ret_code != 0)
  {
    //LOG_ERR("channel %d is not sht40\n", chn);
    sensor_is = nosensor;
  }
  else
  {
    sensor_is = sht40;
    return sensor_is;
  }
  ret_code = bh1750_write_Byte(BH1750_PON);
  if(ret_code != 0)
  {
    //LOG_ERR("channel %d is not bh1750\n", chn);
    sensor_is = nosensor;
  }
  else
  {
    sensor_is = bh1750;
    return sensor_is;
  }
  ret_code = max44009_write_Bytes(MAX_CONFIGURATION, 0x03);
  if(ret_code != 0)
  {
    //LOG_ERR("channel %d is not max44009\n", chn);
    sensor_is = nosensor;
  }
  else
  {
    sensor_is = max44009;
    return sensor_is;
  }
  return sensor_is;
}
void disable_iic_sensor()
{
	int rc;
	rc = pm_device_action_run(i2c_dev, PM_DEVICE_ACTION_SUSPEND);
}




















