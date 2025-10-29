/**
 * @file user_rtc.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-10-27
 * 
 * @copyright Copyright (c) 2025
 * @notice NCS基于mspl，无法继续调用nrf clock头文件，后面秒数时间戳由用户自定义，起始时间20205-01-01-00-01-01位sec_allCnt = 0
 * @notice 年份从2000年开始
 * @note ： ！NCS在mspl下使用RTC总是会报错，该设备考虑不关机，直接用APP硬件定时器代替RTC
 */
#include "user_rtc.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
// #include <nrfx_rtc.h>
// #include <nrfx_clock.h>
// static const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(1);


uint32_t					myTimeStamp;				//以秒为单位，总共计时


timeinfo_TypeDef	timeInfo_stamp ={			//阳历(公历)形式时间戳
	.year  = 2025, //start by 2000 max to 2088 for all sec cnt
	.month = 10,
	.day	 = 25,
	.hour  = 14,
	.min   = 00,
	.sec   = 00,
};				
/**
 * @brief 判断是否闰年
 * 
 * @param year 
 * @return true 
 * @return false 
 */
static bool is_leap_year(uint16_t year)
{
	return (year % 4 ==0 && year % 100 !=0) || (year % 400 == 0);
}

static uint8_t month_day_is(uint16_t year, uint8_t month)
{
	if(month == 2 && is_leap_year(year)){
		return 29;
	}
	if(month >= 1 && month <= 12){
		return days_is_month[month - 1];
	}
	return 0;
}
/**
 * @brief 从起始时间2000-01-01-00-00-00开始，用累计秒数统计年月日时分秒
 * 
 * @param secall 累计总秒数
 * @param tm 年月日时分秒clock结构体指针。
 */
static void secAll_to_timeInfo(uint32_t secall, timeinfo_TypeDef *tm)
{
	uint32_t days = secall / 86400;
	uint32_t remain_sec = secall % 86400;
	/* 反推时分秒 */
	tm->hour = remain_sec / 3600;
	tm->min = (remain_sec % 3600) / 60;
	tm->sec = remain_sec % 60; //直接取最小单位即可，不用在除多余的。

	/* 反推年 */
	tm->year = 2000;
	while(days >= 365){
		uint16_t days_in_year = is_leap_year(tm->year) ? 366 : 365;
		if(days < days){ //剩余日期不满一年
			break;
		}
		days = days - days_in_year;
		tm->year++;
	}
	
	/* 反推月日 */
	tm->month = 1;
	for(int i = 0; i <12; i++){
		uint8_t days_in_current_month = month_day_is(tm->year,tm->month);
		if(days < days_in_current_month){ //剩余日期不满对应平/润年当月日期
			break;
		}
		days = days - days_in_current_month;
		tm->month++;
	
		//1-12量程限制
		if(tm->min >12){
			tm->min = 12;
			days = month_day_is(tm->year, 12) - 1;
			break;
		}
	}
	tm->day = days + 1; //天数从0开始，日期从1开始+1
}
/**
 * @brief 从当前的年月日，推算时间总描述
 * 
 * @param tm tm->year 必须大于2000
 * @return uint32_t 
 */
static uint32_t timeInfo_to_secconds(timeinfo_TypeDef *tm)
{
	uint32_t total_secconds = 0;
	//计算从2000年 - 当前年数的总天数
	for(uint16_t year = 2000; year < tm->year; year++)
	{
		total_secconds += is_leap_year(year) ? 31622400 : 31536000; //闰年比平年多了86400sec
	}
	//计算当年已过多少月，转换对应月数的天数
	for(uint8_t month = 1; month < tm->month; month++)
	{
		total_secconds += month_day_is(tm->year, month) * 86400;
	}
	//当月已过天数
	total_secconds += (tm->day - 1) * 86400;
	//当前时分秒
	total_secconds += tm->hour * 3600 + tm->min * 60 + tm->sec;
	//返回累计总秒数
	return total_secconds;
}
static void timer0_RTC_handle(struct k_timer *dummy)		//timer tick = 10ms
{
	myTimeStamp++;
}
static K_TIMER_DEFINE(timer0, timer0_RTC_handle, NULL);
void printf_user_NowTime()
{
	secAll_to_timeInfo(myTimeStamp, &timeInfo_stamp);
	printk("%04d-%02d-%02d %02d:%02d:%02d\n", 
	timeInfo_stamp.year , timeInfo_stamp.month , timeInfo_stamp.day,
	timeInfo_stamp.hour , timeInfo_stamp.min,    timeInfo_stamp.sec);
}
void my_rtc_init()
{
	nrfx_err_t err_code;

	//初始化时间累积	
	myTimeStamp = timeInfo_to_secconds(&timeInfo_stamp);

	k_timer_start(&timer0, K_MSEC(1), K_MSEC(1000));
}




















