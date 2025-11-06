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
#include "user_storage.h"


/* user define code verial ********************************************************************/
#define LOG_MODULE_NAME YKTM_LOG
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
struct bt_conn *my_conn = NULL;

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
	ADC_INTERVAL_MIN,
	ADC_INTERVAL_MAX,
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
	struct bt_conn_info info;				//连接参数结构体
	err = bt_conn_get_info(conn, &info);	//获取连接参数信息
	if (err) {
		LOG_ERR("bt_conn_get_info() returned %d", err);
		return;
	}
}
void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected. Reason %d", reason);
	if (my_conn) {
		bt_conn_unref(my_conn);
		my_conn = NULL;
	}
}
void on_recycled(void)
{
}
struct bt_conn_cb connection_callbacks = {
		.connected = on_connected,
		.disconnected = on_disconnected,
		.recycled = on_recycled,
};
/* 蓝牙广播设置相关部分代码* end--*****************************************************************/

/* 串口透传NUS 服务部分代码* start****************************************************************/
static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{

};
static void bt_send_enabled_cb(enum bt_nus_send_status status) {

};
static void bt_sent_cb(struct bt_conn *conn) {

};
static struct bt_nus_cb nus_cb = {
		.received = bt_receive_cb,
		.send_enabled = bt_send_enabled_cb,
		.sent = bt_sent_cb,
};
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
			     CONFIG_BT_DIS_FW_REV_STR,
			     sizeof(CONFIG_BT_DIS_FW_REV_STR));
#endif
#if defined(CONFIG_BT_DIS_HW_REV)
	settings_runtime_set("bt/dis/hw",
			     CONFIG_BT_DIS_HW_REV_STR,
			     sizeof(CONFIG_BT_DIS_HW_REV_STR));
#endif
#endif
	return 0;
}
/* 设备信息--- 服务部分代码* start****************************************************************/


int main(void)
{
	int err;
	/* user logic function start ---*/
	my_rtc_init();
	yonker_tm_gpio_init();
	Fatfs_storage_init();
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
	bt_bas_set_battery_level(0);
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
}


// #define USER_THREAD_SIZE	8192	
// #define CUSTOMER_THREAD_PRIORITY 2
// static void customer_func()
// {
// 	k_msleep(50);
	
// }
// K_THREAD_DEFINE(customer, USER_THREAD_SIZE, customer_func, NULL, NULL, NULL, CUSTOMER_THREAD_PRIORITY, 0,	0);













