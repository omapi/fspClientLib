#如何获取私网下OM的内部录音文件

##关键字

`私网穿透`、`p2p约会服务`、`FSP协议`、远程下载OM内部存储的录音文件

##本文摘要

介绍如何远程下载局域网内OM设备的内部录音文件

##部署架构

录音存储在OM内部，而OM部署在局域网内，应用服务器部署在公网。

##应用场景

在此架构下，应用服务器想要获取OM内部的录音文件。
    
![image](http://qiniupicbed.qiniudn.com/upload/226714fa2ed9a1cfb1c9da42f2701fdb.png)


##获取录音文件的方法

获取方法基于`p2p约会`和`FSP协议`。（说明：迅时提供相应的[SDK](https://github.com/omapi/fspClientLib.git)以及使用说明。）
###实现步骤：
+ **第一步**，通过`p2p约会`解决`私网穿透`问题，打通应用服务器和OM之间的UDP通道。

+ **第二步**，通过`FSP协议`来下载录音文件。

####步骤1：OM注册p2p约会服务（说明：本步骤由OM自行完成，用户无需参与。）

    
当OM设备启动后，会向约会服务器发起注册，注册时需要向约会服务器提供一些必要信息，如：

+ `cid`：OM作为约会端点的身份唯一标识符，其它端点通过cid来找到OM。

+ `service`：OM提供的约会服务类别，如：FSP、SIP等。

+ `ic`: 各类服务对应的邀请码。（FSP服务当前默认的邀请码为空。）

![image](http://qiniupicbed.qiniudn.com/upload/a5979a31e324e856025c7f1461e6f673.png)    

####步骤2：应用服务器注册p2p约会服务

当应用服务器想要获取OM的录音文件时，同样要先向约会服务器发起注册，注册时向约会服务器提供一些信息，如：

+ `service`：需要的约会服务类型。下载录音文件用的是FSP服务。

+ `ic`：服务提供方（即，OM）的邀请码。（FSP服务当前默认的邀请码为空。）

与OM不同，应用服务器注册时无需指定cid，而是当注册成功后，由约会服务器下发cid。

**对应的函数名称为：**`rendezvous_endpoint_reg`（详解见《P2P约会服务SDK使用指南》）
    ![image](http://qiniupicbed.qiniudn.com/upload/93d73106ec605d9f1527739974825a57.png)


####步骤3：应用服务器申请和OM进行p2p约会
应用服务器向约会服务器申请和某台OM设备新建约会连接，打通彼此之间的UDP通道。

申请时需要提供OM的`cid`，`service`，`ic`。（利用迅时云平台API可查询到OM的p2p约会服务信息）
**对应的函数名称**： `new_rendezvous_connection`（详解见《P2P约会服务SDK使用指南》）
   
   ![image](http://qiniupicbed.qiniudn.com/upload/d8ba921df4af7bc3a2b4b94f24a59b2b.png)


通过以上步骤，即可完成了p2p约会，应用服务器成功和OM打通UDP通道。

####步骤4：通过FSP协议下载录音文件
见FspClinetLib代码的目录下，test为demo的可执行文件，`test.c`为实现代码。
![image](http://qiniupicbed.qiniudn.com/upload/ae7b518ff9ed8a5773a6aad1f92aa9a2.png)



##关于FSP协议的介绍
FSP协议，全称为File Service Protocol，是基于udp的文件管理协议。该协议功能类似FTP协议（如：上传、下载、查询、删除、复制、移动、删除等），但与FTP协议相比，对硬件和网络都有更低的要求。

###FSP的资料：
+ **wiki**：[https://en.wikipedia.org/wiki/File_Service_Protocol](https://en.wikipedia.org/wiki/File_Service_Protocol)
+ **官网**：[http://fsp.sourceforge.net/index.html](http://fsp.sourceforge.net/index.html)

### 迅时的改进

迅时基于FSP官方提供的开源代码，将其加入p2p约会功能，从而解决了私网穿透的问题。

OM设备内置有FSP服务端程序，开发者只需要调用FSP客户端即可。

开发者直接调用迅时提供的FSP客户端测试程序，或基于fspClientlib自行开发。


**SDK下载地址**：[github](https://github.com/omapi/fspClientLib.git)





