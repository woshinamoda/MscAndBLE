#include <nrfx_uarte.h>
#include <hal/nrf_gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include "server_4G.h"
#include "lcd.h"

#define LOG_PHERAL_UART uart0
LOG_MODULE_REGISTER(LOG_PHERIAL_UART);
uint8_t usim_ccid[20] = {0};      //sim卡唯一识别号
uint8_t usim_rssi = 0;            //信号强度 0 - 4
uint8_t ec801_send_buf[122] = {0x55, 0xaa};
Network_codeInit_Def ec801={
  .delay_secCnt = 0,
  .dev_num = 0,
  .init_is_ok = false,
  .init_cnt = 0,
  .init_num = 0,
  .init_ATE0_okCnt = 0,
};
/** 19页 psm模式设置
 * @brief 4G 初始化步骤
还是和ec801的应用指导测试
  ----  初始化  ----
1. ATEO 一直发，直到检测到数据回显ok
2. AT+CPIN?，看下能不能读到卡状态，20秒内没有识别到直接给模块断电重启， 5秒查一次，查询5次（）
3. AT+QCCID 也是读取1秒1次，读5次没有重启
4. 

 */

 #define   SERVER_LEN     57
 //char AT_OrderBuf[56] = {"AT+QIOPEN=1,0,\"TCP\",\"10154cayu7562.vicp.fun\",14204,0,1\r\n"};
 char AT_OrderBuf[57] = {"AT+QIOPEN=1,0,\"TCP\",\"lianrongdev.yonkercare.cn\",9086,0,1\r\n"};

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
#define BUF_SIZE 256
#define RX_INAVTIVE_TIMEOUT_MS     50000//串口数据长度很小，只检测10ms超时
uint8_t tx_buf[BUF_SIZE];
uint8_t rx_buf[BUF_SIZE];

/** 静态功能函数预声明 */
static void NetWork_init_config();
static void NetWork_send_data();
static void NetWork_init_config_CB_hd(uint8_t *code , uint16_t length);
static void NetWork_send_data_CB_hd(uint8_t *code , uint16_t length);
static void log_raw_string(const uint8_t *data, size_t len)
{
    char buf[256];
    // int pos = 0;
    
    // for (size_t i = 0; i < len; i++) {
    //     if (data[i] == '\r') {
    //         pos += snprintk(buf + pos, sizeof(buf) - pos, "\\r");
    //     } else if (data[i] == '\n') {
    //         pos += snprintk(buf + pos, sizeof(buf) - pos, "\\n");
    //     } else if (data[i] >= 32 && data[i] < 127) {
    //         buf[pos++] = data[i];
    //     } else {
    //         pos += snprintk(buf + pos, sizeof(buf) - pos, ".");
    //     }
    // }
    
    // LOG_INF("Raw: %s", buf);
}

static void power_4G_pinInit()
{
  k_msleep(10);  
  nrf_gpio_cfg_output(MODULE_EN);  
  nrf_gpio_cfg_output(DTR_4G);      //在睡眠模式下，拉低DTR唤醒设备，拉高dtr进入低功耗
  nrf_gpio_pin_clear(MODULE_EN);   
  nrf_gpio_pin_set(DTR_4G);         //DTR拉高是唤醒，可以输入at指令
  k_msleep(100);
  nrf_gpio_pin_set(MODULE_EN); 
  k_msleep(100);  
  ec801.dev_num = 1; //开始初始化状态
}
/** 串口回调事件 */
static void uart_callback(const struct device *dev,
                          struct uart_event *evt,
                          void *user_data)
{
  struct device *uart = user_data;
  uint16_t data_length;
  int err;
  switch(evt->type){
    case UART_TX_DONE:
      //LOG_INF("tx sent %d bytes\n", evt->data.tx.len);
      break;
    case UART_TX_ABORTED: //主动打断发送
      //LOG_INF("tx aborted\n");
      break;
    case UART_RX_RDY:
      //LOG_INF("receiced data %d bytes\n", evt->data.rx.len);
      uint8_t *p = &(evt->data.rx.buf[evt->data.rx.offset]);
      data_length = evt->data.rx.len;
      if(data_length < BUF_SIZE)
      {
        memcpy(rx_buf,p,data_length);
        uart_rx_data_cb(rx_buf, data_length);
      }
      break;  
    case UART_RX_BUF_REQUEST:
    {
      break;
    }
    case UART_RX_BUF_RELEASED:
      break;
    case UART_RX_DISABLED:
      uart_rx_enable(uart_dev, rx_buf, BUF_SIZE, RX_INAVTIVE_TIMEOUT_MS);
      break;
    case UART_RX_STOPPED:
      break;
  }
}
void uart_4Gnetwork_init()
{
  int err;
  power_4G_pinInit();
  if(!device_is_ready(uart_dev)){
    LOG_ERR("device %s is not readyl exiting\n", uart_dev->name);
  }
  err = uart_callback_set(uart_dev, uart_callback, (void*)uart_dev);
  if(err){
    LOG_ERR("Failed to set callback");
  }
  err = uart_rx_enable(uart_dev, rx_buf, BUF_SIZE, RX_INAVTIVE_TIMEOUT_MS);
  if(err){
    LOG_ERR("uart rx enable error is: %d\n", err);
  }  
}
void uart_tx_order(uint8_t *code, uint16_t length)
{
 int err;
  err = uart_tx(uart_dev, code, length, -1);
  if(err){
    LOG_ERR("uart send error is: %d\n", err);
  }
}
void uart_rx_data_cb(uint8_t *code, uint16_t length)
{
  if(ec801.dev_num == 1)
  {
    NetWork_init_config_CB_hd(code, length);
  }
  else if(ec801.dev_num == 2)
  {
    NetWork_send_data_CB_hd(code, length);
  }
}
void EC801_interval_TimerCb()
{
  if(ec801.dev_num == 1)
  {
    NetWork_init_config();
  }
  else if(ec801.dev_num == 2)
  {
    NetWork_send_data();
  }  
}
void disable_uart0()
{
	int rc;
	rc = pm_device_action_run(uart_dev, PM_DEVICE_ACTION_SUSPEND);
}

/** 4G版本下发送命令和回调解析 **/
/*- 设备上电初始化 -*/
static void NetWork_init_config_CB_hd(uint8_t *code , uint16_t length)
{  
  // printk("init state : %d\n",ec801.init_num); 
  // printk("init length : %d\n",length);     
  log_raw_string(code, length);
  if(ec801.init_num == 0) /*检测ATE0，必须6bytes，这个是基础，不然后面的串口全乱了 */
  {
    if(length == 6)
    {
      if(memcmp("\r\nOK\r\n" , &code[0], 6) == 0)
      {
        ec801.init_ATE0_okCnt++;
        if(ec801.init_ATE0_okCnt >= 3){
          ec801.init_ATE0_okCnt = 0;
          ec801.init_num = 1; //ATE0成功发送，并得到正确返回值
        }
      }
      else{
        ec801.init_ATE0_okCnt = 0;
      }
    }
  }
  else if(ec801.init_num == 1)/*检测到卡，准备读取ccid */
  {
    if(length >= 14) //只对比前14bytes，有可能出现16+6 +CPIN: READY  OK
    {
      if(memcmp("\r\n+CPIN: READY" , &code[0], 14) == 0)
      {
        ec801.init_num = 2; //发送USIM卡识别成功，准备读取CCID
        ec801.init_cnt = 0;
      }
    }
  }
  else if(ec801.init_num == 2)/*读取到了ccid，检查网络情况*/
  {
    if(length >= 32) //只对比前32bytes，+QCCID: 89860813102370887645 有可能出现32+6，后面还有回撤，换行ok（6bytes）
    {
      if(memcmp("\r\n+QCCID: " , &code[0], 10) == 0)
      {
        memcpy(&usim_ccid[0], &code[10], 20);
        for(uint8_t i=0; i<20; i++)
        {
          usim_ccid[i] = usim_ccid[i] - 0x30;
        }
        ec801.init_num = 3; //读取设备ccid成功，准备检测是否网络附着成功
        ec801.init_cnt = 0;
      }
    }  
  }
  else if(ec801.init_num == 3)/*检查网络附着*/
  {
    if(length >= 13) //只对比前13， +GATT：1，有可能出现13+6，后面还有回撤换行ok 6bytes
    {
      if(memcmp("\r\n+CGATT: 1" , &code[0], 11) == 0)
      {
        ec801.init_num = 4; //检查网络附着成功,gatt=1,然后关闭socket，准备连接
        ec801.init_cnt = 0;
        ec801.delay_secCnt = 0; //时间重置
      }     
    }  
  }
  else if(ec801.init_num == 4)/*关闭socket，只有6bytes，也可以重复发，所以必须检查6bytes*/
  {
    if(length==6)
    {
      if(memcmp("OK" , &code[2], 2) == 0)
      {
        ec801.init_num = 5; //确实断开socket了，连接服务器
        ec801.init_cnt = 0;
        ec801.delay_secCnt = 20;  //进入网络连接先主动尝试连一次服务器
      }
    }
  }
  else if(ec801.init_num == 5)/*创建socket，连接TCP服务器是否成功？*/
  {
    //printk("link status num : %d\n", length); 
    if(length>=16)    //会返回各种情况 11 3 16 18 bytes都有可能
    {
      if(memcmp("\r\n+QIOPEN: 0,0\r\n" , &code[0], 16) == 0)
      {
        ec801.init_num = 6; //确实连接成功
        ec801.init_cnt = 0; 
      }
    }
  }
  else if(ec801.init_num == 6)/*正确读取天线信号强度*/
  {
    uint8_t ten, one, level;
    if((code[3]==0x43)&&(code[4]==0x53)&&(code[5]==0x51)&&(length==21))
    //有可能收到15+6
    {
      ten = code[8] - 0x30;
      one = code[9] - 0x30;
      level = ten * 10 + one;
      if(level == 99){
        usim_rssi = 0; //断网
      }
      if((31<=level)&&(level<99)){
        usim_rssi = 4;
      }
      if((2<=level)&&(level<30)){
        usim_rssi = 3;
      }  
      if(level == 1){
        usim_rssi = 2;
      }
      if(level == 0){
        usim_rssi = 1;
      }
      if(usim_rssi == 0)
      {
        yk_tm.rssi = 0;
        yk_tm.bt_sta = false;
      }
      else
      {
        yk_tm.rssi = usim_rssi;
        yk_tm.bt_sta = true;
      }
      refresh_flag.ble_sta = true;	
      ec801.init_num = 7; //信号强度读取完毕
      ec801.init_cnt = 0;       
    }
  } 
  else if(ec801.init_num == 7)/*断开socket*/
  {
    if(length==6)
    {
      if(memcmp("OK" , &code[2], 2) == 0)
      {
        ec801.init_num = 8; //确实第二次socket断开成功了，开始开启增强休眠模式
        ec801.init_cnt = 0;
        //printk("ec801 init is 7 \n");
      }
    }
  } 
  else if(ec801.init_num == 8)/*是否成功开启增强休眠模式*/
  {
    if(length==6)
    {
      if(memcmp("OK" , &code[2], 2) == 0)
      {
        ec801.init_num = 9; //关闭完socket，成功开启增强休眠后，准备设置最小功能状态
        ec801.init_cnt = 0;


        ec801.dev_num = 0;  //停止数据发送了
        ec801.init_num = 0; //初始化完成了
        ec801.init_cnt = 0;
        nrf_gpio_pin_clear(DTR_4G);   //至此初始化完全执行完毕，拉低dtr，进入低功耗睡眠
        ec801.init_is_ok = true;        
      }
    }
  } 

}
static void NetWork_init_config()
{
  if(ec801.init_num == 0)
  {
    uart_tx_order("ATE0\r\n",6);
  }
  else if(ec801.init_num == 1) //检查卡状态
  {
    ec801.init_cnt++;
    if(ec801.init_cnt >= 10)
    {
      ec801.init_cnt = 0;
      ec801.init_num = 0; 
    }
    uart_tx_order("AT+CPIN?\r\n",10);
  }
  else if(ec801.init_num == 2) //读取卡ccid
  {
    ec801.init_cnt++;
    if(ec801.init_cnt >= 10)
    {
      ec801.init_cnt = 0;
      ec801.init_num = 0; 
    }
    uart_tx_order("AT+QCCID\r\n",10);
  }
  else if(ec801.init_num == 3) //检查网络附着
  {
    ec801.delay_secCnt++;
    if(ec801.delay_secCnt >= 3)
    {
      ec801.delay_secCnt = 0;
      ec801.init_cnt++;
      if(ec801.init_cnt >= 15)
      {//网络检查需要15*3sec时间
        ec801.init_cnt = 0;
        ec801.init_num = 0; 
        //都超过45秒了，还是检测不到网络附着，不仅结束发送，同时宣布4G连接断开
        yk_tm.rssi = 0;
        yk_tm.bt_sta = false;
        refresh_flag.ble_sta = true;	
      }
      uart_tx_order("AT+CGATT?\r\n",11);
    }
  }
  else if(ec801.init_num == 4) //关闭socket  
  {
    ec801.init_cnt++;
    if(ec801.init_cnt >= 5)
    {
      ec801.init_cnt = 0;
      ec801.init_num = 3;  //关闭socket都执行不了，继续回去检查网络状态
    }
    uart_tx_order("AT+QICLOSE=0\r\n",14);
  }
  else if(ec801.init_num == 5) //创建socket连接设备
  {
    ec801.delay_secCnt++;
    if(ec801.delay_secCnt >= 10)
    {
      ec801.delay_secCnt = 0;
      uart_tx_order(AT_OrderBuf,SERVER_LEN);  //每过10sec连接一次服务器
      ec801.init_cnt++;
      if(ec801.init_cnt >= 2)
      {
        ec801.init_cnt = 0;
        ec801.init_num = 3;  //2次服务器都连不上，回到网络检查，没问题自然会断开socket重新尝试
      }
    }
  }
  else if(ec801.init_num == 6) //读取rssi
  {
    ec801.init_cnt++;
    if(ec801.init_cnt >= 5)
    {
      ec801.init_cnt = 0;
      ec801.init_num = 3;  //信号强度都测不到，说明网络差，继续回去检查网络状态
    }
    uart_tx_order("AT+CSQ\r\n",8); 
  }
  else if(ec801.init_num == 7) //关闭socket
  {
    ec801.init_cnt++;
    if(ec801.init_cnt >= 5)
    {
      ec801.init_cnt = 0;
      ec801.init_num = 3;  //关闭socket都执行不了，继续回去检查网络状态
    }
    uart_tx_order("AT+QICLOSE=0\r\n",14);  
  }
  else if(ec801.init_num == 8) //开启普通休眠模式
  {
    ec801.init_cnt++;
    if(ec801.init_cnt >=5)
    {
      ec801.init_cnt = 0;
      ec801.init_num = 3;  //启动不了休眠模式，就直接回到网络状态检查
    }
    uart_tx_order("AT+QSCLK=1\r\n",12);  
  }
}
/*- 设备每过5min发送一次数据 -*/
static void NetWork_send_data_CB_hd(uint8_t *code , uint16_t length)
{
//  printk("send state : %d\n",ec801.send_num); 
//  printk("send length : %d\n",length);   
 // log_raw_string(code, length);   
  if(ec801.send_num == 0)
  {
    if(length == 6)
    {
      if(memcmp("\r\nOK\r\n" , &code[0], 6) == 0)
      {
        ec801.send_ATE0_okCnt++;
        if(ec801.send_ATE0_okCnt >= 2){
          ec801.send_ATE0_okCnt = 0;
          // ec801.send_num = 1; //ATE0成功发送，并得到正确返回值
          ec801.send_num = 2;
        }
      }
      else{
        ec801.send_ATE0_okCnt = 0;
      }
    }
  }
  //之前2状态时开启射频，现在不能频繁cfun，此状态取消。
  else if(ec801.send_num == 2) /*检查网络附着*/
  { 
    if(length >= 13)
    {
      if(memcmp("\r\n+CGATT: 1" , &code[0], 11) == 0)
      {
        ec801.send_num = 3; //检查网络附着成功,gatt=1,然后关闭socket，准备连接
        ec801.send_cnt = 0;
      }
    }  
  }
  else if(ec801.send_num == 3)/*检查关闭socket*/
  { 
    if(length==6)
    {
      if(memcmp("OK" , &code[2], 2) == 0)
      {
        ec801.send_num = 4; //确实断开socket了，连接服务器
        ec801.send_cnt = 0;
        ec801.delay_secCnt = 20;  //进入网络连接先主动尝试连一次服务器
      }
    }
  }
  else if(ec801.send_num == 4)/*创建socket，连接TCP服务器是否成功？*/
  {
    if(length>=16)    //会返回各种情况 11 3 16 18 bytes都有可能
    {
      if(memcmp("\r\n+QIOPEN: 0,0\r\n" , &code[0], 16) == 0)
      {
        ec801.send_num = 5; //确实连接成功，准备发数据
        ec801.send_cnt = 0; 
      }
    }
  }
  else if(ec801.send_num == 5)/*正确读取天线信号强度*/
  { 
    uint8_t ten, one, level;
    if((code[3]==0x43)&&(code[4]==0x53)&&(code[5]==0x51)&&(length==21))
    //有可能收到15+6
    {
      ten = code[8] - 0x30;
      one = code[9] - 0x30;
      level = ten * 10 + one;
      if(level == 99){
        usim_rssi = 0; //断网
      }
      if((31<=level)&&(level<99)){
        usim_rssi = 4;
      }
      if((2<=level)&&(level<30)){
        usim_rssi = 3;
      }  
      if(level == 1){
        usim_rssi = 2;
      }
      if(level == 0){
        usim_rssi = 1;
      }
      if(usim_rssi == 0)
      {
        yk_tm.rssi = 0;
        yk_tm.bt_sta = false;
      }
      else
      {
        yk_tm.rssi = usim_rssi;
        yk_tm.bt_sta = true;
      }
      refresh_flag.ble_sta = true;	      
      ec801.send_num = 6; //信号强度读取完毕
      ec801.send_cnt = 0;       
    }
  } 
  else if(ec801.send_num == 6)/*在直传模式下，已经发送QISEND，等待收到 > 发送122字节数据
  ----------------------------  唯一特殊情况，不用在期间切换send num状态！！！！*/
  { 
    if(length==4)
    {
      if((code[0]==0x0D)&&(code[1]==0x0A)&&(code[2]==0x3E)&&(code[3]==0x20))
      {
        //收到了 > 准备发送122bytes的数据
        ec801.send_ing_sta = 2;
      }
    }
    if(length==11)
    {
      if(memcmp("\r\nSEND OK\r\n" , &code[0], 11) == 0)
      {
        //收到了SEND OK改变状态3
        ec801.send_ing_sta = 3;
      }
    }
  }
 else if(ec801.send_num == 7)/*检查关闭socket*/
  {  
    if(length==6)
    {
      if(memcmp("OK" , &code[2], 2) == 0)
      {
        ec801.send_num = 8; //确实断开socket了，连接服务器
        ec801.send_cnt = 0;

        //不要模式7测试
        ec801.dev_num = 0;
        ec801.send_ATE0_okCnt = 0;
        ec801.send_cnt = 0;
        ec801.send_num = 0;  //最小功能都设置不了，只能默认关了，拉低dtr准备睡眠吧
        nrf_gpio_pin_clear(DTR_4G);   //至此初始化完全执行完毕，拉低dtr，进入低功耗睡眠
        //不要模式7测试  
      }
    }
  }
}
static void NetWork_send_data()
{
  if(ec801.send_num == 0)
  {
    nrf_gpio_pin_set(DTR_4G); 
    uart_tx_order("ATE0\r\n",6);
  }
  // else if(ec801.send_num == 1) /* 关闭最小功能，开启射频AT+CFUN=1*/
  else if(ec801.send_num == 2) /* 检查驻网状态AT+CGATT？ */
  {
    ec801.delay_secCnt++;
    if(ec801.delay_secCnt >= 2)
    {
      ec801.delay_secCnt = 0;
      ec801.send_cnt++;
      if(ec801.send_cnt >= 10)
      {//网络检查需要20sec时间
        ec801.send_cnt = 0;
        ec801.send_num = 7;    //网络半天连接不成功，直接可以停止发送，睡眠去了
        //都超过20秒了，还是检测CGATT，不仅结束发送，同时宣布4G连接断开
        yk_tm.rssi = 0;
        yk_tm.bt_sta = false;
        refresh_flag.ble_sta = true;	        
      }
      uart_tx_order("AT+CGATT?\r\n",11);
    }
  }
  else if(ec801.send_num == 3) //关闭socket  
  {
    ec801.send_cnt++;
    if(ec801.send_cnt >= 5)
    {
      ec801.send_cnt = 0;
      ec801.send_num = 7;  //关闭socket都执行不了.直接关闭射频睡眠
    }
    uart_tx_order("AT+QICLOSE=0\r\n",14);
  }
  else if(ec801.send_num == 4) /*创建socket,连接TCP服务器AT+QIOPEN=*/
  {
    ec801.delay_secCnt++;
    if(ec801.delay_secCnt >= 10)
    {
      ec801.delay_secCnt = 0;
      uart_tx_order(AT_OrderBuf,SERVER_LEN);  //每过10sec连接一次服务器
      ec801.send_cnt++;
      if(ec801.send_cnt >= 2)
      {
        ec801.send_cnt = 0;
        ec801.send_num = 7;  //2次服务器都连不上，可以断开socket去睡眠了
      }
    }
  }
  else if(ec801.send_num == 5) /*读取rssi */
  {
    ec801.send_cnt++;
    if(ec801.send_cnt >= 5)
    {
      ec801.send_cnt = 0;
      ec801.send_num = 7;  //信号强度都测不到，可以断开socket保持低功耗
    }
    uart_tx_order("AT+CSQ\r\n",8); 
  }
  else if(ec801.send_num == 6) /*发送本地管理好的数据 */
  {
    if(ec801.send_ing_sta == 0)
    {
      uart_tx_order("AT+QISEND=0,122\r\n",17); 
      ec801.send_ing_sta = 1;
    }
    if(ec801.send_ing_sta == 2) //收到  > 发送122bytes真实数据
    {
      uart_tx_order(ec801_send_buf,122); 
      //后面等待send ok改变send_ing_sta=3
    }
    if(ec801.send_ing_sta != 3) //没有收到sendok最多多发两次122bytes数据，但是多发tcp也只是收到122bytes，没有回撤换行，
                                //在发关闭socket也能正常运行
    {
      ec801.send_cnt++;
      if(ec801.send_cnt >= 2)
      {
        ec801.send_ing_sta = 0;
        ec801.send_cnt = 0;
        ec801.send_num = 7;
      }
    }
  }
  else if(ec801.send_num == 7) //关闭socket  
  {
    ec801.send_cnt++;
    if(ec801.send_cnt >= 5)
    {
      ec801.send_cnt = 0;
      ec801.send_num = 8;  //关闭socket都执行不了.直接关闭射频睡眠

      ec801.dev_num = 0;
      ec801.send_ATE0_okCnt = 0;
      ec801.send_cnt = 0;
      ec801.send_num = 0;  //最小功能都设置不了，只能默认关了，拉低dtr准备睡眠吧
    //  nrf_gpio_pin_clear(DTR_4G);   //至此初始化完全执行完毕，拉低dtr，进入低功耗睡眠
      nrf_gpio_pin_clear(DTR_4G); 
  
    }
    uart_tx_order("AT+QICLOSE=0\r\n",14);
  }
}








































