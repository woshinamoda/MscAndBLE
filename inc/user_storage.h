#ifndef _USER_STORAGE_H_
#define _USER_STORAGE_H_

#include "main.h"

typedef enum{
  STORAGE_SUCCESS,
  DATA_SENDING,
  DATA_READING,
  DATA_NONE,
  READ_OK,

}storageReadSend_sta;

static const char ch0_title[] = {"本地通道的温湿度数据\n"};
static const char ch1_title[] = {"通道1数据\n"};
static const char ch2_title[] = {"通道2数据\n"};
static const char fifle_header[] = {"序号,类型,日期,温度,湿度,光强\n"};
/**
 * @brief 初始化flash和usb，同样的包含系统文件fatfs和对应每个通道的csv文件生成初始化。
 * 
 */
void Fatfs_storage_init();
/**向通道0-1-2内添加文件数据 */
void storageCutIn_chn0_data();
void storageCutIn_chn1_data();
void storageCutIn_chn2_data();
/**
 * @brief 删除所有通道的csv文件，重置存储序号和旗标
 */
void storage_clear_allFile();
void all_storage_close();
void all_storage_open();
/**对应通道文件，读取其数据,并且发送 */
void readStorage_SendData();
/* 中断读取操作，初始化所有读取变量 */
void stop_readStorage_SendSta();

void disable_qspi_mx25r32();
#endif