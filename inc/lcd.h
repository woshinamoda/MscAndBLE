/**
 * @file lcd.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-07
 * 
 * @copyright Copyright (c) 2025
 * @see 我们只考虑ht1621写的情况，是做
 */

#ifndef _LCD_H_
#define _LCD_H_

#include "main.h"
//HT1621控制命令
#define		SYSDIS 								0X00   //0B 100	0000	0000	X  关振荡器和LCD偏压发生器
#define 	SYSEN 								0X01   //0B 100	0000	0001	X  打开系统振荡器
#define 	LCDOFF 								0X02   //0B 100	0000	0010	X  关LCD偏压
#define 	LCDON 								0X03   //0B 100	0000	0011	X  开LCD偏压
#define 	TIMERDIS 							0X04   //0B 100	0000	0100	X  时基输出失效
#define 	WDTDIS								0X05   //0B 100	0000	0101	X  WDT溢出标志输出失效
#define 	TIMEREN 							0X06   //0B 100	0000	0110	X  时基输出使能
#define 	WDTEN 								0X07   //0B 100	0000	0111	X  WDT溢出标志输出有效
#define 	TONEOFF								0X08   //0B 100	0000	1000	X  关闭声音输出
#define 	TONEON 								0X09   //0B 100	0000	1001	X  打开声音输出
#define 	CLRTIMER 							0X0C   //0B 100	0000	1100	X  时基发生器清零
#define 	CLRWDT 								0X0E   //0B 100	0000	1110	X  清除WDT状态
#define 	XTAL_32K 							0X14   //0B 100	0001	0100	X  系统时钟源，晶振
#define 	RC256 								0X18   //0B 100	0001	1000	X  系统时钟源，片内RC
#define 	EXT256 								0X1C   //0B 100	0001	1100	X  系统时钟源，外部时钟源

#define 	BIAS1_2_2COM					0X20   //0B 100	0010	0000	X 	LCD 1/2偏压选项，2个公共口 
#define 	BIAS1_2_3COM					0X24   //0B 100	0010	0100	X 	LCD 1/2偏压选项，3个公共口
#define 	BIAS1_2_4COM					0X28   //0B 100	0010	1000	X 	LCD 1/2偏压选项，4个公共口

#define 	BIAS1_3_2COM					0X21   //0B 100	0010	0001	X 	LCD 1/3偏压选项，2个公共口 
#define 	BIAS1_3_3COM					0X25   //0B 100	0010	0101	X 	LCD 1/3偏压选项，3个公共口
#define 	BIAS1_3_4COM					0X29   //0B 100	0010	1001	X 	LCD 1/3偏压选项，4个公共口

#define 	TONE4K 								0X40   //0B 100	0100	0000	X  声音频率4K
#define 	TONE2K								0X50   //0B 100	0110	0000	X  声音频率2K
#define 	IRQ_DIS 							0X80   //0B 100	1000	0000	X  使/IRQ输出失效
#define 	IRQ_EN								0X88   //0B 100	1000	1000	X  使/IRQ输出有效
#define   BIAS                  0x52   //0b1000 0101 0010 1/3duty 4com

#define   SET_LCD_CS(x)               GPIO_SET(LCD_CS, x)
#define   SET_LCD_SCK(x)              GPIO_SET(LCD_WR, x)
#define   SET_LCD_DATA(x)             GPIO_SET(LCD_DATA, x)



typedef struct 
{
  bool ble_sta;
  bool adc_sta;
  bool ext_sta;
  bool rtc_sta;
  bool charging_sta;
}lcd_refresh_TypeDef;



/**
 * @brief 初始化开始显示
 * 
 */
void display_lcd_init();
/**
 * @brief ht1621对应地址，点亮com
 * 
 * @param addr 
 * @param com 
 */
void writeData_ht1621(uint8_t addr, uint8_t com);
/**
 * @brief 断码显示器初始化
 */
void lcd_ht1621_init();




































#endif






