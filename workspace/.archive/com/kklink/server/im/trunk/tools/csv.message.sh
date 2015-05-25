#!/bin/sh
mysql_path=/usr/local/mysql/bin/mysql 
#host=192.168.1.55
host=192.168.10.246
db=moon_v2
user=root
time=`date -d $1 "+%Y-%m-%d %H:%M:%S"`
file_potfix=`date -d $1   "+%Y-%m-%d"`

$mysql_path -h $host -D $db -u $user -p -e "select * from message where ctime>'${time}' into outfile '/data0/ttt/test${file_potfix}.csv' FIELDS TERMINATED BY ',' ENCLOSED BY '\"'  ESCAPED BY '\\\' LINES TERMINATED BY '\n';"
