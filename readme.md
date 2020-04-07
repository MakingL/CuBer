# CuBer

## Description

### CuBer, a light weight asynchronous Linux web server with multi-thread and Reactor event model

Cuber 是一个使用多线程 Reactor 事件处理模式的异步 Linux web 服务器

## Intruduction

Cuber 具备以下特性:

- 多线程并发模型，充分利用多核处理器
- 异步事件模型
- 支持 HTTP GET/HEAD method 以及 POST method（目前仅支持 identity-body，不支持 chuncked-body）
- 支持静态文件访问
- 支持反向代理
- 支持 HTTP 长连接
