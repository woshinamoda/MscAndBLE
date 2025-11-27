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
timeinfo_TypeDef	timeInfo_stamp ={			//阳历(公历)形式时间戳
	.year  = 2025, //start by 2000 max to 2088 for all sec cnt
	.month = 10,
	.day	 = 25,
	.hour  = 23,
	.min   = 59,
	.sec   = 00,
};
static uint8_t interval_compare = 0;

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
 */
static void secAll_to_timeInfo(timeinfo_TypeDef *tm)
{
	tm->sec++;
	if(tm->sec % 3 == 0){
		interval_compare++;	//测试用的，每过3sec发一次，存一次数据
		send_yktm_Data(interval_compare);
	}	
	if(tm->sec >= 60)
	{
		tm->sec = 0;
		tm->min++;
		// interval_compare++;	//每过1min，用于对比时间间隔的变量也自增
		// send_yktm_Data(interval_compare);
		/*min -> hour */
		if(tm->min >= 60)
		{
			tm->min = 0;
			tm->hour++;
			/*hour -> day */
			if(tm->hour >= 24)
			{
				tm->hour = 0;
				tm->day++;
				/*获取当月天数*/
				uint8_t days_in_month = month_day_is(tm->year, tm->month);
				/*day -> month*/
				if(tm->day > days_in_month)
				{
					tm->day = 1;
					tm->month++;
					/*month -> year*/
					if(tm->month > 12)
					{
						tm->month = 1;
						tm->year++;
					}
				}
			}
		}
	}
}
static uint8_t last_minth;
static void timer0_RTC_handle(struct k_timer *dummy)		//timer tick = 10ms
{
	last_minth = timeInfo_stamp.min;
	secAll_to_timeInfo(&timeInfo_stamp);	
	if(timeInfo_stamp.min!=last_minth)
	{
		last_minth = timeInfo_stamp.min;
		refresh_flag.rtc_sta = true;
	}
}
static K_TIMER_DEFINE(timer0, timer0_RTC_handle, NULL);
void my_rtc_init()
{
	k_timer_start(&timer0, K_MSEC(1), K_MSEC(1000));
}




















