#include "button.h"
#include "sensor.h"
#include "lcd.h"


#define LOG_BUTTON_NAME KEY_LOG
LOG_MODULE_REGISTER(LOG_BUTTON_NAME);

extern bool sensor_read_flag;
extern uint16_t sensor_read_Tcnt;

extern bool stop_charging_delay;
extern uint16_t stop_charging_cnt;

/**
 * @brief 按键事件判断逻辑
 * 
 * @see 参考原先61C1的按键逻辑，执行如下
 * 
 *  按下小于50ms弹起， 视为抖动，误触发
 *  按下时间大于50ms， 小于1000ms内弹起， 视作一次短按触发
 *  按下时间大于1000ms， 每过1000ms触发一次长按事件
 */
#define   KEY_ERROR_TIME      5       
#define   KEY_LONG_TIME       100

Button_InitTypeDef  mykey;
Vcheck_InitTypeDef  myVcheck;

/* ============================================================================================== */
#define KEY_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button_dev = GPIO_DT_SPEC_GET(KEY_NODE, gpios);
static struct gpio_callback button_cb_data;
static void button_int_handle(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
  mykey.press_flag = true;
}
void button_gpiote_init()
{
	int ret;
	if (!device_is_ready(button_dev.port))
	{
		return;
	}
	ret = gpio_pin_configure_dt(&button_dev, GPIO_INPUT);
	if (ret < 0)
	{
		return;
	}
	else
		LOG_INF("user button init ok\n");
	ret = gpio_pin_interrupt_configure_dt(&button_dev, GPIO_INT_EDGE_FALLING);    //值检测下降沿按下，不检测按键弹起。 在定时器内判断弹起来时间，决定长短按触发事件
	gpio_init_callback(&button_cb_data, button_int_handle, BIT(button_dev.pin));  
	gpio_add_callback(button_dev.port, &button_cb_data);
}
void button_EvenTimer_handle()
{
  if(mykey.press_flag)
  {
    if(nrf_gpio_pin_read(BUTTON)==0)
    {//按键按下
      mykey.press_cnt++;
      if(mykey.press_cnt >= KEY_LONG_TIME)
      {
        mykey.long_flag = true;
        mykey.press_cnt = 0;  //重置，循环执行
        button_long_cb();
      }
    }
    else
    {//按键弹起
      mykey.press_flag = false;
      if(mykey.long_flag)
      {
        mykey.long_flag = false;
        mykey.press_cnt = 0;
      }
      else
      {
        if(mykey.press_cnt <= KEY_ERROR_TIME)
        {
          mykey.press_cnt = 0;
          LOG_INF("key press error"); //包含前面在内消抖
        }
        else if ((KEY_ERROR_TIME < mykey.press_cnt)&&(mykey.press_cnt < KEY_LONG_TIME))
        {
          mykey.press_cnt = 0;
          button_short_cb();
        }
      }
    }
  }
}
void button_short_cb()
{
  LOG_INF("key press short\n");
  yk_tm.display_chn++;
  if(yk_tm.display_chn == 3)
  {
    yk_tm.display_chn = 0;
  }
  channel_0.temp_type_is_C = true;
  channel_1.temp_type_is_C = true;
  channel_2.temp_type_is_C = true;
  yk_tm.warm_icon_sta = false;    //每次触发按键，也需要重置一下警告图标
  sensor_read_flag = true;
  refresh_flag.channel_dis_sta = true;  
}
void button_long_cb()
{
  if(channel_0.temp_type_is_C)
    channel_0.temp_type_is_C = false;
  else if(!channel_0.temp_type_is_C)
    channel_0.temp_type_is_C = true;

  if(channel_1.temp_type_is_C)
    channel_1.temp_type_is_C = false;
  else if(!channel_1.temp_type_is_C)
    channel_1.temp_type_is_C = true;

  if(channel_2.temp_type_is_C)
    channel_2.temp_type_is_C = false;
  else if(!channel_2.temp_type_is_C)
    channel_2.temp_type_is_C = true;     

  refresh_flag.channel_data_sta = true;   
  LOG_INF("key press long\n");
}
/* ============================================================================================== */
#define VCHECK_NODE DT_ALIAS(sw1)
static const struct gpio_dt_spec vcheck_dev = GPIO_DT_SPEC_GET(VCHECK_NODE, gpios);
static struct gpio_callback vcheck_cb_data;
static void vcheck_int_handle(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
  myVcheck.toogle_flag = true;
}
void vcheck_gpiote_init()
{
	int ret;
	if (!device_is_ready(vcheck_dev.port))
	{
		return;
	}
	ret = gpio_pin_configure_dt(&vcheck_dev, GPIO_INPUT);
	if (ret < 0)
	{
		return;
	}
	else
		LOG_INF("user button init ok\n");
	ret = gpio_pin_interrupt_configure_dt(&vcheck_dev, GPIO_INT_EDGE_BOTH); 
	gpio_init_callback(&vcheck_cb_data, vcheck_int_handle, BIT(vcheck_dev.pin));  
	gpio_add_callback(vcheck_dev.port, &vcheck_cb_data);
}
void vcheck_EvenTimer_handle()
{
  if(myVcheck.toogle_flag)
  {
    myVcheck.active_cnt++;
    if(myVcheck.active_cnt >= 5)
    {
      nrf_gpio_pin_clear(BQ_CE); //无论插入还是拔下充电器，ce引脚都拉低
      if(nrf_gpio_pin_read(VCHECK))
        yk_tm.charging_sta = true;
      else
      {
        stop_charging_cnt = 0;
        stop_charging_delay = true;
        yk_tm.charging_sta = false;
      }
      myVcheck.active_cnt = 0;
      myVcheck.toogle_flag = false;
      refresh_flag.charging_sta = true;        
    }
  }
}














