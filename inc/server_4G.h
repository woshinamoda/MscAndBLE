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
 * AT+CREG    查看ME是否网络注册
 * AT+CFUN    设置功能模式                         ｜ 0最小功能，低功耗状态   1全功能，正常功耗   4飞行模式
 * AT+QICLOSE=0   每次上电先断开socket
 * AT+CGDCONT=？  先查看一下PDP配置
 * AT+CGATT=1     网络附着状态检查
 * 
 * 
 */
#ifndef _SERVER_4G_H
#define _SERVER_4G_H






















































#endif










