#!/bin/env python

import sys
import time
import getopt

TIMS = ('n:0', 'tm', '')
ONLS = ('n:0', 'tm', 'n:current', 'n:total', '')
MSGS = ('n:0', 'tm', 'n:success', 'n:total', 'n:response', '')
ONL  = ('n:uid', 'tm', 'tm', '')
MSG  = ('n:uid', 'n:msgid', 'tm', 'tm', 'tm', '')

def intfy(fields):
    ret = fields[:]
    tpl = globals().get(fields[-1])
    first_tm = None
    for i, s in enumerate(fields):
        if tpl[i].startswith('n:'):
            ret[i] = int(s)
        elif tpl[i] == 'tm':
            ret[i] = int(s)
            if not first_tm:
                first_tm = ret[i]
            elif ret[i] == 0:
                ret[i] = 36000
            else:
                ret[i] -= first_tm
    return ret

def strfy(fields):
    ret = fields[:]
    tpl = globals().get(fields[-1])
    first_tm = None
    for i, x in enumerate(fields):
        if type(x) == type(1):
            if tpl[i].startswith('n:'):
                ret[i] = str(x)
            elif tpl[i] == 'tm':
                if not first_tm:
                    first_tm = ret[i]
                    ret[i] = time.strftime('%H:%M:%S', time.localtime(first_tm))
                else:
                    ret[i] = str(x)
    return ret

def main():
    sort_fx = 0
    in_f = sys.stdin

    opts, args = getopt.getopt(sys.argv[1:],"k:")
    for o,v in opts:
        if o == '-k':
            sort_fx = int(v)
    if args:
        in_f = open(args[0])

    #if len(sys.argv)<2: sys.exit(2)

    records = []
    field_types = []

    for line in in_f:
        fields = intfy(line.strip().split()) 
        records.append(fields)
        if not field_types:
            field_types = [ type(x) for x in fields ]
        else:
            if len(field_types) < len(fields):
                field_types += [None] * (len(fields) - len(field_types))
            for i, x in enumerate(fields):
                if field_types[i] != type(x):
                    field_types[i] = None

    if not records:
        sys.exit(1)

    if sort_fx:
        if sort_fx > len(field_types) or not field_types[sort_fx-1]:
            #print field_types[sort_fx]
            sys.exit(3)
        for rec in sorted(records, key = lambda v: v[sort_fx-1]):
            print '\t'.join( strfy(rec) )
        sys.exit(0)

    for rec in records:
        print '\t'.join( strfy(rec) )

if __name__=='__main__':
    main()

