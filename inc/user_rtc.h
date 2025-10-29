#ifndef _USER_RTC_H_
#define _USER_RTC_H_

#include "main.h"
/* 12个月对应天数 */
static const uint8_t days_is_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/**
 * @brief 设备的时间戳
 * 
 */
typedef struct{
  uint16_t  year;
  uint8_t   month;
  uint8_t   day;
  uint8_t   hour;
  uint8_t   min;
  uint8_t   sec;
}timeinfo_TypeDef;
/**
 * @brief 打印当前时间结构体
 */
void printf_user_NowTime();
/**
 * @brief 初始化RTC计数器
 */
void my_rtc_init();














#endif