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
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/nus.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/logging/log.h>
#include "user_rtc.h"
LOG_MODULE_REGISTER(app);

extern struct bt_conn *my_conn;
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
struct fs_file_t  info_file; 

static void set_info_windows_dis();

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

	set_info_windows_dis();

	return;
}
/**
 * @brief ym_tm csv文件初始化
 */
static void yk_tm_cvs_init()
{
  fs_file_t_init(&chn0_file);
  int rc1 = fs_open(&chn0_file, "/NAND:/ch0.csv", FS_O_CREATE | FS_O_RDWR );	
  fs_file_t_init(&chn1_file);
  int rc2 = fs_open(&chn1_file, "/NAND:/ch1.csv", FS_O_CREATE | FS_O_RDWR );	
  fs_file_t_init(&chn2_file);
  int rc3 = fs_open(&chn2_file, "/NAND:/ch2.csv", FS_O_CREATE | FS_O_RDWR );
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
static void set_info_windows_dis()
{
  fs_file_t_init(&info_file);
  int rc = fs_open(&info_file, "/NAND:/autorun.inf", FS_O_CREATE | FS_O_WRITE);
	fs_write(&info_file,"[AutoRun]\r\nlabel=YonkerTM\r\n",27);
	fs_close(&info_file);
}
/**********************************************************/
void Fatfs_storage_init()
{
  mx25r32_flash_init();
  k_msleep(10);
  setup_disk();
  yk_tm_cvs_init();
	usb_enable(NULL);	
	storage_clear_allFile();
}
void all_storage_open()
{
	usb_disable();
	/*notice:必要的延时，防止断开usb瞬间有存入数据，qspi和usb共同占用fatfs*/
	k_msleep(100);
}
void all_storage_close()
{
	fs_sync(&chn0_file);
	fs_sync(&chn1_file);
	fs_sync(&chn2_file);
	/*必要的延时，同理防止usb和qspi共同占用*/
	k_msleep(100);
	usb_enable(NULL);
}
/**********************************************************/
void storageCutIn_chn0_data()
{
	if(channel_0.channel_type != nosensor)
	{
		char line[256];
		uint16_t len;		
		// 如果存满了，循环到第一条记录，写入表头
		if (channel_0.storage_idx == 0) {
			fs_seek(&chn0_file, 0, FS_SEEK_SET);
			fs_write(&chn0_file, ch0_title, sizeof(ch0_title));
			fs_write(&chn0_file, fifle_header, sizeof(fifle_header));
		}
		len = snprintf(line, sizeof(line), "%05d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",
										channel_0.storage_idx,
										channel_0.channel_type,
										timeInfo_stamp.year, timeInfo_stamp.month, timeInfo_stamp.day,
										timeInfo_stamp.hour, timeInfo_stamp.min,
										channel_0.temp_celsius,
										channel_0.humidity,
										channel_0.klux);
		printk("fatfs len is\n", sizeof(line));						
		if (len > 0 && len < sizeof(line)) 
		{
			fs_write(&chn0_file, line, 40);
		} else 
		{
			printk("格式化错误或缓冲区不足: 需要%d字节\n", len);
		}
		channel_0.storage_idx++;	
		if(channel_0.storage_idx % 500 == 0)	
		{
			fs_sync(&chn0_file);
		}
		if(channel_0.storage_idx >= 19999)
		{
			channel_0.storage_over = true; //存储数据已经超过20000条，开始覆盖存储
			channel_0.storage_idx = 0;
		}
	}
}
void storageCutIn_chn1_data()
{
	if(channel_1.channel_type != nosensor)
	{
		char line[256];
		uint16_t len;		
		// 如果存满了，循环到第一条记录，写入表头
		if (channel_1.storage_idx == 0) {
			fs_seek(&chn1_file, 0, FS_SEEK_SET);
			fs_write(&chn1_file, ch1_title, sizeof(ch1_title));
			fs_write(&chn1_file, fifle_header, sizeof(fifle_header));
		}
		len = snprintf(line, sizeof(line), "%05d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",
										channel_1.storage_idx,
										channel_1.channel_type,
										timeInfo_stamp.year, timeInfo_stamp.month, timeInfo_stamp.day,
										timeInfo_stamp.hour, timeInfo_stamp.min,
										channel_1.temp_celsius,
										channel_1.humidity,
										channel_1.klux);
		printk("fatfs len is\n", sizeof(line));						
		if (len > 0 && len < sizeof(line)) 
		{
			fs_write(&chn1_file, line, 40);
		} else 
		{
			printk("格式化错误或缓冲区不足: 需要%d字节\n", len);
		}
		channel_1.storage_idx++;	
		if(channel_1.storage_idx % 500 == 0)	
		{
			fs_sync(&chn1_file);
		}			
		if(channel_1.storage_idx >= 19999)
		{
			channel_1.storage_over = true; //存储数据已经超过20000条，开始覆盖存储
			channel_1.storage_idx = 0;
		}
	}
}
void storageCutIn_chn2_data()
{
	if(channel_2.channel_type != nosensor)
	{
		char line[256];
		uint16_t len;		
		// 如果存满了，循环到第一条记录，写入表头
		if (channel_2.storage_idx == 0) {
			fs_seek(&chn2_file, 0, FS_SEEK_SET);
			fs_write(&chn2_file, ch2_title, sizeof(ch2_title));
			fs_write(&chn2_file, fifle_header, sizeof(fifle_header));
		}
		len = snprintf(line, sizeof(line), "%05d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",
										channel_2.storage_idx,
										channel_2.channel_type,
										timeInfo_stamp.year, timeInfo_stamp.month, timeInfo_stamp.day,
										timeInfo_stamp.hour, timeInfo_stamp.min,
										channel_2.temp_celsius,
										channel_2.humidity,
										channel_2.klux);
		printk("fatfs len is\n", sizeof(line));						
		if (len > 0 && len < sizeof(line)) 
		{
			fs_write(&chn2_file, line, 40);
		} else 
		{
			printk("格式化错误或缓冲区不足: 需要%d字节\n", len);
		}
		channel_2.storage_idx++;		
		if(channel_2.storage_idx % 500 == 0)	
		{
			fs_sync(&chn2_file);
		}		
		if(channel_2.storage_idx >= 19999)
		{
			channel_2.storage_over = true; //存储数据已经超过20000条，开始覆盖存储
			channel_2.storage_idx = 0;
		}
	}
}
void storage_clear_allFile()
{
	int rc;
	yk_tm.storage_sta = false;  //停止有可能继续采集
	k_msleep(10);
	channel_0.storage_idx = 0;
	channel_0.storage_over = false;
	channel_1.storage_idx = 0;
	channel_1.storage_over = false;
	channel_2.storage_idx = 0;
	channel_2.storage_over = false;		

	fs_close(&chn0_file);
	fs_close(&chn1_file);
	fs_close(&chn2_file);
    
	rc = fs_unlink("/NAND:/ch0.csv");
	if (rc == 0) {
		printk("通道0文件删除成功\n");
	}
	rc = fs_unlink("/NAND:/ch1.csv");
	if (rc == 0) {
		printk("通道1文件删除成功\n");
	}
	rc = fs_unlink("/NAND:/ch2.csv");
	if (rc == 0) {
		printk("通道2文件删除成功\n");
	}		
	yk_tm_cvs_init();
}
uint8_t chn0_sendbuf[214] = {0xdd,0xcc};		/*通道0<-->15个点数据存1包，发送缓存处*/
uint8_t chn0_cnt = 0;												/*通道0发送计数旗标*/
uint8_t chn1_sendbuf[214] = {0xdd,0xcc};		
uint8_t chn1_cnt = 0;									
uint8_t chn2_sendbuf[214] = {0xdd,0xcc};	
uint8_t chn2_cnt = 0;
/*解析文件系统专用缓存--------------------*/
static uint8_t sendbuf[16];						//单纯用于解析使用dd cc + 14bytes数据
static uint8_t line[40]; 							//固定读取40bytes
static int record_size = 40; 					//和写一样，固定一行40bytes，带标点，	
static int storage_idx,year,hum,klux;	//解析数据，蓝牙传参要简略
static int temp;
static int channel_type,month,day,hour,min;
static int readStorage_chn0Data()
{
	int rc;
	/*先判断是否处于发送过程，发送过程中不读数据，否则会丢包*/
	if(channel_0.sending_sta)
	{
		return DATA_SENDING;
	}
	/*在判断是否有存储数据，没有直接判断读取完成*/
	if((channel_0.storage_idx==0)&&(channel_0.storage_over==false))
	{
		channel_0.storage_read_idx = 0;
		channel_0.storage_read_ok = true;
		return DATA_NONE;
	}
	/* 最后判断是否读取完成，如果读取cnt >=20000或者大于=存储cnt，说明读取完成了 */
	if(channel_0.storage_over == true)
	{
		if(channel_0.storage_read_idx >= 20000)
		{
			channel_0.storage_read_idx = 0;
			channel_0.storage_over = false;
			channel_0.storage_read_ok = true;
			if(channel_0.sending_cnt != 0) //刚好读取够15条，同时也读完了。会自动配置好数组包头/包尾巴
			{
				channel_0.sending_cnt = 0;
				chn0_sendbuf[0] = 0xDD;
				chn0_sendbuf[1] = 0xCC;			
				chn0_sendbuf[212] = 0x0d;
				chn0_sendbuf[213] = 0x0a;					
				channel_0.sending_sta = true;
				return READ_OK;
			}	
		}
	}
	else
	{
		if(channel_0.storage_read_idx >= channel_0.storage_idx)
		{
			channel_0.storage_read_idx = 0;
			channel_0.storage_read_ok = true;
			if(channel_0.sending_cnt != 0)
			{
				channel_0.sending_cnt = 0;
				chn0_sendbuf[0] = 0xDD;
				chn0_sendbuf[1] = 0xCC;			
				chn0_sendbuf[212] = 0x0d;
				chn0_sendbuf[213] = 0x0a;			
				channel_0.sending_sta = true;		
				return READ_OK;
			}					
		}
	}
	//从开始到读取完的执行逻辑
	//跳过csv表头
	if(channel_0.storage_read_idx == 0)
	{
		int header_size = sizeof(ch0_title) + sizeof(fifle_header);
		int position = header_size + (channel_0.storage_read_idx * record_size);
		rc = fs_seek(&chn0_file, position, FS_SEEK_SET);
		if (rc < 0) {
			printk("定位文件失败: %d\n", rc);
		}
	}
	//读取并解析				
	fs_read(&chn0_file, line, record_size);	
	int parsed = sscanf(line,  "%05d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",	&storage_idx,&channel_type, &year, &month, &day, &hour, &min, &temp, &hum, &klux);
	printk("parsed is %d:\n\r" ,parsed );
	if(parsed == 10)
	{
		sendbuf[2] = (year >> 8) & 0xff;
		sendbuf[3] = year & 0xff;
		sendbuf[4] = month;
		sendbuf[5] = day;
		sendbuf[6] = hour;
		sendbuf[7] = min;
		sendbuf[8] = 0;		/* 通道0👌 */
		sendbuf[9] = channel_type;
		if(channel_type == sht40)
		{
			sendbuf[10] = (temp >> 8) & 0xff;
			sendbuf[11] = temp & 0xff;
			sendbuf[12] = (hum >> 8) & 0xff;
			sendbuf[13] = hum & 0xff;
		}
		else
		{//除了温湿度，剩下全是光照
			sendbuf[10] = (klux >> 8) & 0xff;
			sendbuf[11] = klux & 0xff;
			sendbuf[12] = 0x00;
			sendbuf[13] = 0x00;
		}
		sendbuf[14] = (storage_idx >> 8) & 0xff;
		sendbuf[15] = storage_idx & 0xff;
	}
	memcpy(&chn0_sendbuf[2+channel_0.sending_cnt*14], &sendbuf[2], 14);
	channel_0.sending_cnt++;
	if(channel_0.sending_cnt >= 15)
	{
		channel_0.sending_cnt = 0;
		chn0_sendbuf[0] = 0xDD;
		chn0_sendbuf[1] = 0xCC;			
		chn0_sendbuf[212] = 0x0d;
		chn0_sendbuf[213] = 0x0a;
		channel_0.sending_sta = true;	/*👀 现在读取完了，该执行并列的发送了*/
	}
	channel_0.storage_read_idx++;	/* 不管发送，现将读取完成后，预定变量+1 */
}
static int BleSend_Chn0Data()
{
	int err;
	if(!channel_0.sending_sta)
	{
		return DATA_READING;
	}
	if(channel_0.sending_retry == true)
	{
		err = bt_nus_send(my_conn, chn0_sendbuf, 214);
		if(err)
		{
			//printk("retry defeat\n");
		}
		else
		{
			channel_0.sending_retry = false;
			memset(chn0_sendbuf,0,sizeof(chn0_sendbuf));
			//printk("retry ok\n");
		}
	}
	else
	{
		err = bt_nus_send(my_conn, chn0_sendbuf, 214);
		if(err)
		{
			channel_0.sending_retry = true;
			//printk("first send data unsuccess\n");		
		}
		else
		{
			channel_0.sending_sta = false;
			memset(chn0_sendbuf,0,sizeof(chn0_sendbuf));
			//printk("first data send ok\n");		
		}
	}
} 
static int readStorage_chn1Data()
{
	int rc;
	/*先判断是否处于发送过程，发送过程中不读数据，否则会丢包*/
	if(channel_1.sending_sta)
	{
		return DATA_SENDING;
	}
	/*在判断是否有存储数据，没有直接判断读取完成*/
	if((channel_1.storage_idx==0)&&(channel_1.storage_over==false))
	{
		channel_1.storage_read_idx = 0;
		channel_1.storage_read_ok = true;
		return DATA_NONE;
	}
	/* 最后判断是否读取完成，如果读取cnt >=20000或者大于=存储cnt，说明读取完成了 */
	if(channel_1.storage_over == true)
	{//数据超过20000条
		if(channel_1.storage_read_idx >= 20000)
		{
			channel_1.storage_over = false;			
			channel_1.storage_read_idx = 0;
			channel_1.storage_read_ok = true;
			if(channel_1.sending_cnt != 0) //如果之前读过，刚好读取够15条，同时也读完了。会自动配置好数组包头/包尾巴
			{
				channel_1.sending_cnt = 0;
				chn1_sendbuf[0] = 0xDD;
				chn1_sendbuf[1] = 0xCC;			
				chn1_sendbuf[212] = 0x0d;
				chn1_sendbuf[213] = 0x0a;					
				channel_1.sending_sta = true;
				return READ_OK;
			}	
		}
	}
	else
	{
		if(channel_1.storage_read_idx >= channel_1.storage_idx)
		{
			channel_1.storage_read_idx = 0;
			channel_1.storage_read_ok = true;
			if(channel_1.sending_cnt != 0)
			{
				channel_1.sending_cnt = 0;
				chn1_sendbuf[0] = 0xDD;
				chn1_sendbuf[1] = 0xCC;			
				chn1_sendbuf[212] = 0x0d;
				chn1_sendbuf[213] = 0x0a;			
				channel_1.sending_sta = true;		
				return READ_OK;
			}
		}
	}
	//从开始到读取完的执行逻辑
	//跳过csv表头
	if(channel_1.storage_read_idx == 0)
	{
		int header_size = sizeof(ch1_title) + sizeof(fifle_header);
		int position = header_size + (channel_1.storage_read_idx * record_size);
		rc = fs_seek(&chn1_file, position, FS_SEEK_SET);
		if (rc < 0) {
			printk("定位文件失败: %d\n", rc);
		}
	}
	//读取并解析				
	fs_read(&chn1_file, line, record_size);	
	int parsed = sscanf(line,  "%05d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",	&storage_idx,&channel_type, &year, &month, &day, &hour, &min, &temp, &hum, &klux);
	printk("parsed is %d:\n\r" ,parsed );
	if(parsed == 10)
	{
		sendbuf[2] = (year >> 8) & 0xff;
		sendbuf[3] = year & 0xff;
		sendbuf[4] = month;
		sendbuf[5] = day;
		sendbuf[6] = hour;
		sendbuf[7] = min;
		sendbuf[8] = 1;		/* 通道1👌 */
		sendbuf[9] = channel_type;
		if(channel_type == sht40)
		{
			sendbuf[10] = (temp >> 8) & 0xff;
			sendbuf[11] = temp & 0xff;
			sendbuf[12] = (hum >> 8) & 0xff;
			sendbuf[13] = hum & 0xff;
		}
		else
		{//除了温湿度，剩下全是光照
			sendbuf[10] = (klux >> 8) & 0xff;
			sendbuf[11] = klux & 0xff;
			sendbuf[12] = 0x00;
			sendbuf[13] = 0x00;
		}
		sendbuf[14] = (storage_idx >> 8) & 0xff;
		sendbuf[15] = storage_idx & 0xff;
	}
	memcpy(&chn1_sendbuf[2+channel_1.sending_cnt*14], &sendbuf[2], 14);
	channel_1.sending_cnt++;
	if(channel_1.sending_cnt >= 15)
	{
		channel_1.sending_cnt = 0;
		chn1_sendbuf[0] = 0xDD;
		chn1_sendbuf[1] = 0xCC;			
		chn1_sendbuf[212] = 0x0d;
		chn1_sendbuf[213] = 0x0a;
		channel_1.sending_sta = true;	/*👀 现在读取完了，该执行并列的发送了*/
	}
	channel_1.storage_read_idx++;	/* 不管发送，现将读取完成后，预定变量+1 */
}
static int BleSend_Chn1Data()
{
	int err;
	if(!channel_1.sending_sta)
	{
		return DATA_READING;
	}
	if(channel_1.sending_retry == true)
	{
		err = bt_nus_send(my_conn, chn1_sendbuf, 214);
		if(err)
		{
			//printk("retry defeat\n");
		}
		else
		{
			channel_1.sending_retry = false;
			memset(chn1_sendbuf,0,sizeof(chn1_sendbuf));
			//printk("retry ok\n");
		}
	}
	else
	{
		err = bt_nus_send(my_conn, chn1_sendbuf, 214);
		if(err)
		{
			channel_1.sending_retry = true;
			//printk("first send data unsuccess\n");		
		}
		else
		{
			channel_1.sending_sta = false;
			memset(chn1_sendbuf,0,sizeof(chn1_sendbuf));
			//printk("first data send ok\n");		
		}
	}
} 
static int readStorage_chn2Data()
{
	int rc;
	/*先判断是否处于发送过程，发送过程中不读数据，否则会丢包*/
	if(channel_2.sending_sta)
	{
		return DATA_SENDING;
	}
	/*在判断是否有存储数据，没有直接判断读取完成*/
	if((channel_2.storage_idx==0)&&(channel_2.storage_over==false))
	{
		channel_2.storage_read_idx = 0;
		channel_2.storage_read_ok = true;
		return DATA_NONE;
	}
	/* 最后判断是否读取完成，如果读取cnt >=20000或者大于=存储cnt，说明读取完成了 */
	if(channel_2.storage_over == true)
	{//数据超过20000条
		if(channel_2.storage_read_idx >= 20000)
		{
			channel_2.storage_over = false;			
			channel_2.storage_read_idx = 0;
			channel_2.storage_read_ok = true;
			if(channel_2.sending_cnt != 0) //如果之前读过，刚好读取够15条，同时也读完了。会自动配置好数组包头/包尾巴
			{
				channel_2.sending_cnt = 0;
				chn1_sendbuf[0] = 0xDD;
				chn1_sendbuf[1] = 0xCC;			
				chn1_sendbuf[212] = 0x0d;
				chn1_sendbuf[213] = 0x0a;					
				channel_2.sending_sta = true;
				return READ_OK;
			}	
		}
	}
	else
	{
		if(channel_2.storage_read_idx >= channel_2.storage_idx)
		{
			channel_2.storage_read_idx = 0;
			channel_2.storage_read_ok = true;
			if(channel_2.sending_cnt != 0)
			{
				channel_2.sending_cnt = 0;
				chn2_sendbuf[0] = 0xDD;
				chn2_sendbuf[1] = 0xCC;			
				chn2_sendbuf[212] = 0x0d;
				chn2_sendbuf[213] = 0x0a;			
				channel_2.sending_sta = true;		
				return READ_OK;
			}
		}
	}
	//从开始到读取完的执行逻辑
	//跳过csv表头
	if(channel_2.storage_read_idx == 0)
	{
		int header_size = sizeof(ch2_title) + sizeof(fifle_header);
		int position = header_size + (channel_2.storage_read_idx * record_size);
		rc = fs_seek(&chn2_file, position, FS_SEEK_SET);
		if (rc < 0) {
			printk("定位文件失败: %d\n", rc);
		}
	}
	//读取并解析				
	fs_read(&chn2_file, line, record_size);	
	int parsed = sscanf(line,  "%05d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",	&storage_idx,&channel_type, &year, &month, &day, &hour, &min, &temp, &hum, &klux);
	printk("parsed is %d:\n\r" ,parsed );
	if(parsed == 10)
	{
		sendbuf[2] = (year >> 8) & 0xff;
		sendbuf[3] = year & 0xff;
		sendbuf[4] = month;
		sendbuf[5] = day;
		sendbuf[6] = hour;
		sendbuf[7] = min;
		sendbuf[8] = 2;		/* 通道2👌 */
		sendbuf[9] = channel_type;
		if(channel_type == sht40)
		{
			sendbuf[10] = (temp >> 8) & 0xff;
			sendbuf[11] = temp & 0xff;
			sendbuf[12] = (hum >> 8) & 0xff;
			sendbuf[13] = hum & 0xff;
		}
		else
		{//除了温湿度，剩下全是光照
			sendbuf[10] = (klux >> 8) & 0xff;
			sendbuf[11] = klux & 0xff;
			sendbuf[12] = 0x00;
			sendbuf[13] = 0x00;
		}
		sendbuf[14] = (storage_idx >> 8) & 0xff;
		sendbuf[15] = storage_idx & 0xff;
	}
	memcpy(&chn2_sendbuf[2+channel_2.sending_cnt*14], &sendbuf[2], 14);
	channel_2.sending_cnt++;
	if(channel_2.sending_cnt >= 15)
	{
		channel_2.sending_cnt = 0;
		chn2_sendbuf[0] = 0xDD;
		chn2_sendbuf[1] = 0xCC;			
		chn2_sendbuf[212] = 0x0d;
		chn2_sendbuf[213] = 0x0a;
		channel_2.sending_sta = true;	/*👀 现在读取完了，该执行并列的发送了*/
	}
	channel_2.storage_read_idx++;	/* 不管发送，现将读取完成后，预定变量+1 */
}
static int BleSend_Chn2Data()
{
	int err;
	if(!channel_2.sending_sta)
	{
		return DATA_READING;
	}
	if(channel_2.sending_retry == true)
	{
		err = bt_nus_send(my_conn, chn2_sendbuf, 214);
		if(err)
		{
			//printk("retry defeat\n");
		}
		else
		{
			channel_2.sending_retry = false;
			memset(chn2_sendbuf,0,sizeof(chn2_sendbuf));
			//printk("retry ok\n");
		}
	}
	else
	{
		err = bt_nus_send(my_conn, chn2_sendbuf, 214);
		if(err)
		{
			channel_2.sending_retry = true;
			//printk("first send data unsuccess\n");		
		}
		else
		{
			channel_2.sending_sta = false;
			memset(chn2_sendbuf,0,sizeof(chn2_sendbuf));
			//printk("first data send ok\n");		
		}
	}
} 
void readStorage_SendData()
{
	/* 通道0的读取和发送都完成了 */
	if(!channel_0.storage_read_ok)//&&(channel_0.sending_sta==false))
	{
		readStorage_chn0Data();
		BleSend_Chn0Data();
	}
	else if(!channel_1.storage_read_ok)//&&(channel_1.sending_sta==false))
	{
		if(channel_0.sending_sta==false)
		{//等通道0的发送也完全执行完，在开始通道1的读/发
			readStorage_chn1Data();
			BleSend_Chn1Data();
		}
	}
	else if(!channel_2.storage_read_ok)//&&(channel_2.sending_sta==false))
	{
		if(channel_1.sending_sta==false)
		{
			readStorage_chn2Data();
			BleSend_Chn2Data();
		}
	}
	else
	{//通道0/1/2的读取状态ok都为真，确实都读完了
		if(channel_2.sending_sta==false)
		{//在确保通道2也发送完成了
			yk_tm.storage_read_sta = false;				//结束读取存储发送
			reback_order_Status("complete", 8);		//发送读取内部存储ok complete旗标
		}
	}
}
void stop_readStorage_SendSta()
{
	channel_0.storage_read_idx = 0;
	channel_0.storage_read_ok = false;
	channel_0.sending_cnt = 0;
	channel_0.sending_retry = false;
	channel_0.sending_sta = false;

	channel_1.storage_read_idx = 0;
	channel_1.storage_read_ok = false;
	channel_1.sending_cnt = 0;
	channel_1.sending_retry = false;
	channel_1.sending_sta = false;

	channel_2.storage_read_idx = 0;
	channel_2.storage_read_ok = false;
	channel_2.sending_cnt = 0;
	channel_2.sending_retry = false;
	channel_2.sending_sta = false;
	yk_tm.storage_read_sta = false;	
}































