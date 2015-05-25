#!/usr/bin/python
#
#encoding:utf-8

import threading
import redis
import MySQLdb
import re

class Mysql_Handle:
    __conn = None
    __lock = threading.Lock()

    def __init__( self, *connection_kwargs):
        self.__lock.acquire()

        if not self.__class__.__conn:
            self.__class__.__conn = MySQLdb.connect(*connection_kwargs) 
            if ( self.__class__.__conn != None ):
                print 'MySQLdb connected...'
            else:
                print 'MySQLdb connection failed...'

        self.__class__.__lock.release()
    @classmethod
    def getInst( cls, *connection_kwargs ):
        if not cls.__conn:
            print 'MySQLdb has not connected...'
            Mysql_Handle( *connection_kwargs )

        return cls.__conn.cursor()

class Redis_Handle:
    __pool = None
    __lock = threading.Lock()

    def __init__( self, **connection_kwargs ):
        self.__class__.__lock.acquire()

        if not self.__class__.__pool:
            self.__class__.__pool = redis.ConnectionPool( **connection_kwargs )
            if ( self.__class__.__pool != None ):
                print 'redis connection pool initialized...'
            else:
                print 'failed to initialize redis connection pool...'

        self.__class__.__lock.release()

    @classmethod  
    def getInst( cls, **connection_kwargs ):
        if not cls.__pool:
            print 'redis connection pool has not initialized...'
            Redis_Handle(**connection_kwargs)

        return redis.Redis( connection_pool = cls.__pool )

class Statistc_Msgs:
    __key_head__ ="msg/q/"
    # def __init__( self, outfile = '' ):
    #     if ( len( outfile ) > 0 ):
    #         self.of = open( outfile, 'a' )
    #     else:
    #         self.of = sys.stdout

    def statistic_one( self, val ):
        rh = Redis_Handle.getInst()
        key=''
        uid = 0
        if isinstance(val, int):
            key = self.make_key( val )
            uid = val
        else:
            key = val
            userids = re.findall( r'\d{4,12}', val )
            if ( 0 != len( userids ) ):
                uid = int(userids[0])

        if ( '' == key and 0 == uid ):
            return None

        length = rh.llen( key )
        nick = self.get_nick( uid )
        if not nick:
            return None

        return ( length, uid, nick )



    def get_all_keys( self ):
        rh = Redis_Handle.getInst()
        return rh.keys( Statistc_Msgs.__key_head__ + '*' )

    def statistic_all( self ):
        for key in self.get_all_keys():
            one_info = self.statistic_one( key )

            if not one_info:
                continue

            print one_info[0], one_info[1], one_info[2]

    @classmethod
    def make_key( cls, uid ):
        return cls.__key_head__ + str(uid)

    def get_nick( self, userid ):
        cursor = Mysql_Handle.getInst()
        cursor.execute('''select nick from users where UserId=%s''' % userid )
        row = cursor.fetchone()
        cursor.close()
        if not row :
            return None

        return row[0]



if __name__ == "__main__":
    Redis_Handle(host='localhost', port=6379, db=0)
    Mysql_Handle( '192.168.1.57', 'lindu', 'lindu12345', 'cxx' )
    sm = Statistc_Msgs()
    sm.statistic_all()
