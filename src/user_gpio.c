/**
 * @file user_gpio.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-10-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include "user_gpio.h"

static void pca9546_gpio_init(){
  nrf_gpio_cfg_output(PCA_RESET);
  nrf_gpio_cfg_output(PCA_A0);
  nrf_gpio_cfg_output(PCA_A1);
  nrf_gpio_cfg_output(PCA_A2); 
  nrf_gpio_pin_set(PCA_RESET); 
  nrf_gpio_pin_clear(PCA_A0);
  nrf_gpio_pin_clear(PCA_A1);
  nrf_gpio_pin_clear(PCA_A2);
}
static void htl621_gpio_init()
{
  nrf_gpio_cfg_output(LCD_CS);
  nrf_gpio_cfg_output(LCD_WR);
  nrf_gpio_cfg_output(LCD_DATA);    
  nrf_gpio_pin_set(LCD_CS);
  nrf_gpio_pin_clear(LCD_WR);
  nrf_gpio_pin_clear(LCD_DATA);  
}

void yonker_tm_gpio_init()
{
  pca9546_gpio_init();
  htl621_gpio_init();

  

}











