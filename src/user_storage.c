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
  * @brief 参考链接
  * https://devzone.nordicsemi.com/f/nordic-q-a/113000/usb-mass-sample-and-partition-manager-build-issues
  * 
  */
 /**
 * @file use overlay set notice 
 * @page 25 : 由明确指出mx25r32的RDID，相比8MB，memory density 由 17 -> 16
 * @page 30 : status寄存器bit6 设置flash的QE位为1，开启4线模式
 */
#include "user_storage.h"
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h> 
#include <ff.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);


const struct device *flash_dev = DEVICE_DT_GET(DT_NODELABEL(mx25r32));

/* devicetree 分区存储 */
#define STORAGE_PARTITION		    fatfs_storage
#define STORAGE_PARTITION_ID		FIXED_PARTITION_ID(STORAGE_PARTITION)

/* 挂载描述结构体 */
static struct fs_mount_t fs_mnt;
/* 文件的结构体  */
struct fs_file_t  chn0_file;  //本地温湿度通道文件
struct fs_file_t  chn1_file;  //外接探头1
struct fs_file_t  chn2_file;  //外接探头2

/**
 * @brief 初始化flash分区
 * 
 * @param mnt 挂载结构体
 * @return int 初始化返回值
 */
static int setup_flash(struct fs_mount_t *mnt)
{
	int rc = 0;
#if CONFIG_DISK_DRIVER_FLASH
	unsigned int id;
	const struct flash_area *pfa;

	mnt->storage_dev = (void *)STORAGE_PARTITION_ID;
	id = STORAGE_PARTITION_ID;
  /* 分区ID获取flash飞去详细信息 */
	rc = flash_area_open(id, &pfa);
	LOG_INF("Area %u at 0x%x on %s for %u bytes",
    id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
    (unsigned int)pfa->fa_size);
  
  /* kconfig自己定义app wipe storage，如果打开分区失败，擦出全片 */
	if (rc < 0 && IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
		LOG_INF("Erasing flash area ... ");
		rc = flash_area_erase(pfa, 0, pfa->fa_size);
		LOG_INF("%d", rc);
	}
  /* 分区失败，关闭分区，释放掉flash area资源 */
	if (rc < 0) {
		flash_area_close(pfa);
	}
#endif
	return rc;
}
/**
 * @brief 将fatfs文件系统挂载到指定挂载点。
 * 
 * @param mnt 
 * @return int 
 */
static int mount_app_fs(struct fs_mount_t *mnt)
{
	int rc;
	static FATFS fat_fs;

	mnt->type = FS_FATFS; 
	mnt->fs_data = &fat_fs;
	mnt->mnt_point = "/NAND:";

	rc = fs_mount(mnt);

	return rc;
}
/**
 * @brief Set the up disk object
 * QSPI flash分区初始化，文件系统挂载，空间信息查询，目录内容遍历
 * 
 */
static void setup_disk(void)
{
	struct fs_mount_t *mp = &fs_mnt;    //挂载(fatfs)结构体
	struct fs_dir_t dir;                //目录操作结构体
	struct fs_statvfs sbuf;             //用于获取文件系统空间信息
	int rc;

  /* 初始化目录结构体，防止野指针 */
	fs_dir_t_init(&dir);

  /* flash分区 */
	if (IS_ENABLED(CONFIG_DISK_DRIVER_FLASH)) {
		rc = setup_flash(mp);
		if (rc < 0) {
			LOG_ERR("Failed to setup flash area");
			return;
		}
	}

  /* 文件系统类型，咱们config定义的fatfs */
	if (!IS_ENABLED(CONFIG_FILE_SYSTEM_LITTLEFS) &&
	    !IS_ENABLED(CONFIG_FAT_FILESYSTEM_ELM)) {
		LOG_INF("No file system selected");
		return;
	}

  /* 挂载文件系统 */
	rc = mount_app_fs(mp);
	if (rc < 0) {
		LOG_ERR("Failed to mount filesystem");
		return;
	}

	/* Allow log messages to flush to avoid interleaved output */
	k_sleep(K_MSEC(50));
	LOG_INF("Mount %s: %d", fs_mnt.mnt_point, rc);

  /* 查询文件系统空间信息 */
	rc = fs_statvfs(mp->mnt_point, &sbuf);
	if (rc < 0) {
		LOG_INF("FAIL: statvfs: %d", rc);
		return;
	}

	LOG_INF("%s: bsize = %lu ; frsize = %lu ;"
  " blocks = %lu ; bfree = %lu",
  mp->mnt_point,
  sbuf.f_bsize, sbuf.f_frsize,
  sbuf.f_blocks, sbuf.f_bfree);

  /* 遍历更目录内容，并打印 */
	rc = fs_opendir(&dir, mp->mnt_point);
	LOG_INF("%s opendir: %d", mp->mnt_point, rc);

	if (rc < 0) {
		LOG_ERR("Failed to open directory");
	}

	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if (rc < 0) {
			LOG_ERR("Failed to read directory entries");
			break;
		}
		if (ent.name[0] == 0) {
			LOG_INF("End of files");
			break;
		}
		LOG_INF("  %c %u %s",
    (ent.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
    ent.size,
    ent.name);
	}
  /* 关闭文件空间获取 */
	(void)fs_closedir(&dir);

	return;
}
/**
 * @brief ym_tm csv文件初始化
 */
static void yk_tm_cvs_init()
{
  fs_file_t_init(&chn0_file);
  fs_file_t_init(&chn1_file);
  fs_file_t_init(&chn2_file);
  int rc1 = fs_open(&chn0_file, "/NAND:/ch0.csv", FS_O_CREATE | FS_O_RDWR);
  int rc2 = fs_open(&chn1_file, "/NAND:/ch1.csv", FS_O_CREATE | FS_O_RDWR);
  int rc3 = fs_open(&chn2_file, "/NAND:/ch2.csv", FS_O_CREATE | FS_O_RDWR);
  if (rc1 >= 0) {
    fs_write(&chn0_file, ch0_title, sizeof(ch0_title));
    fs_write(&chn0_file, fifle_header, sizeof(fifle_header));
  }
  if (rc2 >= 0) {
    fs_write(&chn1_file, ch1_title, sizeof(ch1_title));
    fs_write(&chn1_file, fifle_header, sizeof(fifle_header));    
  }
  if (rc3 >= 0) {
    fs_write(&chn2_file, ch2_title, sizeof(ch2_title));
    fs_write(&chn2_file, fifle_header, sizeof(fifle_header));    
  }
  if (rc1 >= 0) {
    fs_close(&chn0_file);
  }
  if (rc2 >= 0) {
    fs_close(&chn1_file);
  }
  if (rc3 >= 0) {
    fs_close(&chn2_file);
  }
}
/**
 * @brief 测试mx25r32设备是否存在
 */
static void mx25r32_flash_init()
{
  int err = 10;
  err = device_is_ready(flash_dev);
  printk("flash dev is :%d \n", err);
}
/**********************************************************/

void Fatfs_storage_init()
{
  int err;
  mx25r32_flash_init();
  k_msleep(10);
  setup_disk();
  yk_tm_cvs_init();
  usb_enable(NULL);
}










































