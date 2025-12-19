/**
 * @file bat_ssadc.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-11-14
 * 
 * @copyright Copyright (c) 2025
 * @secreflist 电池电量还是
 */
#include "bat_ssadc.h"
#include "lcd.h"
#include <zephyr/bluetooth/services/bas.h>

static nrfx_saadc_channel_t channel = NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN0, 0);
static uint16_t sample;
static uint16_t bat_vol;
static uint16_t batRead_cnt;

static void BatAdc_tranTo_BatLev(uint16_t mvolts);
static void BatAdc_tranTo_FirstBatLeve(uint16_t mvolts);

void ssadc_init()
{
  IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)),
  DT_IRQ(DT_NODELABEL(adc), priority),
  nrfx_isr, nrfx_saadc_irq_handler, 0);
  nrfx_err_t err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
  if (err != NRFX_SUCCESS) 
  {
    printk("nrfx_saadc_mode_trigger error: %08x", err);
    return;
  }
  //nrfx_saadc_uninit();
  channel.channel_config.gain = NRF_SAADC_GAIN1_6;
  err = nrfx_saadc_channels_config(&channel, 1);
  if (err != NRFX_SUCCESS) 
  {
    printk("nrfx_saadc_channels_config error: %08x", err);
    return;
  }
  err = nrfx_saadc_simple_mode_set(BIT(0),
                                  NRF_SAADC_RESOLUTION_12BIT,
                                  NRF_SAADC_OVERSAMPLE_DISABLED,
                                  NULL);
  if (err != NRFX_SUCCESS){
      printk("nrfx_saadc_simple_mode_set error: %08x", err);
      return;
  }
  /* 初始化完成后再读一次数据 */
  err = nrfx_saadc_buffer_set(&sample, 1);
  if (err != NRFX_SUCCESS) {
      printk("nrfx_saadc_buffer_set error: %08x", err);
      return;
  }

  err = nrfx_saadc_mode_trigger();
  if (err != NRFX_SUCCESS) {
      printk("nrfx_saadc_mode_trigger error: %08x", err);
      return;
  }  
  bat_vol = ((600*6) * sample) / ((1<<12));
  /* 读完一次以后剩下全在定时器中读取*/
  BatAdc_tranTo_FirstBatLeve(bat_vol);  
  refresh_flag.adc_sta = true;
  bt_bas_set_battery_level(yk_tm.bat_precent);
}

static uint16_t batVolBuffer[10];
static uint8_t  batBuf_cnt=0;
void bat_Systime_handle()
{
  nrfx_err_t err;
  /* 充电状态下2min测一次电量 */
  if(yk_tm.charging_sta == true)
  {
    batRead_cnt++;
    if(batRead_cnt == 500)  //5sec时停止充电
    {
      nrf_gpio_pin_set(BQ_CE); 
    }
    if(batRead_cnt == 1000) //10sec时读取电量，恢复充电
    {
      err = nrfx_saadc_buffer_set(&sample, 1);
      if (err != NRFX_SUCCESS) {
          printk("Timer: nrfx_saadc_buffer_set error: %08x\n", err);
          return;
      }
      //处罚单次采样
      err = nrfx_saadc_mode_trigger();
      if (err != NRFX_SUCCESS) {
        printk("nrfx_saadc_mode_trigger error: %08x", err);
        return;
      }
      bat_vol = ((600 * 6) * sample) / (1 << 12);
      BatAdc_tranTo_BatLev(bat_vol);
      refresh_flag.adc_sta = true;
      bt_bas_set_battery_level(yk_tm.bat_precent);
      nrf_gpio_pin_clear(BQ_CE);  
    }
    if(batRead_cnt >= 12000) //2min为间隔，测一次电量
    {
      batRead_cnt = 0;
      nrf_gpio_pin_clear(BQ_CE);  
    }
  }
  /* 不充电时， 5sec测一次，累计10次计算一次电量 */
  else
  {
    batRead_cnt++;
    if(batRead_cnt >= BAT_LEV_PERIOD)
    {
      batRead_cnt = 0;
      //set buffer
      err = nrfx_saadc_buffer_set(&sample, 1);
      if (err != NRFX_SUCCESS) {
          printk("Timer: nrfx_saadc_buffer_set error: %08x\n", err);
          return;
      }
      //处罚单次采样
      err = nrfx_saadc_mode_trigger();
      if (err != NRFX_SUCCESS) {
        printk("nrfx_saadc_mode_trigger error: %08x", err);
        return;
      }
      batVolBuffer[batBuf_cnt] = sample;
      batBuf_cnt++;
      if(batBuf_cnt == 10)
      {
        batBuf_cnt = 0;
        uint16_t sub, avr;
        for(uint8_t i=0; i<10; i++)
        {
          sub = sub + batVolBuffer[i];
        }
        avr = sub/10;
        bat_vol = ((600 * 6) * avr) / (1 << 12);
        BatAdc_tranTo_BatLev(bat_vol);
        refresh_flag.adc_sta = true;
        bt_bas_set_battery_level(yk_tm.bat_precent);
      }
    }      
  }
}
static void BatAdc_tranTo_BatLev(uint16_t mvolts)
{
	if(mvolts >= 2050)
		{yk_tm.bat_precent = 100; yk_tm.bat_level = 4;}
	else if((mvolts <= 2008)&&(mvolts >= 2022))
		{yk_tm.bat_precent = 90; yk_tm.bat_level = 4;}
	else if((mvolts <= 1973)&&(mvolts >= 1987))
		{yk_tm.bat_precent = 80; yk_tm.bat_level = 4;}
	else if((mvolts <= 1938)&&(mvolts >= 1952))
		{yk_tm.bat_precent = 70; yk_tm.bat_level = 3;}
	else if((mvolts <= 1903)&&(mvolts >= 1917))
		{yk_tm.bat_precent = 60; yk_tm.bat_level = 3;}
	else if((mvolts <= 1868)&&(mvolts >= 1882))
		{yk_tm.bat_precent = 50; yk_tm.bat_level = 2;}
	else if((mvolts <= 1833)&&(mvolts >= 1847))
		{yk_tm.bat_precent = 40; yk_tm.bat_level = 2;}
	else if((mvolts <= 1798)&&(mvolts >= 1812))
		{yk_tm.bat_precent = 30; yk_tm.bat_level = 1;}
	else if((mvolts <= 1763)&&(mvolts >= 1777))
		{yk_tm.bat_precent = 20; yk_tm.bat_level = 1;}
	else if((mvolts <= 1728)&&(mvolts >= 1742))
		{yk_tm.bat_precent = 10; yk_tm.bat_level = 0;}
	else if((mvolts <= 1700))		//小于3.4V电池就应该断电了
		{yk_tm.bat_precent = 0; yk_tm.bat_level = 0;}
	else
		yk_tm.bat_precent = yk_tm.bat_precent;	//如果都不在范围内，就保持上一次读取的范围值
}
static void BatAdc_tranTo_FirstBatLeve(uint16_t mvolts)
{
	if(mvolts >= 2050)	//>4.1V
		{yk_tm.bat_precent = 100; yk_tm.bat_level = 4;}
	else if((mvolts < 2050)&&(mvolts >= 2015))	//4.1 > X > 4.03
		{yk_tm.bat_precent = 90; yk_tm.bat_level = 4;}
	else if((mvolts < 2015)&&(mvolts >= 1980))  //4.03 > X > 3.96
		{yk_tm.bat_precent = 80; yk_tm.bat_level = 3;}
	else if((mvolts < 1980)&&(mvolts >= 1945))	//3.96 > X > 3.89
		{yk_tm.bat_precent = 70; yk_tm.bat_level = 3;}
	else if((mvolts < 1945)&&(mvolts >= 1910))  //3.89 > X > 3.82
		{yk_tm.bat_precent = 60; yk_tm.bat_level = 3;}
	else if((mvolts < 1910)&&(mvolts >= 1875))
		{yk_tm.bat_precent = 50; yk_tm.bat_level = 2;}
	else if((mvolts < 1875)&&(mvolts >= 1840))
		{yk_tm.bat_precent = 40; yk_tm.bat_level = 2;}
	else if((mvolts < 1840)&&(mvolts >= 1805))
		{yk_tm.bat_precent = 30; yk_tm.bat_level = 1;}
	else if((mvolts < 1805)&&(mvolts >= 1770))
		{yk_tm.bat_precent = 20; yk_tm.bat_level = 1;}
	else if((mvolts <= 1770)&&(mvolts >= 1735))
		{yk_tm.bat_precent = 10; yk_tm.bat_level = 0;}
	else if((mvolts < 1700))
		{yk_tm.bat_precent = 0; yk_tm.bat_level = 0;}
}

















