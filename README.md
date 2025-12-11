# YK - TM project
待完善部分：<br>
1.充电逻辑<br>
2.多包合并发送<br>
3.发送完成通知<br>
4.初始/异常格式化文件<br>

硬件现存问题：<br>
现存问题1:是否可以充电时停止存储<br>
现存问题2:传感器计算公式不一样？？？<br>

---
### update 2025-12-11
完成温湿度存储+发送<br>
notice：发送的时候增加发送效率，修改mtu data length, phy. 实测10000条数据不到7sec<br>
然后文件系统不要在自己定位坐标，很容出错<br>
另外不能频繁的开关文件，关闭就用sync同步一下即可<br>
（commit原因：修改硬件。暂存这个版本程序）<br>

---
### update 2025-11-27
先暂存一版，所有功能基本都以实现，明天开始多包合并发送，提高蓝牙发送效率<br>

完善系统文件读写部分。<br>
写入部分：<br>
    写的时候认为控制坐标，为后面读创造便利。否则读的时候还需要找"\r"来解析<br>
    notice：写的函数其中有段内容snprintf(line, sizeof(line), "%04d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",)<br>
    我们规定好每个变量的位数%04d，这样写入的就定长的。<br>
读取部分：<br>
    通过通道写入的idex来设置postion，读一个设置一次坐标位置。<br>
    字符串解析可以直接通过sscanf解析，所以由此可得写入的时候，要确定位数重要性。由于位数是定长的，解析函数如下，非常方便的到值<br>
```c
	int parsed = sscanf(line,  "%04d,%01d,%04d-%02d-%02d-%02d-%02d,%04d,%04d,%04d\n",
											&storage_idx,
											&channel_type,
											&year, &month, &day, &hour, &min,
											&temp,
											&hum,
											&klux);
```


---


### update 2025-11-26
基本温湿度全部搞定<br>
基于pca9546APWR的切换功能，已加已验证，确定没有问题。在ncs下，可以直接设置传感器命令即可，如果没有设备，iic的api会回报找不到设备地址。<br>
现在就剩下功耗和存储的优化<br>

>/*-------基于fatfs的cvs存储一些知识补充-------*/

notice: fatfs对于文件地址要求非常苛刻，错位或者重叠，没有正确的配置文件模式，都会导致一直写错！！<br>

1. cvs必须存储为字符串，所以需要用snprintf函数内部转换。<br>
int snprintf(char *str, size_t size, const char *format, ...);我们常规存储十进制或者16进制。如下代码<br>
```c
char buf[50];
// 十六进制
snprintf(buf, sizeof(buf), "%X", 255);    // "FF" (大写)
snprintf(buf, sizeof(buf), "%x", 255);    // "ff" (小写)
snprintf(buf, sizeof(buf), "%02X", 15);   // "0F" (固定2位，前面补零)
// 十进制
snprintf(buf, sizeof(buf), "%d", 123);    // "123"
snprintf(buf, sizeof(buf), "%04d", 23);   // "0023" (固定位，补零)
// 无符号
snprintf(buf, sizeof(buf), "%u", 123);    // "123"
```
    我们只需要定义一个比较大的缓冲char[buffer],然后添加pos位置，不断的向内规律的添加数据，然后自增pos+sizeof(data)即可。<br>

---

### not commit update 2025-10-24
可以在techincal document下用AI ASK，挂梯子，中文问答即可。

---

### update 2025-10-16
添加温湿度手册文档。设计详见文档记录。<br>
开始搭建工程，基于nRF52840dk为核心主控，做overlay删减多余perpherial。






















