#ifndef _SENSOR_H_
#define _SENSOR_H_

#include "main.h"

/** @see sht40 datasheet information notice 
 * device type is SHT40-AD1B, 0x44 I2c addr.
 */
#define SHT40_RESET                  0x94
#define SHT40_NUMBER                 0x89
#define SHT40_HIGH_REPEAT            0xFD
#define SHT40_MEDIUM_REPEAT          0xF6
#define SHT40_LOW_REPEAT             0xE0
/** @see active heater measurement */
#define SHT40_200MW_1SEC             0x39
#define SHT40_200MW_0_1SEC           0x32
#define SHT40_110MW_1SEC             0x2F
#define SHT40_110MW_0_1SEC           0x24
#define SHT40_20MW_1SEC              0x1E
#define SHT40_20MW_0_1SEC            0x15

/**
 * @brief device bh1750 register list
 * bh1750 resolurtion depend on mode set , @see P5
 * H-mode2 - 0.5lx 
 * H-mode - 1lx
 * L-mode - 4lx
 */
#define BH1750_PDOWN                  0x00
#define BH1750_PON                    0x01
#define BH1750_RESET                  0x07
#define BH1750_CONTINU_H_MODE         0x10
#define BH1750_CONTINU_H_MODE2        0x11
#define BH1750_CONTINU_L_MODE         0x13
#define BH1750_SINGLE_H_MODE          0x20
#define BH1750_SINGLE_H_MODE2         0x21
#define BH1750_SINGLE_L_MODE          0x23

/**
 * @brief device max44009 ambient light sensor register list
 * wide dynamic range 0.045lux - 188000lux
 */
#define MAX_INT_STATUS                0x00
#define MAX_INIT_ENABLE               0x01
#define MAX_CONFIGURATION             0x02
#define MAX_HIGH_BYTE                 0x03
#define MAX_LOW_BYTE                  0x04
#define MAX_UPPER_THRESHOLD           0x05
#define MAX_LOWER_THRESHOLD           0x06
#define MAX_THRESHOLD_TIMER           0x07

typedef enum
{
  none      = 0x00,
  sht40     = 0x01,
  bh1750    = 0x02,
  max44009  = 0x03
}sensor_TypeDef;


/**
 * @brief 检测通道是否存在传感器
 * 
 * @param chn 需要检测的通道序号
 * @return 返回对应通道的传感器是否存在，与存在的传感器类型
 */
uint8_t CheckChn_Sensor_is(uint8_t chn);
/**
 * @brief 根据寄存器类型读取对应的数据，然后通过指针传递外部数组
 * 
 * @param databuf 数据缓存
 * @param type 传感器类型
 */
void ReadSensor_Data(uint8_t *databuf , sensor_TypeDef type);










#endif