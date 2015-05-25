#!/bin/sh

DIR_=`dirname $0`
TOP_=`dirname $0`/..

which ptree-ini || exit $?

export PATH=/usr/local/mysql/bin:$PATH
which mysql || exit $?

cfg=$DIR_/db.conf
if [ -n "$1" ]; then
    [ -e "$1" ] || exit 2
    cfg=$1
fi

usr=`ptree-ini $cfg "database.user"`
pass=`ptree-ini $cfg "database.password"`
host=`ptree-ini $cfg "database.host"`
port=`ptree-ini $cfg "database.port"`
db=`ptree-ini $cfg "database.db"`

#
# mysql --table -p$pass -u $usr --host $host --port $port $db
mysql -p$pass -u $usr --host $host --port $port $db

