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
    users = {}

    wv = []
    for line in in_f:
        line = line.strip()
        if not line:
            continue
        li = line.split()
        uid = int(li[0])
        if not uid:
            continue
        users[uid] = li
        wv.append( 'userid={0}'.format(uid) )
    if not wv:
        return

    sql = 'SELECT userid,app_type FROM token WHERE ' + ' OR '.join(wv)
    # print (sql)
    cursor.execute( sql )
    for r in cursor.fetchall():
        # print (r)
        uid = int( r['userid'] )
        li = users[uid]
        li.insert(-1, r['app_type'])
        print ('\t'.join(li))

### ### ### ### ### ### ### ### ### ### ### ###
if __name__ == '__main__':
    host   = '192.168.1.57'
    port   = 3306
    user   = 'laohu'
    passwd = 'nicecool'
    db     = 'yuexia2'

    # dbc = mysql.connect(user=user, passwd=passwd, database=db, host=host, charset='utf8')
    dbc = mysql.connect(host=host, user=user, passwd=passwd, db=db, charset='utf8')

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

