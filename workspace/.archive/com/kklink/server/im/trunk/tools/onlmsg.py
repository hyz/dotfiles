#!/bin/env python

import sys
import time
import getopt

TIMS = ('N:', 'T:', 'T:', None)
ONLS = ('N:', 'N:', 'N:', None)
MSGS = ('N:', 'N:', 'N:', 'N:', 'N:', None)
ONL  = ('N:uid', 'T:', 'T:', 'N:', None)
MSG  = ('N:uid', 'N:msgid', 'T:', 'T:', 'N:', None)

def intfy(fields):
    ret = fields[:]
    tpl = globals().get(fields[-1])
    first_tm = None
    for i, s in enumerate(fields):
        if not tpl[i]:
            continue
        if tpl[i].startswith('N:'):
            ret[i] = int(s)
        elif tpl[i].startswith('T:'):
            ret[i] = int(s)
            if not first_tm:
                first_tm = ret[i]
            elif ret[i] == 0:
                ret[i] = 999
            else:
                ret[i] -= first_tm
    return ret

def strfy(fields):
    ret = fields[:]
    tpl = globals().get(fields[-1])
    first_tm = None
    for i, x in enumerate(fields):
        if not tpl[i]:
            continue
        if type(x) == type(1):
            if tpl[i].startswith('N:'):
                ret[i] = str(x)
            elif tpl[i].startswith('T:'):
                if not first_tm:
                    first_tm = ret[i]
                    ret[i] = time.strftime('%H:%M:%S', time.localtime(first_tm))
                else:
                    ret[i] = str(x)
    return ret

_sort_fx = 0
_human_s = 0

def main(in_f):
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

    if _sort_fx:
        if _sort_fx > len(field_types) or not field_types[_sort_fx-1]:
            #print field_types[_sort_fx]
            sys.exit(3)
        for rec in sorted(records, key = lambda v: v[_sort_fx-1]):
            print ('\t'.join( strfy(rec) ))
        sys.exit(0)

    for rec in records:
        print ('\t'.join( strfy(rec) ))

if __name__=='__main__':
    in_f = sys.stdin

    opts, args = getopt.getopt(sys.argv[1:],"k:H:")
    for o,v in opts:
        if o == '-k':
            _sort_fx = int(v)
        if o == '-H':
            _human_s = int(v)
    if args:
        in_f = open(args[0])

    #if len(sys.argv)<2: sys.exit(2)

    main(in_f)

