/**
 * @file user_storage.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-10-27
 * 
 * @copyright Copyright (c) 2025
 * @see mx25r32的驱动部分，sector:4k  block:32k  IO速度可达80MHZ，我们用qspi丝弦传输，速率约为320MHZ，我们实际就用ultra low power
 * mode, 对应8MHZ频率即可够用
 * @see flash memory organization 一共1024sector(4kB) 128block(32KB) 地址0x000000 ---  0x3fffff
 * @page 40 : dual read mode 
 */

 /**
 * @file use overlay set notice 
 * @page 25 : 由明确指出mx25r32的RDID，相比8MB，memory density 由 17 -> 16
 * @page 30 : status寄存器bit6 设置flash的QE位为1，开启4线模式
 */
#include "user_storage.h"
#include <zephyr/drivers/flash.h>
// #include <zephyr/fs/fs.h>
// #include <ff.h>
// #include <zephyr/device.h>
// #include <zephyr/drivers/flash.h>
// #include <stdio.h>
// #include <string.h>

const struct device *flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r32));
// static FATFS fat_fs;
// struct fs_file_t file;
// static struct fs_mount_t fatfs_mnt = {
//   .type = FS_FATFS,
//   .fs_data = &fat_fs,
//   .mnt_point = "/NAND:",
// };


static void mx25r32_flash_init()
{
  int err = 10;
  err = device_is_ready(flash_dev);
  printk("flash dev is :%d \n", err);
}
void Fatfs_storage_init()
{
  int err;
  mx25r32_flash_init();
  k_msleep(10);
  // err = fs_mount(&fatfs_mnt);
  //   if (err == 0) {
  //     printk("FATFS mounted successfully.\n");
  //   } else {
  //     printk("Error mounting FATFS: %d\n", err);
  // }
  // /* 开始系统文件初始化 */
  // fs_file_t_init(&file);

  // const char *file_path = "/NAND:/mydata.cvs";
  // int res = fs_open(&file, file_path, FS_O_CREATE | FS_O_WRITE);
  // if (res < 0) {
  //   printk("Failed to open file: %d\n", res);
  //   return;
  // }
  // /* 测试CSV内容 */ 
  // const char *csv_data = "id,value\n1,100\n2,200\n";
  // res = fs_write(&file, csv_data, strlen(csv_data));
  // if (res < 0) {
  //   printk("Failed to write file: %d\n", res);
  // } else {
  //   printk("CSV file written successfully.\n");
  // }
  // fs_close(&file);
}





















