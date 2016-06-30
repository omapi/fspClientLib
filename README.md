# fspClientLib(Linux版)

## 简介

fsp协议是基于udp的文件管理协议。该协议功能类似ftp协议（如：上传、下载、查询、删除、复制、移动等）

该项目由上海迅时通信设备有限公司基于fsp官网提供的开源客户端库进行改进，加入了`点对点NAT穿透`功能。

利用该软件，用户可远程管理迅时OM设备的部分本地文件。比如，用户可备份多台OM设备的本地录音文件到一台电脑上进行集中管理。



## 功能

**目前，该软件Demo(V0.12_V5版本)已实现的功能有：**

- **点对点NAT穿透：**
  - 一个点在公网，一个点在私网(100%可穿透)
  - 两个点都在私网；(非100%可穿透，跟具体的NAT类型有关)


- **可管理的文件类型：**
  - 录音文件(服务端目录为Recorder)
  - 语音文件(服务端目录为voice file\_1 和voicefile\_2)
- **可管理的手段：**
  - 下载文件或文件夹
  - 列出文件夹下的文件详情(类型，大小，更新日期，文件名称)
  - 修改fsp密码(⚠️需重启OM设备才能生效)



**其他更多功能，请大家参考Demo自行开发。**



## Demo的使用方法

### 1. 下载：

进入bin目录，下载和自己系统对应的tar包。如：fspClientDemo\_0.12_V5\_x64.tar.gz



### 2. 解压：

命令行执行：

`tar -zxvf {文件名}.tar.gz`



文件说明

- fspClientDemo 为可执行程序


- p2pnat 为程序所依赖的动态库



### 3. 运行

##### 查看帮助说明

`./fspClientDemo —help`



正常使用时，需要3个必选参数：`device_id`，`invite_code`，`password`

- `id (device_id)`： 设备唯一标识符，获取方法：
  	1.  下载OM的日志文件，查看fspd.log，找到如：device_id-xxxx 
  	2.   通过迅时云平台API获取，具体请联系400电话获取云平台相关资料和技术支持(400电话：4007779719和4006172700) 
- `ic (invite_code)`： p2p约会的邀请码，默认值为neworocktech。要想修改该参数值则需修改OM的配置文件。
- `p (password)`：fsp密码，默认值为newrocktech，要想修改该参数值，可直接在客户端执行修改密码的操作，或修改服务端(OM)的配置文件。



##### **下载文件或文件夹**

`./fspClientDemo -id {OM设备的device_id}  -ic {p2p邀请码}   -p {fsp密码}   -g {下载路径}   -s {保存路径}`



⚠️：

1. 如果要下载的为文件夹，则下载路径必须已／结尾 。如： -g Recorder/20160630/
2. 如果要下载的文件夹包括子文件夹，则会忽略子文件夹，只下载文件；
3. 如果未指定保存路径，则下载到软件运行的当前目录；



##### **查看文件夹的文件详细列表**

`./fspClientDemo -id {OM设备的device_id}  -ic {p2p邀请码}   -p {fsp密码}   -ls {要查看的文件夹}`



##### 修改密码

`./fspClientDemo -id {OM设备的device_id}  -ic {p2p邀请码}   -p {老的密码}   -np {新的密码}`



## 二次开发

如果Demo提供的功能不能完全满足你的要求，你可基于该代码进行二次开发。

### 1. 下载源码

如果你安装了git，直接在命令行执行`git clone https://github.com/omapi/fspClientLib.git`

如果没有安装git，则下载zip包

![下载说明示图](http://qiniupicbed.qiniudn.com/upload/7c6db11210dfd89166704ea2d566d3d8.png)

### 2. 安装scons

本程序采用的代码构建工具为[scons](http://scons.org/)，如果没有，需要先安装。

**安装说明见：**[scons安装和使用说明](https://github.com/omapi/fspClientLib/blob/master/doc/scons%E7%9A%84%E5%AE%89%E8%A3%85%E5%92%8C%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md)

### 3. 编译

本程序引用的`p2pnat`动态库的有32位和64位之分。

需要先打开`SConstruct`文件，将`P2PLAB_PATH`的值修改为和自己的Linux系统匹配的库文件路径。

- **32位操作系统：**`P2PLAB_PATH='p2pnat/lib/x86/‘`


- **64位操作系统：**`P2PLAB_PATH='p2pnat/lib/x64/‘`



之后，在命令行执行`scons`即可开始编译。编译成功后，生成可执行文件。



## 其它	

更多资料请查询[doc](https://github.com/omapi/fspClientLib/blob/master/doc/)文件夹下的内容

