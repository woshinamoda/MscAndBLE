#ifndef _USER_STORAGE_H_
#define _USER_STORAGE_H_

#include "main.h"

static const char ch0_title[] = {"本地通道的温湿度数据\n\r"};
static const char ch1_title[] = {"通道1数据\n\r"};
static const char ch2_title[] = {"通道2数据\n\r"};
static const char fifle_header[] = {"序号,类型,日期,数据\n\r"};
/**
 * @brief 初始化flash和usb，同样的包含系统文件fatfs和对应每个通道的csv文件生成初始化。
 * 
 */
void Fatfs_storage_init();











#endif