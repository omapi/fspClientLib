# fspClientLib

## 简介

由上海迅时通信设备有限公司基于fsp官网提供的开源代码进行了修改。 加入了p2p约会功能，可解决NAT问题。 该程序可应用于管理迅时IPPBX设备（如，OM20和OM50）本地存储的文件(如：录音文件、语音文件)。

## 编译

### 1. 下载源码

如果你安装了git，直接在命令行执行`git clone https://github.com/omapi/fspClientLib.git`

如果没有安装git，则下载zip包

![下载说明示图](http://qiniupicbed.qiniudn.com/upload/7c6db11210dfd89166704ea2d566d3d8.png)

### 2. 安装scons

本程序采用的代码构建工具为[scons](http://scons.org/)，如果没有，需要先安装。

**安装说明见：**[doc/scons安装和使用说明](https://github.com/omapi/fspClientLib/blob/master/doc/scons%E7%9A%84%E5%AE%89%E8%A3%85%E5%92%8C%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md)

### 3. 编译

本程序引用的`p2pnat`库文件，有32位和64位之分。

需要先打开`SConstruct`文件，将`P2PLAB_PATH`的值修改为和自己的Linux系统匹配的库文件路径。

- **32位操作系统：**`P2PLAB_PATH='p2pnat/lib/x86/‘`


- **64位操作系统：**`P2PLAB_PATH='p2pnat/lib/x64/‘`



之后，在命令行执行`scons`即可开始编译。编译成功后，生成可执行文件test。

## 运行

命令行执行：`./fspClientDemo --help`

即可看到该程序的帮助说明。

**重点提醒(动态库引用)**

因为该程序引用了动态库`libp2pnat.so`，所以程序运行时，只有能找到该库文件才能正常使用。

linux默认的库查找路径为：`/lib`和`/usr/lib`。另外，程序在编译时添加了新的库查找路径----`程序当前运行路径／p2pnat/lib/x64/`(如果你是32位系统则改成x86)。

所以，libp2pnat.so只有在这三个目录的任一目录存在，程序才能正常使用。



如果要将libp2pnat.so放到其他地方，则通过导入环境变量实现

如：`export LD_LIBRARY_PATH=/home/xxfan/p2pnat/lib/x64`

(`/home/xxfan/p2pnat/lib/x64`为我的libp2pnat.so存放路径)



## 其它	

更多资料请查询[doc](https://github.com/omapi/fspClientLib/blob/master/doc/)文件夹下的内容

