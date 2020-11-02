# WebServer
# 多线程并发处理http请求web服务器，具体实现功能如下：

## 一. 使用Epoll边缘触发，对文件描述符（socket文件描述符和accept文件描述符）进行监听
## 二. 构造多线池并发处理http请求响应
## 三. 对http请求方式get、post进行解析
## 四. 构造最小堆实现http的长连接机制
