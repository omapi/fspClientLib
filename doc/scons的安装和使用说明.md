# scons的安装和使用说明

## 简介

SCons是一个开源的代码构架工具，是传统的`Make`工具改良后的跨平台替代品，使得代码编译更简单、可靠、快速。

**官网地址：**http://www.scons.org/

## 安装

### 依赖

python

**版本要求：**Python version >= 2.4 and \< 3.0. 

### ubuntu 安装

`sudo apt-get install scons`

### Red Hat 安装

> 如果你的Linux发行版支持yum安装，你可以运行如下命令安装SCons：
>
> `yum install scons`
>
> 否则，下载通用的RPM,下载地址：http://scons.org/pages/download.html，找到最新发布的.rpm文件，比如：[scons-2.5.0-1.noarch.rpm](http://prdownloads.sourceforge.net/scons/scons-2.5.0-1.noarch.rpm)
>
> 从命令行安装，下载合适的.rpm文件，然后运行：
>
> `rpm -Uvh scons-2.5.0-1.noarch.rpm`

### 源码安装

不在此详细介绍。参考：http://blog.csdn.net/andyelvis/article/details/7055377

## 使用

针对本程序，直接在命令行执行`scons`即可。

更多使用细节请自行查找相关资料。