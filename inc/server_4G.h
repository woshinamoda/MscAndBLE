/**
 * @file server_4G.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-01-05
 * 
 * @copyright Copyright (c) 2026
 * ME：移动设备
 * MS：移动台
 * MT：移动终端
 * APN：接入点名称。指的是一种网络技术，通过手机上网时必须配置的一个参数，他决定手机通过哪种方式来访问网络
 * PDN：公用数据网络
 * PDP：分组数据协议->PDP上下文包括APN，QOS，PDP类型，PDP地址
 * PPP：点到点协议
 * PSM power saving mode 节能模式
 * 是否注册GPRS：
 * **********************************************
 * 流程AT通信测试  ATE0关闭回西安
 * 然后开启全功能CFUN
 * CGDCONT 设置APN
 * CGATT 附着网络
 * QiACT 激活上下文
 * QIOPEN 建立连接
 ************************************************
 核心指令细节
 
 ************************************************
 休眠模式，
 设置at+cfun=0
 设置at+qsclk=1
************************************************
 * ATE0       关闭指令的回显
 * [后面连接上执行？] AT+CSQ     查看信号强度         ｜ 返回rssi和ber(信道误码率，百分比格式，99未知或不可测)
 * ATI        版本号
 * AT+CIMI    查看SIM卡状态                        | 返回USIM卡的国际移动用户识别吗IMSI，返回前3个460代表是中国
 * AT+CFUN    设置功能模式                         ｜ 0最小功能，低功耗状态   1全功能，正常功耗   4飞行模式
 * AT+QICLOSE=0   每次上电先断开socket
 * AT+CGDCONT=？  先查看一下PDP配置
 * AT+CGATT=1     网络附着状态检查
 * 
 */
#ifndef _SERVER_4G_H
#define _SERVER_4G_H

#include "main.h"

#define EC801_INIT_PERIOD   1000        //上电初始化的时间间隔

typedef struct 
{
  bool    init_is_ok;                   //默认false，初始化完成后为true，这样就有usim卡的ccid了
  uint8_t delay_secCnt;                 //所有设备延时状态
  /**
   * @brief 设备当前所处状态 0：停止状态，串口和4G standby
   * @brief               1: 刚上电/复位后在初始化。对应的init 状态
   * @brief               2: 正常5min发送数据，对应 send 状态
   */
  uint8_t dev_num;
  /**
   * @brief 移远设备初始化所在的位置
   * @see 0 : 发送ATE0
   * @see 1 ：发送AT+CPIN
   * 略，具体流程参考手册
   */
  uint8_t init_num;
  uint8_t init_cnt;               //指令发送次数
  uint8_t init_ATE0_okCnt;        //ATE0成功次数，这个是第一条，务必要求保障，同时验证是否已经正常开机了
  /**
   * @brief 移远间隔5min发送数据
   * @see 0: 发送ATE0(保险起见) 拉高dtr
   * @see 1: 开启cfun全功能模式
   * @see 2: 检查驻网               /需等待，连续5次不成功，跳到6
   * @see 3: 关闭socket
   * @see 4: 创建socket 连接tcp     /连续3次不成功，跳到6
   * @see 5: 读取rssi
   * @see 6: 发送数据               /数据发送成不成功，都跳到6，不做冲发送判断
   * @see 7: 关闭socket
   * @see 8: 关闭射频，拉低dtr，等待下1个5min发送数据
   */
  uint8_t send_num;
  uint8_t send_cnt;               //指令发送次数
  uint8_t send_ATE0_okCnt;        //发送数据时，发送ATE0成功次数
  uint8_t send_ing_sta;           //0:默认状态，发送完指令后自动转1->/1:发送QISEND/2:收到>，发送122bytes数据。 3:等待收到SEND OK
}Network_codeInit_Def;
extern Network_codeInit_Def ec801;


void uart_4Gnetwork_init();
void uart_tx_order(uint8_t *code, uint16_t length);
void uart_rx_data_cb(uint8_t *code, uint16_t length);
void EC801_interval_TimerCb();
void disable_uart0();




















































#endif










