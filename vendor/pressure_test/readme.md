# 压力测试

## 环境准备

1. CuBer

    - 安装编译环境: `sudo yum -y install git make cmake gcc-c++ boost boost-devel`
    - 下载源码: `git clone git@github.com:MakingL/CuBer.git`

2. Nginx 和 echo模块

    - 源码安装 Nginx 及  echo 模块。可参考: [cnblogs - Nginx安装echo模块](https://www.cnblogs.com/52fhy/p/10166333.html)

3. 压力测试工具 Apache Bench (简称 ab)

    - 安装（CentOS）: `sudo yum -y install httpd-tools`

4. `ulimit -a` 查看当前能打开的文件数，确保比 `conf/pressure_test.yaml` 中的 `worker_connections` 大

## CuBer 配置

1. 编译构建时使用:

    - `cmake ./ -DPRESSURE_TEST_MODEL=ON`
    - `make`

2. 运行 CuBer， 命令:

    `./bin/CuBer -c conf/pressure_test.yaml`

3. 长连接压测命令:

    - `ab -n 100000 -k -r -c 1 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 5 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 10 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 50 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 100 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 500 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 1000 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 5000 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -k -r -c 10000 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`

4. 短连接压测命令:

    - `ab -n 100000 -r -c 1 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 5 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 10 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 50 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 100 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 500 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 1000 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 5000 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`
    - `ab -n 100000 -r -c 10000 -w 127.0.0.1:8080/hello/ >>pressure_test_result.html`

## Nginx 配置

1. 配置 Nginx echo 应用

    - 按照 `pressure_test/nginx.conf` 文件中的内容对 Nginx 进行配置

2. 修改配置文件后确保更新 Nginx 配置，命令 `sudo nginx -s reload`
3. Nginx 短连接压测命令:

      - `ab -n 100000 -r -c 1 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 5 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 10 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 50 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 100 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 500 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 1000 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 5000 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`
      - `ab -n 100000 -r -c 10000 -w 127.0.0.1:8088/hello/ >>pressure_test_result.html`

## 查看结果

1. 压测结果保存在 pressure_test_result.html 文件中，下载后直接查看

## 本人测试环境

- 云服务器单机环境
- CPU: 8vCPUs | Intel Cascade Lake 3.0GHz
- 内存: 16GB

## 测试结果

- CuBer 压力测试结果

    ![CuBer压测结果](../images/CuBer_long_short_conn.png)

- CuBer、Nginx 的 Http 短连接压测结果

    ![CuBer Nginx 压测结果](../images/CuBer_Nginx_short_conn.png)
