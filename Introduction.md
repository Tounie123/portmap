端口映射工具

# Introduction #

portmap 主要是作为各个机器间端口映射的工具
项目的最初目的是作为vnc端口映射，主要是将不同机器的vnc接口映射到一台能够对外的
物理机上，安全和方便管理；<br />
iptables可以实现端口映射，但是每次增加一个映射端口都需要重新启动，故自己动手完成了端口映射工具；<br />
还有像80端口的映射，这个可以归总为反向代理，作为负载均衡器来使用，原理是相通，像nginx七层的负载均衡功能应该使用的就是端口映射，portmap暂时不支持哈~
![http://portmap.googlecode.com/files/arch.png](http://portmap.googlecode.com/files/arch.png)
# Details #


  * 使用epoll方式，支持高并发和长连接模式
  * 多线程支持，线程个数可配置，主要作为工作线程
  * 使用简单，只需发送待映射的端口到守护进程，守护进程会返回映射后的端口，即可使用
  * 

# TODO #
  * 配置文件单独提取出来
  * bug修复
  * 多机扩展？

# Example #
  1. make
  1. ./portmap  //启动进程
  1. cd test    修改test1.c中相关变量
> > addr: portmap server的地址
> > port: portmap server 的端口
> > request:是需要映射的地址和端口
```

    const char *addr = "10.60.1.124";
    unsigned int port = 5903;

    const char *request = "10.60.1.124:5905";
```
  1. make && ./test1
> > 正常情况下，会给你返回映射后的地址和端口