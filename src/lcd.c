#include "lcd.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#define LOG_MODULE_NAME HT1621
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
/**
 * @brief 由于硬件没有连接ht1651的rx引脚，而且界面部分功能是乱的，所以必须用缓存添加，才能制定区域修改绘制。
 */
uint8_t seg_comBuf[32];
lcd_refresh_TypeDef   refresh_flag;


//头部声明
/*-------------------------------------------------------------------*/
static void ht1621_delay_us(uint32_t us);
static void LittleMode_SendBit_ht1621(uint8_t Data, uint8_t num);
static void writeCmd_ht1621(uint8_t cmd);
/*-------------------------------------------------------------------*/

void display_ble_sta()
{
  if(yk_tm.bt_sta)
  {
    seg_comBuf[28] &=  0x07;    //关闭s3的x
    seg_comBuf[27] |=  0x06;    //点亮蓝牙和广播图标
    switch(yk_tm.strength)
    {
      case 0:
        seg_comBuf[27] &= 0x0E;
        seg_comBuf[28]  = 0x00;
        break;
      case 1:
        seg_comBuf[27] &= 0x0E;
        seg_comBuf[28]  = 0x04;  
        break;
      case 2:
        seg_comBuf[27] &= 0x0E;
        seg_comBuf[28]  = 0x06;  
        break;      
      case 3:
        seg_comBuf[27] &= 0x0E;
        seg_comBuf[28]  = 0x07;  
        break;      
      case 4:
        seg_comBuf[27] |= 0x01;
        seg_comBuf[28]  = 0x07;  
        break;   
    }
  }
  else
  {
    seg_comBuf[27]  = 0x02;  
    seg_comBuf[28]  = 0x08;  
  }
  writeData_ht1621(27-1, seg_comBuf[27]);
  writeData_ht1621(28-1, seg_comBuf[28]);
}


static void ht1621_delay_us(uint32_t us)
{
  k_busy_wait(us);
}
static void LittleMode_SendBit_ht1621(uint8_t Data, uint8_t num)
{
  uint8_t i;
  uint8_t aData = Data;
  for(i=0;i<num;i++)
  {
    ht1621_delay_us(1);
    SET_LCD_SCK(0);
    ht1621_delay_us(1);
    if(((aData&0x80)>>7)!=0)
      SET_LCD_DATA(1);
    else
      SET_LCD_DATA(0);
    ht1621_delay_us(1);
    SET_LCD_SCK(1);
    ht1621_delay_us(1);
    aData<<=1;  
  }
}
static void writeCmd_ht1621(uint8_t cmd)
{
  ht1621_delay_us(1);
  SET_LCD_CS(0);
  ht1621_delay_us(1);
  LittleMode_SendBit_ht1621(0x80,3);
  ht1621_delay_us(1);
  LittleMode_SendBit_ht1621(cmd,8);
  ht1621_delay_us(1);
  LittleMode_SendBit_ht1621(0,1);
  SET_LCD_CS(1);
}
void writeData_ht1621(uint8_t addr, uint8_t com)
{
  uint8_t i;
  k_usleep(0.1);
  SET_LCD_CS(0);
  k_usleep(0.1);
  //write symbol 101
  LittleMode_SendBit_ht1621(0xa0,3);  
  for(i=2; i<8; i++)
  {
    SET_LCD_SCK(0);
    k_usleep(0.1);
    if((addr<<i)&0x80)
      SET_LCD_DATA(1);
    else
      SET_LCD_DATA(0);
    k_usleep(0.1);
    SET_LCD_SCK(1);
    k_usleep(0.1);
  }
  for(i=0; i<4; i++)
  {
    SET_LCD_SCK(0);
    k_usleep(0.1);
    if((com>>(3-i))&0x01)
      SET_LCD_DATA(1);
    else
      SET_LCD_DATA(0);
    k_usleep(0.1);
    SET_LCD_SCK(1);
    k_usleep(0.1);
  }
  SET_LCD_CS(1);
  k_usleep(0.1);
}
void display_lcd_init()
{
  yk_tm.bt_sta = true;
  yk_tm.strength = 3;
  display_ble_sta();
  k_msleep(1000);
  yk_tm.bt_sta = false;
  yk_tm.strength = 3;
  display_ble_sta();
  k_msleep(1000);
  yk_tm.bt_sta = true;
  yk_tm.strength = 4;
  display_ble_sta();
  k_msleep(1000);
  yk_tm.bt_sta = false;
  yk_tm.strength = 3;
  display_ble_sta();
  k_msleep(1000);
}
void lcd_ht1621_init()
{
  writeCmd_ht1621(WDTDIS);    //禁止看门狗暂停标志输出
  writeCmd_ht1621(RC256);     //系统时钟，片内RC震荡
  writeCmd_ht1621(BIAS1_3_4COM);
  writeCmd_ht1621(SYSEN);
  writeCmd_ht1621(LCDON);
  writeCmd_ht1621(TIMEREN);      //允许time base输出
  for(int i=0x00;i<32;i++)
  {
    writeData_ht1621(i,0x00);
  }
}



































