/**
 * @file ble_nus_order.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-18
 * 
 * @copyright Copyright (c) 2025
 * 
 * @see 命令解析成功，返回1:success
 *         ～解析失败，返回999:error～
 */
#include "ble_nus_order.h"
#include "user_rtc.h"
#include "user_storage.h"

uint8_t che_sync_time_order(const uint8_t *const rxbuf, uint16_t len)//同步时间
{
  if(len == 20)
  {
    if((rxbuf[0]==0x5b)&&(rxbuf[1]==0x01)&&(rxbuf[19]==0xbb))
    {
      /* year */
      timeInfo_stamp.year = (uint16_t)((rxbuf[2]<<8) | rxbuf[3]);
      /* month */
      if((1<=rxbuf[4])&&(rxbuf[4]<=12))
        timeInfo_stamp.month = rxbuf[4];
      /* day */
      if((1<=rxbuf[5])&&(rxbuf[5]<=31))
        timeInfo_stamp.day = rxbuf[5];
      /* hour */
      if((0<=rxbuf[6])&&(rxbuf[6]<=23))
        timeInfo_stamp.hour = rxbuf[6];   
       /* min */
      if((0<=rxbuf[7])&&(rxbuf[7]<=59))
        timeInfo_stamp.min = rxbuf[7];       
       /* sec */
      if((0<=rxbuf[8])&&(rxbuf[8]<=59))
        timeInfo_stamp.sec = rxbuf[8];   
      printk("set time is ok \n\r");
      refresh_flag.rtc_sta = true;
      reback_order_Status("set time ok", 11);
      return 1;
    }
  }
}
uint8_t che_SetAram_threshold_order(const uint8_t *const rxbuf, uint16_t len)//设置报警
{
  if(len == 18)
  {
    if((rxbuf[0]==0xbb)&&(rxbuf[1]==0xaa)&&(rxbuf[16]==0xdd)&&(rxbuf[17]==0xcc))
    {
      /*==set chn0 alarm===*/
      if((rxbuf[3] == 0x00)&&(rxbuf[2] == 0x01))
      {
        channel_0.h_temp = (int16_t)((rxbuf[4]<<8) | rxbuf[5]);
        channel_0.l_temp = (int16_t)((rxbuf[6]<<8) | rxbuf[7]);
        channel_0.h_hum  = (uint16_t)((rxbuf[8]<<8) | rxbuf[9]);
        channel_0.l_hum  = (uint16_t)((rxbuf[10]<<8) | rxbuf[11]);
        channel_0.h_lux  = (uint16_t)((rxbuf[12]<<8) | rxbuf[13]);
        channel_0.l_lux  = (uint16_t)((rxbuf[14]<<8) | rxbuf[15]);
        printk("set chn0 warm \n\r");
      }
      /*==set chn1 alarm===*/
      if((rxbuf[3] == 0x01)&&(rxbuf[2] == 0x01))
      {
        channel_1.h_temp = (int16_t)((rxbuf[4]<<8) | rxbuf[5]);
        channel_1.l_temp = (int16_t)((rxbuf[6]<<8) | rxbuf[7]);
        channel_1.h_hum  = (uint16_t)((rxbuf[8]<<8) | rxbuf[9]);
        channel_1.l_hum  = (uint16_t)((rxbuf[10]<<8) | rxbuf[11]);
        channel_1.h_lux  = (uint16_t)((rxbuf[12]<<8) | rxbuf[13]);
        channel_1.l_lux  = (uint16_t)((rxbuf[14]<<8) | rxbuf[15]);
        printk("set chn1 warm \n\r");
      }
      /*==set chn2 alarm===*/
      if((rxbuf[3] == 0x02)&&(rxbuf[2] == 0x01))
      {
        channel_2.h_temp = (int16_t)((rxbuf[4]<<8) | rxbuf[5]);
        channel_2.l_temp = (int16_t)((rxbuf[6]<<8) | rxbuf[7]);
        channel_2.h_hum  = (uint16_t)((rxbuf[8]<<8) | rxbuf[9]);
        channel_2.l_hum  = (uint16_t)((rxbuf[10]<<8) | rxbuf[11]);
        channel_2.h_lux  = (uint16_t)((rxbuf[12]<<8) | rxbuf[13]);
        channel_2.l_lux  = (uint16_t)((rxbuf[14]<<8) | rxbuf[15]);
        printk("set chn2 warm \n\r");      
      }
      reback_order_Status("set warn ok", 11);
    }
  }
}
uint8_t che_collect_data_order(const uint8_t *const rxbuf, uint16_t len)//设置采样间隔
{
  if(len == 6)
  {
    if((rxbuf[0]==0xbb)&&(rxbuf[1]==0xaa)&&(rxbuf[4]==0xdd)&&(rxbuf[5]==0xcc))
    {
      /* 设置采样间隔时间 */
      if(rxbuf[2] == 0x02)
      {
        yk_tm.samp_interval = rxbuf[3];
        printk("set collect interval is ok \n\r");
        reback_order_Status("set interval ok", 15);
        return 1;
      }
    }
  }
}
uint8_t che_open_storage_order(const uint8_t *const rxbuf, uint16_t len)//开存储
{
  if(len == 8)
  {
    if((rxbuf[0]==0xbb)&&(rxbuf[1]==0xaa)&&(rxbuf[6]==0xdd)&&(rxbuf[7]==0xcc))
    {
      /* 设置是否存储 */
      if(rxbuf[2] == 0x03)
      {
        yk_tm.storage_sta = true;
        refresh_flag.storage_sta = true;
        printk("start storage fun\n\r");
        reback_order_Status("open storage", 12);
        return 1;
      }  
    }
  }
}
uint8_t che_close_storage_order(const uint8_t *const rxbuf, uint16_t len)//关闭存储
{
  if(len == 8)
  {
    if((rxbuf[0]==0xbb)&&(rxbuf[1]==0xaa)&&(rxbuf[6]==0xdd)&&(rxbuf[7]==0xcc))
    {
      /* 设置中断停止存储功能 */
      if((rxbuf[2] == 0x04)&&(rxbuf[3] == 0x45)&&(rxbuf[4] == 0x4e)&&(rxbuf[5] == 0x44))
      {
        all_storage_close();
        yk_tm.storage_sta = false;
        refresh_flag.storage_sta = true;
        printk("stop storage fun\n\r");
        reback_order_Status("stop storage", 12);        
        return 1;
      }  
     }
  }
}
uint8_t che_get_storage_data_order(const uint8_t *const rxbuf, uint16_t len)//获取存储数据
{
  if(len == 13)
  {
    if((rxbuf[0]==0xbb)&&(rxbuf[1]==0xaa)&&(rxbuf[11]==0xdd)&&(rxbuf[12]==0xcc))
    {
      if(rxbuf[2] == 0x05)
      {
        if(memcmp(&rxbuf[3], "SAVEDATA", 8) == 0)
        {
          yk_tm.start_send_flag = false;    //停止实时读取功能
          yk_tm.storage_read_sta = true;    //开启蓝牙发送内部存储数据
          /* 
          * 停止存储功能，不再向flash写入数据 
          * 但是不着急开usb，等发送完存储数据后再开
          */
          //all_storage_close();
          yk_tm.storage_sta = false;
          refresh_flag.storage_sta = true;
          /* 刷新各个通道发送存储数据的旗标和变量 */
          channel_0.storage_read_idx = 0;
          channel_0.storage_read_ok = false;
          channel_1.storage_read_idx = 0;
          channel_1.storage_read_ok = false;
          channel_2.storage_read_idx = 0;
          channel_2.storage_read_ok = false;                   
          printk("start read storage send by ble \n\r");
          //reback_order_Status("savedata", 8);  
          return 1;
        }
      }
    }
  }
}
uint8_t che_systemoff_order(const uint8_t *const rxbuf, uint16_t len)//pwdn
{
  if(len == 4)
  {
    //if(memcmp(rxbuf, order_pwdn[0], 4) == 0)
    if((rxbuf[0]==0x50)&&(rxbuf[1]==0x57)&&(rxbuf[2]==0x44)&&(rxbuf[3]==0x4e))
    {
      printk("system device \n\r");
      return 1;
    }
  }
}
uint8_t che_DevStatus_order(const uint8_t *const rxbuf, uint16_t len)//获取设备状态
{
  if(len == 6)
  {
    //if(memcmp(rxbuf, order_status[0], 5) == 0)
    if((rxbuf[0]==0x53)&&(rxbuf[1]==0x54)&&(rxbuf[2]==0x41)&&(rxbuf[3]==0x54)&&(rxbuf[4]==0x55)&&(rxbuf[5]==0x53))
    {
      uint8_t reback_code[16];
      reback_code[0] = 0x55;
      reback_code[1] = 0xaa;
      reback_code[2] = (timeInfo_stamp.year >> 8) & 0xff;
      reback_code[3] = timeInfo_stamp.year & 0xff;
      reback_code[4] = timeInfo_stamp.month;
      reback_code[5] = timeInfo_stamp.day;
      reback_code[6] = timeInfo_stamp.hour;
      reback_code[7] = timeInfo_stamp.min;
      reback_code[8] = channel_0.storage_idx >> 8 & 0xff;
      reback_code[9] = channel_0.storage_idx & 0xff;
      reback_code[10] = channel_1.storage_idx >> 8 & 0xff;
      reback_code[11] = channel_1.storage_idx & 0xff;
      reback_code[12] = channel_2.storage_idx >> 8 & 0xff;
      reback_code[13] = channel_2.storage_idx & 0xff;
      reback_code[14] = 0x66;
      reback_code[15] = 0xbb;
      printk("get dev storage and data sign \n\r");
      yk_tm.start_send_flag = true;
      reback_order_Status(reback_code, 16);   
      return 1;
    }
  }
}
uint8_t che_clearStorage(const uint8_t *const rxbuf, uint16_t len)//清除存储指令
{
  if(len == 5)
  {
    //if(memcmp(rxbuf, order_pwdn[0], 4) == 0)
    if((rxbuf[0]==0x43)&&(rxbuf[1]==0x4C)&&(rxbuf[2]==0x45)&&(rxbuf[3]==0x41)&&(rxbuf[4]==0x52))
    {
      storage_clear_allFile();      
      reback_order_Status("clear over", 10);
    }
  }
}
















