# nidavellir-devices

nidavellir devices system

## OS项目输出物 TODOs

**Device 设备端**

1. **sdk.a**

`interface`

- [ ] application request lib
- [ ] callback request
- [ ] module lib

2. **os.bin**

`application code + kernel + module manager + module lib + drivers`

- [ ] module manager(`kernel thread`)
- [ ] resource manager(`kernel thread`)
- [ ] kernel
- [ ] application init code(`main()`)
- [ ] module manager(`application life cycle`)
- [ ] drivers

3. **modules.bin**

可独立分发的应用程序

- [ ] modularize applications(`applications--user thread`)
	- [ ] base_app.bin
	- [ ] uart_tx_app.bin*3 
	- [ ] uart_rx_app.bin*3 
	- [ ] inter_cpu_communication_app.bin
- [ ] modularize drivers

- [ ] demos(`iar+keil+gcc`)

4. **utilities**
- [ ] uploader
- [ ] debug utilities(`shell + tracex`)
- [ ] qemu

5. **docs**

- [ ] ppt
- [ ] 开发文档

**module dashboard**

1. **开发**
	- [ ] 申请module ID(`access token`)
	- [ ] 展示本地module.bin
	- [ ] 上传module到设备端(调试模式)
	- [ ] 数据管理(上位机)
	- [ ] 打包(签名) 
	- [ ] 将本地module.bin发布到云端
	
2. **管理和部署**
	- [ ] 下载云端module.bin列表
	- [ ] 解析module.bin 
	- [ ] 展示所有的module(`app store`)
	- [ ] 发现和联接设备端
	- [ ] 展示设备信息
		- [ ] 硬件信息
		- [ ] 系统信息
		- [ ] 软件信息
		- [ ] 运行状态信息
	- [ ] 上传module到设备端
	- [ ] 远程操作module

## milestones

|  时间   | 里程碑  |
|  ----  | ----  |
|2021-10-30|   合并代码并能运行<br>按照测试用例分工自测|
|2021-11-06 |   第一次台体测试|
|2021-11-13 |  第二次台体测试|
|2021-11-20  | 提交WG测试|

## high level design


|     | 名  | 
|  ----  | ----  |
|**instances**| stategrid-meter  | 
|**applications** |  meterbase-x/698_server/<br>698-client/dlms-server/dlms_client|
|**nidavellir-api** |nidavellir-c/nidavellir-os/nidavellir-x-languages |
|**packages**  | meterbase/dlms/dl645/dl698/<br>tx/zx/freertos/windows/liteos/baremetal/<br>resource-manager/drivers/cpus/filex/levelx/crypto|
----

## 生态系统
**应用管理**
- 用户注册
- 应用申请
- 开发工具链
- 应用开发
- 应用部署前的打包

**测试管理**
- 包级测试
- 应用级测试
- 设备测试
- 部署测试

**设备管理**  

- 创建设备
- 设备描述信息和注册
- 应用部署

## 举例

```

单相表: nidavellir-baremetal + app_meterbase + app_698_server
物联网计量芯: nidavellir-baremetal + app_meterbase_1 + app_698_server(2) + app_exchange_with_m33
物联网管理芯: nidavellir-os + app_meterbase_2 + app_698_server(3) + app_exchange_with_m0
国网三相表: nidavellir-baremetal + app_meterbase + app_698_server(3)

```


协议无关    --》  1. 无id   2. 地址和存储  3. 

其中，协议类应用(app_698_server)跟基表应用(app_meterbase)的数据转换方式如下:

在map中实现转换，确保meterbase的协议无关性

meterbase聚合根  (变量Ua)       <- map ->   ID (1.8.0)

meterbase<-->resource manager  [3个分区 static ]

协议   <--->  resource manager  [driver]


map()
{
	sw
}

## packages

含**业务概念/业务状态/业务规则**

不可单独部署,但要单独发布源码和库，发布时提供移植指南，**方便供第三方进行移植和使用**。

平台组件  或者叫 包

基础服务层

domain领域层

不可单独执行

导出`packages.h`供上层`application`使用

每个组件的配置

编程注意事项:

1. 多数package能够使用到多个环境(前后台/多线程/多进程/动态/静态/dll/lib)下，故单个package不能依赖别的package，*只依赖fifo、instance->cache和消息*。更不能直接操作设备(外设和内设)，特别是业务包不能依赖操作系统包。(kernel包也不依赖  **考虑中，待定**)

2. 同理，package要满足**可重入和多实例**的要求。因此，代码中不能出现全局变量，甚至文件作用域的变量。

3. 消息格式设计  'cmd len data cmd' 

4. 此目录单独代码仓库

## nidavellir

此层次是发布包。可基于此发布包开发定制的application、test和instance

- bin  //发布的二级制文件和工具集
- include //发布的开发包
- samples // 一些例子
- tests  //测试用例

## applications

调用packages构建的业务强相关的应用

本质上只是package的重组和重排

可单独执行

可单独部署

独立仓库

发布时务必提供三种形态: `baremetal/thread/process`,分别对应`baremetal_entry.c;thread_entry.c;process_entry.c`。

导出 applications.h

###  编程模型

总体上，采用消息或事件驱动模型。其中，消息队列的设计如下

- 在前后台环境下，利用`fifo`来充当消息队列。
- 在多线程和多进程环境下，利用操作系统提供的线程间通信和同步机制(信号量/事件标志/队列/邮箱/管道)来充当消息队列，提高消息处理速度。

具体上，创建application可分以下步骤

1. 在applications目录下新建应用目录

2. 在应用目录中新建baremetal_entry.c

3. baremetal_entry.c的基本模型如下

```
int app_entry(struct instance_t* instance, 
			struct task_t* task, struct message_t* mesg)
{
    int r = 0;
    //1.接入校验，验证接收到的消息的完整性和合法性
    //2.处理消息
    switch (mesg->ctrl)
    {
    case 0x__:
		//3.调用领域层完成业务  反映用例场景
        break;
    case 0x__:
		//3.调用领域层完成业务  反映用例场景
        break;
    default:
        break;
    }

    return 0;
}
```
### message


```
struct message_t

head cmd len data tail
head: 0x88
cmd: <= 32 kernel message
cmd: > 32  user message
data: 
tail: 0x88
```

**说明**:

在前后台环境下，此message通过fifo传递
在多线程环境下，此message通过queue传递
### 资源管理


## instances

产品实例集合

里面的每个实例对应一个产品

独立仓库

其包括以下内容:

1. 硬件配置声明: 当前实例所使用的硬件和设备配置清单

2. 数据资源声明: 当前实例所使用的"数据声明"及其在硬件上的布局情况
	
	数据资源的声明方法？
	
	数据资源的管理方法？

3. 组件配置声明: 当前实例所使用的组件及其配置

4. 应用配置声明: 当前实例所使用的应用及其配置

5. 部署方式声明: `baremetal/multi_threads/multi-processes`

本目录下只有一个函数入口`xxxx_instance_entry`. **即要得到不同的instance，只需改写此入口函数**。

前后台的入口函数如下:

```
int baremetal_instance_entry(int argc, char** argv)
{
    //1. 构造instance
		//初始化硬件
		//初始化固件
		//初始化资源管理器
			//获取设备信息
			//获取数据模型布局
			//绑定模型  设备 和方法
		//初始化软件
			//创建每个application
			//初始化每个application
			//每个application的control block如何初始化?且如何绑定到instance里?
			//每个application如何访问自己的control block?
		//链接和绑定硬件 固件 和 软件
    //2. 启动主循环，执行此instance
    return baremetal_loop(instance);
}
```
多线程的入口函数如下:

```

```
## 测试

## workflow
