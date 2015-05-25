#!/bin/sh

filename=`date +%Y%m%d`
host=192.168.10.246
username=lindu
password=lindu12345
store_path=/data0/moon_backups/
db=yuexia

/usr/local/mysql/bin/mysqldump -p $db -h$host -u $username -p$password | gzip > $store_path$db$filename.gz
