/**
 * @file main.c
 * @author sok (you@domain.com)
 * @brief 
 * @version 0.0.1
 * @date 2025-10-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */
/*蓝牙基础头文件gap uuid*/
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/addr.h>
/*蓝牙gatt引用头文件*/
#include <zephyr/bluetooth/gatt.h>
/*蓝牙连接修改头文件*/
#include <zephyr/bluetooth/conn.h>
/*添加nus服务头文件*/
#include <bluetooth/services/nus.h>
/*添加DIS用到的协助头文件*/
#include <zephyr/sys/byteorder.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/settings/settings.h>
/*添加电池电量服务头文件*/
#include <zephyr/bluetooth/services/bas.h>
/*user define include*/
#include "main.h"
#include "sensor.h"
#include "user_gpio.h"
#include "user_rtc.h"
#include "lcd.h"
#include "user_storage.h"
#include "user_workQueue.h"
#include "bat_ssadc.h"
#include "ble_nus_order.h"
#include "button.h"

/* user define code verial ********************************************************************/
#define LOG_MODULE_NAME YKTM_LOG
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
struct bt_conn *my_conn = NULL;
uint16_t sensor_read_Tcnt = 0;
bool sensor_read_flag = false;		//允许读取传感器旗标，开机的时候会卡住，随后正常运行过程中超过<更新>间隔，置(真)一次
bool storage_flag = false;				//允许存储/发送旗标，运行过程中超过<采样>间隔，置为真。
uint8_t send_data_buf[28] = {0x55,0xaa};

extern uint8_t interval_compare;	//用于对比采集间隔计时

yongker_TM_initTypedef yk_tm={
	.warm_icon_sta = false,
	.bat_level = 0,
	.bt_sta = false,
	.rssi = 0,
	.storage_sta = false,
	.storage_read_sta = false,
	.charging_sta = false,
	.samp_interval = 1,			//采样时间默认1min，在rtc的1min中内同步增加bool flag，在线程中发送
	.display_chn = 0,
	.start_send_flag = false,
};
yongker_tm_channelDef   channel_0={
	.channel_type = sht40,
	.temp_type_is_C = true,
	.temp_celsius = -365,
	.temp_fahrenheit = -977,
	.humidity = 256,
	.h_hum = 600,
	.l_hum = 200,
	.h_temp = 500,
	.l_temp = 100,
	.h_lux = 3000,
	.l_lux = 100,
	.storage_idx = 0,
	.storage_over = false,
	.storage_read_idx = 0,
	.storage_read_ok = false,
	.sending_retry = false,
	.sending_sta = false,
	.sending_cnt = 0,
};
yongker_tm_channelDef   channel_1={
	.channel_type = nosensor,
	.h_hum = 600,
	.l_hum = 200,
	.h_temp = 500,
	.l_temp = 100,
	.h_lux = 3000,
	.l_lux = 100,	
	.storage_idx = 0,
	.storage_over = false,
	.storage_read_idx = 0,	
	.storage_read_ok = false,	
	.sending_retry = false,
	.sending_sta = false,	
	.sending_cnt = 0,
};
yongker_tm_channelDef   channel_2={
	.channel_type = nosensor,
	.h_hum = 600,
	.l_hum = 200,
	.h_temp = 500,
	.l_temp = 100,
	.h_lux = 3000,
	.l_lux = 100,	
	.storage_idx = 0,
	.storage_over = false,
	.storage_read_idx = 0,	
	.storage_read_ok = false,	
	.sending_retry = false,
	.sending_sta = false,	
	.sending_cnt = 0,
};

/* 更新连接参数部分代码* start********************************************************************/
static struct bt_gatt_exchange_params exchange_params;
static void exchange_func(struct bt_conn *conn, uint8_t att_err,  struct bt_gatt_exchange_params *params)
{
	LOG_INF("MTU exchange %s", att_err == 0 ? "successful" : "failed");
	if (!att_err) {
		uint16_t payload_mtu =
			bt_gatt_get_mtu(conn) - 3; // 3 bytes used for Attribute headers.
		LOG_INF("New MTU: %d bytes", payload_mtu);
	}
}
static void update_phy(struct bt_conn *conn)																														
{
	int err;
	const struct bt_conn_le_phy_param preferred_phy = {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
	};
	err = bt_conn_le_phy_update(conn, &preferred_phy);
	if (err) {
			LOG_ERR("bt_conn_le_phy_update() returned %d", err);
	}
};
static void update_data_length(struct bt_conn *conn)																								
{
	int err;
	struct bt_conn_le_data_len_param my_data_len = {
			.tx_max_len = BT_GAP_DATA_LEN_MAX,
			.tx_max_time = BT_GAP_DATA_TIME_MAX,
	};
	err = bt_conn_le_data_len_update(my_conn, &my_data_len);
	if (err) {
			LOG_ERR("data_len_update failed (err %d)", err);
	}
};
static void update_mtu(struct bt_conn *conn)																														
{
	int err;
	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		LOG_ERR("bt_gatt_exchange_mtu failed (err %d)", err);
	}
}
void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency,	 uint16_t timeout)	
{
	double connection_interval = interval * 1.25; // in ms
	uint16_t supervision_timeout = timeout * 10; // in ms
	LOG_INF("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms",
		connection_interval, latency, supervision_timeout);
}
void on_le_phy_updated(struct bt_conn *conn, struct bt_conn_le_phy_info *param)													
{
	if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_1M) {
		LOG_INF("PHY updated. New PHY: 1M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_2M) {
		LOG_INF("PHY updated. New PHY: 2M");
	} else if (param->tx_phy == BT_CONN_LE_TX_POWER_PHY_CODED_S8) {
		LOG_INF("PHY updated. New PHY: Long Range");
	}
}
void on_le_data_len_updated(struct bt_conn *conn, struct bt_conn_le_data_len_info *info)								
{
	uint16_t tx_len = info->tx_max_len;
	uint16_t tx_time = info->tx_max_time;
	uint16_t rx_len = info->rx_max_len;
	uint16_t rx_time = info->rx_max_time;
	LOG_INF("Data length updated. Length %d/%d bytes, time %d/%d us", tx_len, rx_len, tx_time,
		rx_time);
}
/* 更新连接参数部分代码* end**********************************************************************/

/* 蓝牙广播设置相关部分代码* start*****************************************************************/
static uint8_t bt_mac_add[6] = {0};
static void bt_get_device_address(void)
{
	/* 蓝牙规范5.3 MAC地址最高两个有效位必须等于1，nrf connect显示出来时已经补上了 */
	unsigned int device_addr_0 = NRF_FICR->DEVICEADDR[0];
	unsigned int device_addr_1 = NRF_FICR->DEVICEADDR[1];
	const uint8_t *part_0 = (const uint8_t *)&device_addr_0;
	const uint8_t *part_1 = (const uint8_t *)&device_addr_1;
	bt_mac_add[0] = part_1[1];
	bt_mac_add[1] = part_1[0];
	bt_mac_add[2] = part_0[3];
	bt_mac_add[3] = part_0[2];
	bt_mac_add[4] = part_0[1];
	bt_mac_add[5] = part_0[0];
	LOG_INF("Get mac address ok");
}
static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
	// 静态修改广播
	(BT_LE_ADV_OPT_CONNECTABLE |
	BT_LE_ADV_OPT_USE_IDENTITY),
	1600,			//5000ms x 0.625
	1600,		  //5000ms x 0.625
	NULL);
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)), // 可发现，并且不支持经典蓝牙
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),					// 广播名称
	BT_DATA(BT_DATA_MANUFACTURER_DATA, bt_mac_add, 6),										// MAC 地址，ios gatt读不到，需要从广播获取
};
static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
	BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)),
};
void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection error %d", err);
		return;
	}
	LOG_INF("Connected");
	my_conn = bt_conn_ref(conn);
	struct bt_conn_info info;							//连接参数结构体
	err = bt_conn_get_info(conn, &info);	//获取连接参数信息
	if (err) {
		LOG_ERR("bt_conn_get_info() returned %d", err);
		return;
	}
	yk_tm.bt_sta = true;
	yk_tm.rssi = 4;	
	refresh_flag.ble_sta = true;	
	k_msleep(100);
	update_phy(my_conn);
	update_data_length(my_conn);
	update_mtu(my_conn);
}
void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected. Reason %d", reason);
	if (my_conn) {
		bt_conn_unref(my_conn);
		my_conn = NULL;
	}
	yk_tm.bt_sta = false;
	refresh_flag.ble_sta = true;
	yk_tm.start_send_flag = false;
	stop_readStorage_SendSta();
}
void on_recycled(void)
{
}
struct bt_conn_cb connection_callbacks = {
		.connected = on_connected,
		.disconnected = on_disconnected,
		.recycled = on_recycled,
    .le_param_updated   = on_le_param_updated,
    .le_phy_updated     = on_le_phy_updated,
    .le_data_len_updated    = on_le_data_len_updated,		
};
/* 蓝牙广播设置相关部分代码* end--*****************************************************************/

/* 读取连接状态下rssi数值 * start*****************************************************************/
static void read_conn_rssi(uint16_t handle, int8_t *rssi)
{
	struct net_buf *buf, *rsp = NULL;
	struct bt_hci_cp_read_rssi *cp;
	struct bt_hci_rp_read_rssi *rp;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_READ_RSSI, sizeof(*cp));
	if (!buf) {
			printk("Unable to allocate command buffer\n");
			return;
	}
	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = sys_cpu_to_le16(handle);

	err = bt_hci_cmd_send_sync(BT_HCI_OP_READ_RSSI, buf, &rsp);
	if (err) {
			uint8_t reason = rsp ?
					((struct bt_hci_rp_read_rssi *)rsp->data)->status : 0;
			printk("Read RSSI err: %d reason 0x%02x\n", err, reason);
			return;
	}
	rp = (void *)rsp->data;
	*rssi = rp->rssi;
	net_buf_unref(rsp);	
}
static int8_t yk_rssi= 0xff;
static uint16_t read_rssi_cnt;
void read_yonkerTM_BleRssi()
{
	uint16_t conn_handle;
	read_rssi_cnt++;
	if(read_rssi_cnt >= RSSI_READ_PERIOD)
	{
		read_rssi_cnt = 0;
		if(yk_tm.bt_sta)
		{
			int rc = bt_hci_get_conn_handle(my_conn, &conn_handle);
			if (rc) {
					printk("fail getting conn handle\n");
					return;
			}	
			read_conn_rssi(conn_handle, &yk_rssi);
			if(yk_rssi < -75)
				yk_tm.rssi = 1;
			if((-75 <= yk_rssi)&&(yk_rssi < -65))
				yk_tm.rssi = 2;
			if((-65 <= yk_rssi)&&(yk_rssi < -55))			
				yk_tm.rssi = 3;
			if(-55 <= yk_rssi)
				yk_tm.rssi = 4;
			refresh_flag.ble_sta = true;
			//printk("RSSI in connection: %d dBm\n", yk_rssi);
		}
	}
}
/* 读取连接状态下rssi数值 * end*******************************************************************/

/* 串口透传NUS 服务部分代码* start****************************************************************/
bool ble_order_is = false;
uint8_t order_buffer[20];
uint16_t length = 0;
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
	//发送存储数据时，停止一些接受命令的操作
	if(yk_tm.storage_read_sta == false)
	{
		length = len;
		memcpy(&order_buffer, data, length);
		ble_order_is = true;
	}
};
void yk_tm_order_cb()
{
	if(ble_order_is)
	{
		ble_order_is = false;
		che_sync_time_order(order_buffer, length);
		che_SetAram_threshold_order(order_buffer, length);
		che_collect_data_order(order_buffer, length);
		che_open_storage_order(order_buffer, length);
		che_close_storage_order(order_buffer, length);
		che_get_storage_data_order(order_buffer, length);
		che_systemoff_order(order_buffer, length);
		che_DevStatus_order(order_buffer, length);
		che_clearStorage(order_buffer, length);
	}
}
static void bt_send_enabled_cb(enum bt_nus_send_status status) {

};
static void bt_sent_cb(struct bt_conn *conn) {

};
static struct bt_nus_cb nus_cb = {
		.received = bt_receive_cb,
		.send_enabled = bt_send_enabled_cb,
		.sent = bt_sent_cb,
};
void reback_order_Status(uint8_t *code, uint16_t len)
{
	int err;
	if(yk_tm.bt_sta == true)
	{
		err = bt_nus_send(my_conn, code, len);
		if(err)
		{
			LOG_WRN("order reback error is:%d\n",err);
		}
		else
		{
			LOG_INF("order send ok \n");
		}
	}
}
static void anaylse_channelType(yongker_tm_channelDef *chn, uint8_t *data)
{
	if(chn->channel_type == nosensor)
	{
		data[0] = 0x00;
		data[1] = 0x00; 
		data[2] = 0x00; 
		data[3] = 0x00; 
	}
	else if(chn->channel_type == sht40)
	{
		data[0] = (chn->temp_celsius >> 8) & 0xff;
		data[1] = chn->temp_celsius & 0xff;
		data[2] = (chn->humidity >> 8) & 0xff;
		data[3] = chn->humidity & 0xff;
	}
	else if((chn->channel_type == bh1750)||(chn->channel_type == max44009))
	{
		data[0] = (chn->klux >> 8) & 0xff;
		data[1] = chn->klux & 0xff;
		data[2] = 0x00; 
		data[3] = 0x00; 		
	}
}
void send_yktm_Data()
{
	if(interval_compare >= yk_tm.samp_interval)
	{
		interval_compare = 0;
		storage_flag = true;
		send_data_buf[0] = 0x55; 
		send_data_buf[1] = 0xaa;
		send_data_buf[2] = (timeInfo_stamp.year >> 8) & 0xff;
		send_data_buf[3] = timeInfo_stamp.year & 0xff;
		send_data_buf[4] = timeInfo_stamp.month;
		send_data_buf[5] = timeInfo_stamp.day;
		send_data_buf[6] = timeInfo_stamp.hour;
		send_data_buf[7] = timeInfo_stamp.min;
		//channel 0
		send_data_buf[8] = 0x00;
		send_data_buf[9] = sensorType_is(channel_0);
		anaylse_channelType(&channel_0,&send_data_buf[10]);
		//channel 1
		send_data_buf[14] = 0x01;
		send_data_buf[15] = sensorType_is(channel_1);
		anaylse_channelType(&channel_1,&send_data_buf[16]);
		//channel 2
		send_data_buf[20] = 0x02;
		send_data_buf[21] = sensorType_is(channel_2);
		anaylse_channelType(&channel_2,&send_data_buf[22]);
		//包尾
		send_data_buf[26] = 0x66;
		send_data_buf[27] = 0xbb;
		if((yk_tm.bt_sta == true)&&(yk_tm.start_send_flag))
		{
			reback_order_Status(send_data_buf, 28);
		}
	}
}
/* 串口透传NUS 服务部分代码* end******************************************************************/

/* 设备信息--- 服务部分代码* start****************************************************************/
static int settings_runtime_load(void)
{
#if defined(CONFIG_BT_DIS_SETTINGS)
	settings_runtime_set("bt/dis/model",
			     "YK-TM",	//设备名称
			     sizeof("YK-TM"));
	settings_runtime_set("bt/dis/manuf",
			     "徐州永康电子",	//制造商名称
			     sizeof("徐州永康电子"));
#if defined(CONFIG_BT_DIS_SERIAL_NUMBER)
	settings_runtime_set("bt/dis/serial",
			     CONFIG_BT_DIS_SERIAL_NUMBER_STR,
			     sizeof(CONFIG_BT_DIS_SERIAL_NUMBER_STR));
#endif
#if defined(CONFIG_BT_DIS_SW_REV)
	settings_runtime_set("bt/dis/sw",
			     CONFIG_BT_DIS_SW_REV_STR,
			     sizeof(CONFIG_BT_DIS_SW_REV_STR));
#endif
#if defined(CONFIG_BT_DIS_FW_REV)
	settings_runtime_set("bt/dis/fw",
			    	"V0.1.6",
			     sizeof("V0.1.6"));
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
	settings_runtime_set("bt/dis/hw",
			     CONFIG_BT_DIS_HW_REV_STR,
			     sizeof(CONFIG_BT_DIS_HW_REV_STR));
#endif
#endif
	return 0;
}
/* 设备信息--- 服务部分代码* end******************************************************************/

/* 用户逻辑功能*----------- start****************************************************************/
void dev_intoSleep_front()
{
	disable_iic_sensor();
	close_lcd_display();
	storage_clear_allFile();

}
void dev_intoSleep(bool lowPow)
{
	disable_qspi_mx25r32();
	nrf_gpio_pin_clear(PCA_RESET); 
  nrf_gpio_pin_clear(CH1_EN); 
  nrf_gpio_pin_clear(CH2_EN);     
  nrf_gpio_pin_clear(LCD_CS);
  nrf_gpio_pin_clear(LCD_WR);
  nrf_gpio_pin_clear(LCD_DATA);  
  nrf_gpio_pin_clear(BQ_CE);

	nrf_gpio_cfg_output(PCA_SDA);
	nrf_gpio_cfg_output(PCA_SCL);
  nrf_gpio_pin_clear(PCA_SDA);  
  nrf_gpio_pin_clear(PCA_SCL);
	
	if(lowPow == false)
	{
  	nrf_gpio_cfg_input(BUTTON, NRF_GPIO_PIN_NOPULL);
  	nrf_gpio_cfg_sense_set(BUTTON, NRF_GPIO_PIN_SENSE_LOW); 	
	}
  	nrf_gpio_cfg_input(VCHECK, NRF_GPIO_PIN_NOPULL);
  	nrf_gpio_cfg_sense_set(VCHECK, NRF_GPIO_PIN_SENSE_HIGH); 

	sys_poweroff();
}

/* 用户逻辑功能*----------- end******************************************************************/
uint8_t senser_is_num = 0;
int main(void)
{
	int err;
	/* user logic function start ---*/
	my_rtc_init();
	yonker_tm_gpio_init();
	lcd_ht1621_init();
	Fatfs_storage_init();
	ssadc_init();
	display_lcd_init();
	/* 注册连接事件回调 */
	bt_conn_cb_register(&connection_callbacks);	
	/* 先使能协议栈 */
	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	/* 系统存储ble */
	if (IS_ENABLED(CONFIG_BT_SETTINGS))
	{
		settings_load();
	}	
	/* 开启协议栈后，可获取本地mac */
	bt_get_device_address();	
	/* 更新DIS服务 */
	settings_runtime_load();
	/* 使能NUS服务 */
	err = bt_nus_init(&nus_cb);
	if (err)
	{
		LOG_ERR("Failed to initialize UART service (err: %d)", err);
		return;
	}	
 /* 开启广播 */
	err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err)
	{
		LOG_ERR("Advertising failed to start (err %d)\n", err);
		return;
	}
	yongker_tm_work_queue_init();
	button_gpiote_init();
	vcheck_gpiote_init();
	sensor_read_flag = true;
	myVcheck.toogle_flag = true;
}
/**
 * @brief： 消费者线程
 * @priority:  8
 * @description: 每过10sec检测一次当前通道，更新数据并刷新屏幕
 * @contiun:     根据采样间隔，周期性的读取数据
 * @contiun:     每当检测到切换界面按键动作，检测一次通道，切换更新显示。
 * 
 */
#define STACKSIZE	4096	
#define CUSTOMER_THREAD_PRIORITY 8
uint16_t number_cnt_test = 0;
static void customer_thread()
{
	while(1)
	{
		k_msleep(10);				
		sensor_read_Tcnt++;		
		if(sensor_read_Tcnt >= CHECK_CHN_SENSOR)
		{
			sensor_read_Tcnt = 0;
			sensor_read_flag = true;  
		}
		if(sensor_read_flag)
		{
			nrf_gpio_pin_set(CH1_EN); 
			nrf_gpio_pin_set(CH2_EN);  
			k_msleep(10);			 			
			/** @brief：读取数据，并且更新数值显示 */
			sensor_read_flag = false;
			channel_0.channel_type = CheckChn_Sensor_is(0);
			read_sensor_data(&channel_0);		
			channel_1.channel_type = CheckChn_Sensor_is(1);
			read_sensor_data(&channel_1);
			channel_2.channel_type = CheckChn_Sensor_is(2);
			read_sensor_data(&channel_2);
			refresh_flag.channel_data_sta  = true;
			/** @brief: 判断该通道是否有超过设定的阈值，有责电量报警图标 */
			switch(yk_tm.display_chn)
			{
				case 0:
				judege_sensor_warming(&channel_0);
				break;
				case 1:
				judege_sensor_warming(&channel_1);
				break;
				case 2:
				judege_sensor_warming(&channel_2);
				break;
			}
			refresh_flag.channel_warming_sta = true;
			// nrf_gpio_pin_clear(CH1_EN); 
 		  // nrf_gpio_pin_clear(CH2_EN);     
		}
	}
}
K_THREAD_DEFINE(customer_ID, STACKSIZE, customer_thread, NULL, NULL, NULL, CUSTOMER_THREAD_PRIORITY, 0,	0);
/**
 * @brief： 存储与发送存储线程
 * @priority:  2
 * @description: 1. 触发存储旗标storage_sta为真时，在此线程执行存储，优先级要高于现实和数据读取，因为存储绝对不能错一行。
 * @contiun:     2. 触发蓝牙发送存储数据任务时，再次线程中执行，同时也会主动关闭存储功能，优先此线程中只有蓝牙发送功能。
 */
#define STORAGE_SIZE	2048	
#define STORAGE_THREAD_PRIORITY 1
static void storage_thread()
{
	while(1)
	{
		k_msleep(1);
		if(yk_tm.storage_read_sta == true)
		{
			readStorage_SendData();
		}		
		if((storage_flag == true)&&(yk_tm.storage_sta == true))
		{
			storage_flag = false;
			storageCutIn_chn0_data();
			storageCutIn_chn1_data();
			storageCutIn_chn2_data();
		}
	}
}
K_THREAD_DEFINE(storage_ID, STORAGE_SIZE, storage_thread, NULL, NULL, NULL, STORAGE_THREAD_PRIORITY, 0,	0);


































