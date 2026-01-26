#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "main.h"

/**
 * @brief 按键中断初始化
 */
void button_gpiote_init();
/**
 * @brief 定时器中处理按键长短按函数
 * 
 */
void button_EvenTimer_handle();
/**
 * @brief 按键短按事件执行功能
 */
void button_short_cb();
/**
 * @brief 按键长按事件执行功能
 */
void button_long_cb();
/**
 * @brief 充电引脚gpiote初始化
 */
void vcheck_gpiote_init();
/**
 * @brief vheck中断事件判断
 */
void vcheck_EvenTimer_handle();


void button_more_long_cb();

















#endif









