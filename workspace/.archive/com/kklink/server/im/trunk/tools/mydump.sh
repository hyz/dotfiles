#!/bin/sh

TOP_=`dirname $0`/..

ini=ptree-ini

cfg=$1 ; shift
if [ ! -f "$cfg" ]; then
    exit 1
fi

usr=`ptree-ini $cfg "mysql.user"`
pass=`ptree-ini $cfg "mysql.password"`
host=`ptree-ini $cfg "mysql.host"`
port=`ptree-ini $cfg "mysql.port"`
db=`ptree-ini $cfg "mysql.db"`

export PATH=/usr/local/mysql/bin:$PATH
#
mysqldump -p$pass -u $usr --host $host --port $port $db $*

