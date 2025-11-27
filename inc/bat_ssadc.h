#ifndef _BAT_SSADC_H_
#define _BAT_SSADC_H_

#include "main.h"
#include <nrfx_saadc.h>


/**
 * @brief 初始化内部adc，并且开始采集一次初始电池电量
 * 
 */
void ssadc_init();
/**
 * @brief 定时器周期读取电池电量
 */
void bat_Systime_handle();




#endif