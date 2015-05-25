#!/bin/env python3

import sys, os
import time
import getopt

TIMS = ('N:', 'T:', 'T:')
ONLS = ('N:', 'N:', 'N:')
MSGS = ('N:', 'N:', 'N:', 'N:', 'N:')
ONL  = ('N:uid', 'T:', 'T:', 'N:')
MSG  = ('N:uid', 'N:msgid', 'T:', 'T:', 'N:')

def intfy(fields):
    ret = fields[:]
    tpl = globals().get(fields[-1], [])
    first_tm = None
    for i, s in enumerate(fields):
        if not (i < len(tpl)) or not tpl[i]:
            continue
        if tpl[i].startswith('N:'):
            ret[i] = int(s)
        elif tpl[i].startswith('T:'):
            ret[i] = int(s)
            if not first_tm:
                first_tm = ret[i]
            elif first_tm <= ret[i]:
                ret[i] -= first_tm
    return ret

def strfy(fields):
    ret = [ str(x) for x in fields ]
    tpl = globals().get(fields[-1], [])
    first_tm = None
    for i, x in enumerate(fields):
        if type(x) != int:
            continue
        if not (i < len(tpl)) or not tpl[i]:
            continue
        if tpl[i].startswith('T:'):
            if not first_tm:
                first_tm = x
                ret[i] = time.strftime('%H:%M:%S', time.localtime(first_tm))
    return ret

def main(in_f, out_dir):
    records = []
    onl_ios, onl_and = [], []
    msg_ios, msg_and = [], []
    onl_ios_s, onl_and_s, onl_s_p = [ 0, 0, 0, 0, 0 ] , [ 0, 0, 0, 0, 0 ] , ( 3, 10, 30, 180, 0x7fffffff )
    msg_ios_s, msg_and_s, msg_s_p = [ 0, 0, 0, 0, 0 ] , [ 0, 0, 0, 0, 0 ] , ( 2,  5, 10,  30, 0x7fffffff )
    def index(x, p):
        for i, m in enumerate(p):
            if x <= m:
                return i

    for line in in_f:
        line = line.strip()
        rec = intfy(line.split('\t')) 
        records.append(rec)
        if rec[-1] == 'ONL': # 100008	1407136845	1407136965	0	1	wei	ONL
            if rec[-2] == '1':
                onl_ios.append(rec)
                onl_ios_s[index(rec[2], onl_s_p)] += 1
            elif rec[-2] == '3':
                onl_and.append(rec)
                onl_and_s[index(rec[2], onl_s_p)] += 1
        elif rec[-1] == 'MSG': # 100016	2363	1407137376	1407137376	0	1	老南	MSG
            if rec[-2] == '1':
                msg_ios.append(rec)
                msg_ios_s[index(rec[3], msg_s_p)] += 1
            elif rec[-2] == '3':
                msg_and.append(rec)
                msg_and_s[index(rec[3], msg_s_p)] += 1
    onl_ios_s.append( sum(onl_ios_s) )
    onl_and_s.append( sum(onl_and_s) )
    msg_ios_s.append( sum(msg_ios_s) )
    msg_and_s.append( sum(msg_and_s) )

    def output(records, fn, mode='w'):
        with open(os.path.join(out_dir, fn), mode) as out_f:
            for rec in records:
                out_f.write( '\t'.join(strfy(rec)) + '\n' )
    output(onl_ios, '_onl_ios.txt')
    output(onl_and, '_onl_and.txt')
    output(msg_ios, '_msg_ios.txt')
    output(msg_and, '_msg_and.txt')
    output([ ['onl:ios'] + onl_ios_s ], '_s.txt', mode='w')
    output([ ['onl:and'] + onl_and_s ], '_s.txt', mode='a')
    output([ ['msg:ios'] + msg_ios_s ], '_s.txt', mode='a')
    output([ ['msg:and'] + msg_and_s ], '_s.txt', mode='a')
    output(records, '_onlmsg.txt')

if __name__=='__main__':
    in_f = sys.stdin
    out_dir = '.'

    opts, args = getopt.getopt(sys.argv[1:],"o:")
    for o,v in opts:
        if o == '-o':
            out_dir = v
            os.makedirs(out_dir, exist_ok=True)
    if args:
        in_f = open(args[0])

    main(in_f, out_dir)

