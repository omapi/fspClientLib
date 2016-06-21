# fspClientLib  Release Notes

> **代码开源地址：**https://github.com/omapi/fspClientLib



## fspClientLib 0.12_V3  ReleaseNotes 2016-06-21  by xxfan

- ##### 新功能和改进

| 功能描述              | 使用方法                                     | 注意事项                                  | 关注人员    |
| ----------------- | ---------------------------------------- | ------------------------------------- | ------- |
| 1. 传参改进           | ./fspClientDome —help                    | 参数名和参数值之间要有空格                         | dmzhang |
| 2. 查询文件夹下的文件列表详情  | ./fspClientDome  -id *设备id* -p *设备密码* -ls *文件夹名称* | 默认密码为newrocktech                      | dmzhang |
| 3. 下载整个文件夹        | ./fspClientDome -id *设备id* -p *设备密码* -g *文件夹名称*  [-s *本地保存路径*] | 1. 文件夹名称必须以斜杠结尾；2. 指定本地保存路径时，保存到当前目录下 | dmzhang |
| 4. 下载文件时可指定本地保存路径 | ./fspClientDome -id *设备id* -p *设备密码* -g *文件名称*  [-s *本地保存路径*] | 未指定保持路径时，保存在当下目录下                     | dmzhang |



## fspClientLib 0.12_V2  ReleaseNotes  2016-06-07  by xxfan



- 更新：更新p2pnat的库(1.1.3)，以及相关的调用接口；
- 新增： 支持获取文件的URL带有目录；