#!/bin/sh
# 创建人：林都
# 说明：此脚本是为远程冷备份redis数据库准备的

if [ `id -u` -ne 0 ];then
	echo 请以超级用户（root）执行此脚本。
	exit 0
fi

export PATH=/usr/local/bin:$PATH

which redis-cli
if [ $? -ne 0 ];then
	echo "redis-cli程序不可用，请安装。"
	exit 0
fi

# 设置需要备份到本地远程redis连接信息
remove_ip=192.168.10.243
remote_port=6378

# 设置用于备份到本地redis服务程序
local_service=redis_6378
local_rdb=/var/lib/redis/6378/dump.rdb
store_path=/data0/moon_backups/
filename=dump`date +%Y%m%d`.rdb

# 关闭本地redis-server
echo 关闭本地redis-server
service $local_service stop

# 为要备份的redis服务生成最新的二进制数据备份
echo 为要备份的redis服务生成最新的二进制数据备份
redis-cli -h $remove_ip -p $remote_port -r 1 save

# 将最新的数据备份覆盖本地的旧文件
echo 将最新的数据备份覆盖本地的旧文件
redis-cli -h $remove_ip -p $remote_port --rdb $local_rdb

# 重启本地redis-server服务，完成数据备份
echo 重启本地redis-server服务，完成数据备份
service $local_service start

# 将新的dump.rdb文件保存到备份目录
echo 将新的dump.rdb文件保存到备份目录
cp $local_rdb $store_path$filename
