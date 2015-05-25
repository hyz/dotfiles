
build:

  $ cd path/to/yxim
  $ b2
  $

run:

  $ export LD_LIBRARY_PATH=/opt/lib64
  $
  $ usr/local/bin/yxim-server yxim.conf
  $

服务启动/重启操作：

1. 真实用户互动
    (1). 文件路径:
        /data0/lindu/moon2.0/interaction/bin/gcc-4.8/release/threading-multi/main ( 192.168.1.55 )
    (2). 启动命令:
        nohup /data0/lindu/moon2.0/interaction/bin/gcc-4.8/release/threading-multi/main &

2. 酒吧聊天室代理:
    (1). 文件路径:
        /usr/local/bin/yxim-chat-proxy ( 192.168.10.243 )
    (2). 启动命令:
        /etc/init.d/yxim-proxys start | stop | restart yxim-chat-proxy  ( 192.168.10.243 )

3. 马甲代理:
    (1). 文件路径:
        /usr/local/bin/yxim-virtual-proxy ( 192.168.10.243 )
    (2). 启动命令:
        /etc/init.d/yxim-proxys start | stop | restart yxim-virtual-proxy  ( 192.168.10.243 )

4. 重启马甲代理、酒吧聊天室代理命令
    /etc/init.d/yxim-proxys start | stop | restart  #( 192.168.10.243 )

5. 重启马甲代理、酒吧聊天室代理命令
    /etc/init.d/yxim-proxys start | stop | restart  #( 192.168.10.243 )

6. 月下IM主服务
    /etc/init.d/yxim-server start | stop | restart  #( 192.168.10.243 )

7. 摇一摇服务端
    /etc/init.d/shk-racing start | stop | restart  #( 192.168.10.243 )

8. flash策略文件服务
    nohub /usr/local/bin/policy_file_request & #( 192.168.10.243 )

9. 马甲消息自动回复
    /etc/init.d/virtual-autoreply start | stop | restart  #( 192.168.10.243 )

10. 聊天室消息自动发送消息
    /etc/init.d/virtual-genchatmessage start | stop | restart  #( 192.168.10.243 )

11. yxim服务监控
    /etc/init.d/yxim-server-monitor start | stop | restart  #( 192.168.10.243 )

