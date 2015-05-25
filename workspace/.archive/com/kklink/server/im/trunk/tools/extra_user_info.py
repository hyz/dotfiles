#!/bin/env python3
#
# -*- coding: utf-8 -*-

import os, sys, time
import re
import pymysql as mysql
# import MySQLdb as mysql
# import mysql.connector as mysql

# import ctypes, ctypes.util
# libc_path = ctypes.util.find_library('c')
# libc = ctypes.cdll.LoadLibrary(libc_path)

def extra_info(cursor, in_f):
    records = []

    wv = set()
    for line in in_f:
        line = line.strip()
        li = line.split()
        if not li:
            continue
        uid = int(li[0])
        if not uid:
            continue
        records.append( li )
        wv.add(uid)
    if not wv:
        return

    infos = {}

    cond = ' OR '.join('t1.userid={0}'.format(x) for x in wv)
    sql = 'SELECT t1.userid,app_type,nick,app_version FROM token t1 LEFT JOIN users t2 on t1.userid=t2.userid WHERE ' + cond
    # print (sql)
    cursor.execute( sql )
    for r in cursor.fetchall():
        # print (r)
        uid = int( r['userid'] )
        infos[uid] = r;

    for li in records:
        uid = int( li[0] )
        r = infos.get(uid, None)
        if not r:
            # sys.stderr.write( str(uid) + '\n')
            continue
        nick = r['nick']
        # nick = nick.replace('\t',' ')
        li.insert(-1, nick)
        li.insert(-1, r['app_version'])
        li.insert(-1, r['app_type'])
        print ('\t'.join(str(x) for x in li))

def get_mysql_conf():
    import configparser as inicfg
    config = inicfg.ConfigParser()
    config.read(os.getenv("MY_CONF", "/etc/user-info-mysql.conf"))
    cnf = dict(config.items('mysql'))
    cnf['port'] = int( cnf.get('port', 3306) )
    return cnf
    ##host   = cnf.get('host')
    ##port   = cnf.get('port')
    ##user   = cnf.get('user')
    ##passwd = cnf.get('passwd')
    ##db     = cnf.get('db')
    ##return host, port, user, passwd, db

### ### ### ### ### ### ### ### ### ### ### ###
if __name__ == '__main__':
    # dbc = mysql.connect(user=user, passwd=passwd, database=db, host=host, charset='utf8')
    dbc = mysql.connect( **get_mysql_conf() ) #(host=host, user=user, passwd=passwd, db=db, charset='utf8')

    cursor = dbc.cursor(mysql.cursors.DictCursor) #(cursorclass=mysql.cursors.DictCursor)
    extra_info(cursor, sys.stdin)

    #cursor.execute('SELECT userid,app_type FROM token limit 10')
    #cursor.execute('SELECT id,app_type FROM users limit 10')
    #for l in cursor.fetchall(): print(l)
    #cursor.execute('SELECT t.userid,t.app_type,u.nick FROM token as t, users as u where t.userid=u.id limit 10')
    #for l in cursor.fetchall():
    #    print(l)

    cursor.close()
    dbc.close()

