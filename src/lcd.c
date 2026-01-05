#include "lcd.h"
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include "user_rtc.h"


#define LOG_MODULE_NAME HT1621
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
/**
 * @brief 刷新旗标类
 */
lcd_refresh_TypeDef   refresh_flag = {false};
/**
 * @brief 由于硬件没有连接ht1651的rx引脚，而且界面部分功能是乱的，所以必须用缓存添加，才能制定区域修改绘制。
 */
uint8_t seg_comBuf[32];

/*头部声明- start 基础绘图函数都包含于此----------------------------------*/
static void writeData_ht1621(uint8_t addr, uint8_t com);
static void ht1621_delay_us(uint32_t us);
static void LittleMode_SendBit_ht1621(uint8_t Data, uint8_t num);
static void writeCmd_ht1621(uint8_t cmd);
static void display_upNumber(bool is_err, int16_t num, uint8_t pot);
static void display_upNumber_klux(bool is_err, int16_t num, uint8_t pot);
static void display_downNumber(bool is_none, uint16_t num, uint8_t pot);
/*====================================================================*/

/*所有界面分割模块绘图，互不影响*/
void display_ble_sta()
{
  uint8_t s10_mask;
  s10_mask = seg_comBuf[27] &  0x08;
  if(yk_tm.bt_sta)
  {
    seg_comBuf[28] &=  0x07;    //关闭s3的x
    seg_comBuf[27] |=  0x06;    //点亮蓝牙和广播图标
    switch(yk_tm.rssi)
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
  seg_comBuf[27] = (seg_comBuf[27]|s10_mask);
  writeData_ht1621(27-1, seg_comBuf[27]);
  writeData_ht1621(28-1, seg_comBuf[28]);
}
void display_storage_sta()
{
  if(yk_tm.storage_sta)
  {
    SET_BIT(seg_comBuf[27],COM1);
    CLEAR_BIT(seg_comBuf[25], COM4);
  }
  else
  {
    CLEAR_BIT(seg_comBuf[27],COM1);
    SET_BIT(seg_comBuf[25], COM4);
  }
  writeData_ht1621(27-1, seg_comBuf[27]);
  writeData_ht1621(25-1, seg_comBuf[25]);  
}
void display_rtc_number(bool colSta)
{
  uint8_t hour_one, hour_ten, min_one, min_ten;
  uint8_t s11_mask, s12_mask, s13_mask;
  hour_ten = timeInfo_stamp.hour / 10 % 10;
  hour_one = timeInfo_stamp.hour / 1 % 10;
  min_ten = timeInfo_stamp.min / 10 % 10;
  min_one = timeInfo_stamp.min / 1 % 10;

  //数码管10 + s11保持
  s11_mask = seg_comBuf[25] &  0x01;
  s12_mask = seg_comBuf[26] &  0x08;
  if(hour_ten == 0)
  {
    seg_comBuf[25] = (0x00 | s11_mask);
    seg_comBuf[26] = 0x00;
  }
  else
  {
    seg_comBuf[25] = (num_wilcard_l[hour_ten] | s11_mask);
    seg_comBuf[26] = num_wilcard_h[hour_ten];
  }
  //数码管11 + 冒号col
  seg_comBuf[23] = num_wilcard_l[hour_one];
  seg_comBuf[24] = num_wilcard_h[hour_one];
  if(colSta)
    SET_BIT(seg_comBuf[23], COM4);
  else
    CLEAR_BIT(seg_comBuf[23], COM4);
  //数码管12 + 电池边框（必须有）
  seg_comBuf[21] = num_wilcard_l[min_ten];
  seg_comBuf[22] = num_wilcard_h[min_ten];
  SET_BIT(seg_comBuf[21], COM4);
  //数码管13 + s13保持
  s13_mask = seg_comBuf[19] &  0x01;
  seg_comBuf[19] = (num_wilcard_l[min_one] | s13_mask);
  seg_comBuf[20] = num_wilcard_h[min_one];
  seg_comBuf[26] = (seg_comBuf[26] | s12_mask);

  writeData_ht1621(19-1, seg_comBuf[19]); 
  writeData_ht1621(20-1, seg_comBuf[20]);  
  writeData_ht1621(21-1, seg_comBuf[21]); 
  writeData_ht1621(22-1, seg_comBuf[22]);     
  writeData_ht1621(23-1, seg_comBuf[23]); 
  writeData_ht1621(24-1, seg_comBuf[24]);     
  writeData_ht1621(25-1, seg_comBuf[25]); 
  writeData_ht1621(26-1, seg_comBuf[26]); 
}
void display_charge_icon()
{
  if(yk_tm.charging_sta)
    SET_BIT(seg_comBuf[19], COM4);
  else
    CLEAR_BIT(seg_comBuf[19], COM4);
  writeData_ht1621(19-1, seg_comBuf[19]); 
}
void display_waring_icon(bool warm_sta)
{
  if(warm_sta)
    SET_BIT(seg_comBuf[26], COM1);
  else
    CLEAR_BIT(seg_comBuf[26], COM1);
  writeData_ht1621(26-1, seg_comBuf[26]); 
}
void display_bat_level(uint8_t batLev)
{
  switch(batLev)
  {
    case 0: seg_comBuf[18] = 0x00;  break;
    case 1: seg_comBuf[18] = 0x08;  break;
    case 2: seg_comBuf[18] = 0x0C;  break;
    case 3: seg_comBuf[18] = 0x0E;  break;
    case 4: seg_comBuf[18] = 0x0F;  break;        
  }
  writeData_ht1621(18-1, seg_comBuf[18]); 
}
void display_channel_icon(uint8_t chn)
{
  if(chn == 0)
  {
    CLEAR_BIT(seg_comBuf[31], COM1);  //清除ext图标
    seg_comBuf[29] = num_wilcard_l[0];
    seg_comBuf[30] = num_wilcard_h[0];  
  }
  if(chn == 1)
  {
    SET_BIT(seg_comBuf[31], COM1);  //ext图标
    seg_comBuf[29] = num_wilcard_l[1];
    seg_comBuf[30] = num_wilcard_h[1];    
  }
  if(chn == 2)
  {
    SET_BIT(seg_comBuf[31], COM1);  //ext图标
    seg_comBuf[29] = num_wilcard_l[2];
    seg_comBuf[30] = num_wilcard_h[2];    
  }  
  writeData_ht1621(29-1, seg_comBuf[29]); 
  writeData_ht1621(30-1, seg_comBuf[30]); 
  writeData_ht1621(31-1, seg_comBuf[31]);       
}
void display_sensor_data(uint8_t chn)
{
  if(chn == 0)
  {
    //通道0只能是sht40
    if(channel_0.channel_type == sht40)
    {
      if(channel_0.temp_type_is_C)
      {
        seg_comBuf[17] = 0x05;//显示摄氏度，同时熄灭klux
        display_upNumber(false, channel_0.temp_celsius, 1);
        display_downNumber(false, channel_0.humidity, 1);
      }
      else
      {
        seg_comBuf[17] = 0x06;//显示华氏度，同时熄灭klux
        display_upNumber(false, channel_0.temp_fahrenheit, 1);
        display_downNumber(false, channel_0.humidity, 1);
      }
      /*默认点亮S2，熄灭S1*/
      SET_BIT(seg_comBuf[31], COM2);
      CLEAR_BIT(seg_comBuf[31], COM3);    
      /*显示%*/
      SET_BIT(seg_comBuf[8], COM4);      
    }
  }
  if(chn == 1)
  {
    if(channel_1.channel_type == nosensor)
    {
      seg_comBuf[8] = 0x00;
      seg_comBuf[17] = 0x00;
      seg_comBuf[31] = 0x00;
      display_upNumber(true, channel_1.channel_type, 1);
      display_downNumber(true, channel_1.channel_type, 1);
    }
    if(channel_1.channel_type == sht40)
    {
      if(channel_1.temp_type_is_C)
      {
        seg_comBuf[17] = 0x05;//显示摄氏度，同时熄灭klux
        display_upNumber(false, channel_1.temp_celsius, 1);
        display_downNumber(false, channel_1.humidity, 1);
      }
      else
      {
        seg_comBuf[17] = 0x06;//显示华氏度，同时熄灭klux
        display_upNumber(false, channel_1.temp_fahrenheit, 1);
        display_downNumber(false, channel_1.humidity, 1);
      }
      /*默认点亮S2，熄灭S1*/
      SET_BIT(seg_comBuf[31], COM2);
      CLEAR_BIT(seg_comBuf[31], COM3);    
      /*显示%*/
      SET_BIT(seg_comBuf[8], COM4);      
    }
    if((channel_1.channel_type == bh1750)||(channel_1.channel_type == max44009))
    {
      display_upNumber_klux(false, channel_1.klux, 2);
      display_downNumber(true, channel_1.channel_type, 1);    
      CLEAR_BIT(seg_comBuf[31], COM2);
      SET_BIT(seg_comBuf[31], COM3);    
      seg_comBuf[17] = 0x08;  //清掉温度单位，显示lux
    }
  }
  if(chn == 2)
  {
    if(channel_2.channel_type == nosensor)
    {
      seg_comBuf[8] = 0x00;
      seg_comBuf[17] = 0x00;
      seg_comBuf[31] = 0x00;
      display_upNumber(true, channel_2.channel_type, 1);
      display_downNumber(true, channel_2.channel_type, 1);
    }
    if(channel_2.channel_type == sht40)
    {
      if(channel_2.temp_type_is_C)
      {
        seg_comBuf[17] = 0x05;//显示摄氏度，同时熄灭klux
        display_upNumber(false, channel_2.temp_celsius, 1);
        display_downNumber(false, channel_2.humidity, 1);
      }
      else
      {
        seg_comBuf[17] = 0x06;//显示华氏度，同时熄灭klux
        display_upNumber(false, channel_2.temp_fahrenheit, 1);
        display_downNumber(false, channel_2.humidity, 1);
      }
      /*默认点亮S2，熄灭S1*/
      SET_BIT(seg_comBuf[31], COM2);
      CLEAR_BIT(seg_comBuf[31], COM3);    
      /*显示%*/
      SET_BIT(seg_comBuf[8], COM4);      
    }
    if((channel_2.channel_type == bh1750)||(channel_2.channel_type == max44009))
    {
      display_upNumber_klux(false, channel_2.klux, 2);
      display_downNumber(true, channel_2.channel_type, 1);    
      CLEAR_BIT(seg_comBuf[31], COM2);
      SET_BIT(seg_comBuf[31], COM3);    
      seg_comBuf[17] = 0x08;  //清掉温度单位，显示lux
    }
  }  
  writeData_ht1621(8-1, seg_comBuf[8]); 
  writeData_ht1621(17-1, seg_comBuf[17]); 
  writeData_ht1621(31-1, seg_comBuf[31]); 
}

/*所有绘图用的基础函数，前段头部声明*/
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
static void display_upNumber(bool is_err, int16_t num, uint8_t pot)
{
  bool is_negative = false;
  uint8_t one,ten,hundred,thousand;
  if(num < 0)
  {
    is_negative = true;
    num = abs(num);
  }
  one      = num/1%10;
  ten      = num/10%10;
  hundred  = num/100%10;
  thousand = num/1000%10;  
  if(is_err)
  {
    seg_comBuf[10] = 0x00;
    seg_comBuf[9]  = 0x00;
    seg_comBuf[12] = 0x08;
    seg_comBuf[11] = 0x0f;
    seg_comBuf[14] = 0x08;
    seg_comBuf[13] = 0x0a;
    seg_comBuf[15] = 0x0a;
    seg_comBuf[16] = 0x08;  
  }
  else
  {
    if(thousand != 0){
      seg_comBuf[16] = num_wilcard_l[one];
      seg_comBuf[15] = num_wilcard_h[one];
      seg_comBuf[14] = num_wilcard_l[ten];
      seg_comBuf[13] = num_wilcard_h[ten]; 
      seg_comBuf[12] = num_wilcard_l[hundred];
      seg_comBuf[11] = num_wilcard_h[hundred];  
      seg_comBuf[10] = num_wilcard_l[thousand];
      seg_comBuf[9]  = num_wilcard_h[thousand];        
    }
    else{
      seg_comBuf[10] = 0x00;
      seg_comBuf[9] = 0x00;
      if(hundred != 0)
      {
        seg_comBuf[16] = num_wilcard_l[one];
        seg_comBuf[15] = num_wilcard_h[one];
        seg_comBuf[14] = num_wilcard_l[ten];
        seg_comBuf[13] = num_wilcard_h[ten]; 
        seg_comBuf[12] = num_wilcard_l[hundred];
        seg_comBuf[11] = num_wilcard_h[hundred]; 
      }
      else{
        seg_comBuf[16] = num_wilcard_l[one];
        seg_comBuf[15] = num_wilcard_h[one];
        seg_comBuf[14] = num_wilcard_l[ten];
        seg_comBuf[13] = num_wilcard_h[ten];       
        seg_comBuf[12] = 0x00;
        seg_comBuf[11] = 0x00;
      }
    }
    if(pot == 1)
    {
      SET_BIT(seg_comBuf[14],COM4);
      CLEAR_BIT(seg_comBuf[12],COM4);
      CLEAR_BIT(seg_comBuf[10],COM4);
    }
    if(pot == 2)
    {
      CLEAR_BIT(seg_comBuf[14],COM4);
      SET_BIT(seg_comBuf[12],COM4);
      CLEAR_BIT(seg_comBuf[10],COM4);
    }
    if(pot == 3)
    {
      CLEAR_BIT(seg_comBuf[14],COM4);
      CLEAR_BIT(seg_comBuf[12],COM4);
      SET_BIT(seg_comBuf[10],COM4);
    }  
  }
  if(is_negative)
  {
    is_negative = false;
    seg_comBuf[10] = 0x00;
    seg_comBuf[9]  = 0x04;
  }
  for(uint8_t i=9; i<17; i++)
  {
   writeData_ht1621(i-1, seg_comBuf[i]); 
  }
}
static void display_upNumber_klux(bool is_err, int16_t num, uint8_t pot)
{
  bool is_negative = false;
  uint8_t one,ten,hundred,thousand;
  if(num < 0)
  {
    is_negative = true;
    num = abs(num);
  }
  one      = num/1%10;
  ten      = num/10%10;
  hundred  = num/100%10;
  thousand = num/1000%10;  
  if(is_err)
  {
    seg_comBuf[10] = 0x00;
    seg_comBuf[9]  = 0x00;
    seg_comBuf[12] = 0x08;
    seg_comBuf[11] = 0x0f;
    seg_comBuf[14] = 0x08;
    seg_comBuf[13] = 0x0a;
    seg_comBuf[15] = 0x0a;
    seg_comBuf[16] = 0x08;  
  }
  else
  {
    if(thousand != 0){
      seg_comBuf[16] = num_wilcard_l[one];
      seg_comBuf[15] = num_wilcard_h[one];
      seg_comBuf[14] = num_wilcard_l[ten];
      seg_comBuf[13] = num_wilcard_h[ten]; 
      seg_comBuf[12] = num_wilcard_l[hundred];
      seg_comBuf[11] = num_wilcard_h[hundred];  
      seg_comBuf[10] = num_wilcard_l[thousand];
      seg_comBuf[9]  = num_wilcard_h[thousand];        
    }
    else
    {
      seg_comBuf[10] = 0x00;
      seg_comBuf[9] = 0x00;
      seg_comBuf[16] = num_wilcard_l[one];
      seg_comBuf[15] = num_wilcard_h[one];
      seg_comBuf[14] = num_wilcard_l[ten];
      seg_comBuf[13] = num_wilcard_h[ten];
      seg_comBuf[12] = num_wilcard_l[hundred];
      seg_comBuf[11] = num_wilcard_h[hundred];           
    }
    if(pot == 1)
    {
      SET_BIT(seg_comBuf[14],COM4);
      CLEAR_BIT(seg_comBuf[12],COM4);
      CLEAR_BIT(seg_comBuf[10],COM4);
    }
    if(pot == 2)
    {
      CLEAR_BIT(seg_comBuf[14],COM4);
      SET_BIT(seg_comBuf[12],COM4);
      CLEAR_BIT(seg_comBuf[10],COM4);
    }
    if(pot == 3)
    {
      CLEAR_BIT(seg_comBuf[14],COM4);
      CLEAR_BIT(seg_comBuf[12],COM4);
      SET_BIT(seg_comBuf[10],COM4);
    }  
  }
  if(is_negative)
  {
    is_negative = false;
    seg_comBuf[10] = 0x00;
    seg_comBuf[9]  = 0x04;
  }
  for(uint8_t i=9; i<17; i++)
  {
   writeData_ht1621(i-1, seg_comBuf[i]); 
  }
}
static void display_downNumber(bool is_none, uint16_t num, uint8_t pot)
{
  uint8_t one,ten,hundred,thousand;
  one      = num/1%10;
  ten      = num/10%10;
  hundred  = num/100%10;
  thousand = num/1000%10;  
  if(is_none)
  {
    seg_comBuf[8] = 0x00;
    seg_comBuf[7] = 0x00;
    seg_comBuf[6] = 0x00;
    seg_comBuf[5] = 0x00; 
    seg_comBuf[4] = 0x00;
    seg_comBuf[3] = 0x00;
    seg_comBuf[2] = 0x00;
    seg_comBuf[1] = 0x00;
  }
  else
  {
    if(thousand != 0){
      seg_comBuf[8] = num_wilcard_l[one];
      seg_comBuf[7] = num_wilcard_h[one];
      seg_comBuf[6] = num_wilcard_l[ten];
      seg_comBuf[5] = num_wilcard_h[ten]; 
      seg_comBuf[4] = num_wilcard_l[hundred];
      seg_comBuf[3] = num_wilcard_h[hundred];  
      seg_comBuf[2] = num_wilcard_l[thousand];
      seg_comBuf[1]  = num_wilcard_h[thousand];        
    }
    else{
      seg_comBuf[2] = 0x00;
      seg_comBuf[1] = 0x00;
      if(hundred != 0)
      {
        seg_comBuf[8] = num_wilcard_l[one];
        seg_comBuf[7] = num_wilcard_h[one];
        seg_comBuf[6] = num_wilcard_l[ten];
        seg_comBuf[5] = num_wilcard_h[ten]; 
        seg_comBuf[4] = num_wilcard_l[hundred];
        seg_comBuf[3] = num_wilcard_h[hundred];  
      }
      else{
        seg_comBuf[8] = num_wilcard_l[one];
        seg_comBuf[7] = num_wilcard_h[one];
        seg_comBuf[6] = num_wilcard_l[ten];
        seg_comBuf[5] = num_wilcard_h[ten];       
        seg_comBuf[4] = 0x00;
        seg_comBuf[3] = 0x00;
      }
    }
    if(pot == 1)
    {
      SET_BIT(seg_comBuf[6],COM4);
      CLEAR_BIT(seg_comBuf[4],COM4);
      CLEAR_BIT(seg_comBuf[2],COM4);
    }
    if(pot == 2)
    {
      CLEAR_BIT(seg_comBuf[6],COM4);
      SET_BIT(seg_comBuf[4],COM4);
      CLEAR_BIT(seg_comBuf[2],COM4);
    }
    if(pot == 3)
    {
      CLEAR_BIT(seg_comBuf[6],COM4);
      CLEAR_BIT(seg_comBuf[4],COM4);
      SET_BIT(seg_comBuf[2],COM4);
    }  
  }
  for(uint8_t i=1; i<9; i++)
  {
   writeData_ht1621(i-1, seg_comBuf[i]); 
  }
}
static void writeData_ht1621(uint8_t addr, uint8_t com)
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

/** @brief 引用置外部的函数*********************/
void display_lcd_init()
{
  display_ble_sta();
  display_storage_sta();
  display_rtc_number(true);
  display_charge_icon();
  display_waring_icon(false);
  display_bat_level(1);
  display_channel_icon(0);
  display_sensor_data(0);
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















































