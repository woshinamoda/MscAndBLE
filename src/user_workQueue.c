/**
 * @file user_workQueue.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-13
 * 
 * @copyright Copyright (c) 2025
 * 所有设备工作队列合集
 */
#include "user_workQueue.h"
#include "user_rtc.h"
#include "lcd.h"
#include "bat_ssadc.h"
#include "button.h"
/**
 * @brief 更新lcd刷屏的功能函数
 */
static void lcd_refresh_function()
{
  if(refresh_flag.ble_sta){
    refresh_flag.ble_sta = false;
    display_ble_sta();
  }
  if(refresh_flag.rtc_sta){
    refresh_flag.rtc_sta = false;
    display_rtc_number(true);
  }
  if(refresh_flag.adc_sta){
    refresh_flag.adc_sta = false;
    display_bat_level(yk_tm.bat_level);
  }
  if(refresh_flag.storage_sta){
    refresh_flag.storage_sta = false;
    display_storage_sta();
  }
  if(refresh_flag.channel_dis_sta){
    refresh_flag.channel_dis_sta = false;
    display_channel_icon(yk_tm.display_chn);
    display_sensor_data(yk_tm.display_chn);
  }
  if(refresh_flag.channel_data_sta){
    refresh_flag.channel_data_sta = false;
    display_sensor_data(yk_tm.display_chn);
  }
  if(refresh_flag.channel_warming_sta){
    refresh_flag.channel_warming_sta = false;
    display_waring_icon(yk_tm.warm_icon_sta);
  }

}
/**
 * @brief 系统定时器工作队列的
 */
static void systimer_function()
{
  read_yonkerTM_BleRssi();
  bat_Systime_handle();
  yk_tm_order_cb();
  button_EvenTimer_handle();

}
/**************************************************************************
 * @brief refresh work queue 
 * @see   app定时器10ms周期检测是否更新显示器内容
 * @priority : 10
 *************************************************************************/
#define WORK_REFRESH_LCD_SIZE         512
#define WORK_REFRESH_LCD_PRIORITY     10
static K_THREAD_STACK_DEFINE(refresh_lcd_stack_area, WORK_REFRESH_LCD_SIZE);
static struct k_work_q work_refresh_lcd = {0};
struct work_refresh_lcd_info{
  struct k_work work;
  char name[10];
}work_refresh_lcd_info;
static void work_refresh_lcd_init()
{
  k_work_queue_start(&work_refresh_lcd, refresh_lcd_stack_area,
                      K_THREAD_STACK_SIZEOF(refresh_lcd_stack_area), 
                      WORK_REFRESH_LCD_PRIORITY,
                      NULL);
  strcpy(work_refresh_lcd_info.name, "data");
  k_work_init(&work_refresh_lcd_info.work, lcd_refresh_function);
}
static void submit_to_queue_refresh_lcd()
{
  k_work_submit_to_queue(&work_refresh_lcd, &work_refresh_lcd_info.work);
}
static void refresh_timer_handle(struct k_timer *dummy)		//timer tick = 10ms
{
	submit_to_queue_refresh_lcd();
}
static K_TIMER_DEFINE(LcdTimer, refresh_timer_handle, NULL);


/**************************************************************************
 * @brief 系统定时器 
 * @see   systimer tick 10ms
 * @priority:  8
 *************************************************************************/
#define WORK_SYSTIMER_SIZE         1024
#define WORK_SYSTIMER_PRIORITY     8
static K_THREAD_STACK_DEFINE(systimer_area, WORK_SYSTIMER_SIZE);
static struct k_work_q work_systimer = {0};
struct work_systimer_info{
  struct k_work work;
  char name[10];
}work_systimer_info;
static void work_systimer_stack_init()
{
  k_work_queue_start(&work_systimer, systimer_area,
                      K_THREAD_STACK_SIZEOF(systimer_area), 
                      WORK_SYSTIMER_PRIORITY,
                      NULL);
  strcpy(work_systimer_info.name, "systime");
  k_work_init(&work_systimer_info.work, systimer_function);
}
static void submit_to_queue_system_Timer()
{
  k_work_submit_to_queue(&work_systimer, &work_systimer_info.work);
}
static void SysTimer_handle(struct k_timer *dummy)		//timer tick = 10ms
{
  submit_to_queue_system_Timer();
}
static K_TIMER_DEFINE(sysTimer, SysTimer_handle, NULL);


void yongker_tm_work_queue_init()
{
  /**
   * @brief Construct a new work read ecg stack init object
   * @brief lcd工作队列 + lcd刷新定时器初始化
   */
  work_refresh_lcd_init();
  k_timer_start(&LcdTimer, K_MSEC(1), K_MSEC(10));
  /**
   * @brief 系统定时器
   */
  work_systimer_stack_init();
  k_timer_start(&sysTimer, K_MSEC(1), K_MSEC(10));
}




