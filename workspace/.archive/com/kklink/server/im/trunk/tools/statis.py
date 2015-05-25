#!/bin/env python3

import os, sys, time
from datetime import datetime
from functools import partial
from getopt import getopt
import pymysql as mysql

def make_lists(if_source, idx, tags):
    ls = [ [] for x in tags ] # [ [] ] * len(tags)
    for line in if_source:
        l = line.strip().split('\t')
        t = l.pop(idx)
        if t not in tags:
            continue
        #print(t, tags.index(t), l)
        ls[ tags.index(t) ].append( l )
    return ls # records

def make_dict(if_fx, index=0):
    dct = {}
    for line in if_fx:
        try:
            l = line.strip().split('\t')
            dct[ l[index] ] = l
        except:
            pass
    return dct

def extend_filter(ls, xids):
    ret = []
    for l in ls:
        lx = xids.get(l[0], None)
        if not lx:
            continue
        ret.append(l + lx)
    return ret

def parts(ls, idx, arr):
    ret = [ [] for x in range(len(arr)+1) ] # [ [] ] * (len(arr) + 1)
    def index(x, a):
        for i, m in enumerate(a):
            if x <= m:
                return i
        return len(a)
    for l in ls:
        ret[ index(l[idx], arr) ].append(l)
    return ret;

def prep_onl(fields, desc):
    ret = fields[:]
    first_tm = None
    for i, sval in enumerate(fields):
        if not (i < len(desc)) or not desc[i]:
            continue
        _type, _name = desc[i]
        if _type == datetime:
            ret[i] = ts = int(sval)
            if first_tm is None:
                first_tm = ts # ret[i] = datetime.fromtimestamp( ts )
            elif first_tm <= ret[i]:
                ret[i] -= first_tm
        elif _type:
            ret[i] = _type(sval)
    return ret
prep_msg = prep_onl

# '%F %T', # %T = %H:%M:%S
def ofmt_onl(fields, desc, tfmt='%T'):
    # assert len(fields) == len(desc)
    ret = [ str(x) for x in fields ]
    first_tm = None
    for i, xval in enumerate(fields):
        if not (i < len(desc)) or not desc[i]:
            continue
        _type, _name = desc[i]
        if _type == datetime:
            if first_tm is None:
                first_tm = xval
                ret[i] = time.strftime(tfmt, time.localtime(xval))
            elif xval >= first_tm:
                ret[i] = time.strftime(tfmt, time.localtime(xval))
    return '\t'.join( ret )
ofmt_msg = ofmt_onl

def output(lines, fn, linesep='\n', mode='w'):
    with open(fn, mode) as out_f:
        out_f.write( linesep.join( lines ) )
        #for l in lines: out_f.write( l + linesep )

_desc_onl = (int,'uid'), (datetime,'begin'), (datetime,'end'), (int,'stat')
_desc_msg = (int,'uid'), (int,'msgid'), (datetime,'begin'), (datetime,'end'), (int,'stat')

def main(if_source, if_ext, out_dir):
    desc_us = if_ext.readline().strip()
    desc_us = tuple( eval(x) for x in desc_us.split('\t') )
    #10027	294	1407857823	1407857823	0	MSG
    #10027	1407856956	1407856972	0	ONL

    extd = make_dict(if_ext, 0)
    ONL, MSG, TIMS = make_lists(if_source, -1, ('ONL', 'MSG', 'TIMS'))
    ONL, desc_onl = extend_filter(ONL, extd), _desc_onl + desc_us
    MSG, desc_msg = extend_filter(MSG, extd), _desc_msg + desc_us

    ### preprocess
    ONL = list( map(partial(prep_onl, desc=desc_onl), ONL) )
    MSG = list( map(partial(prep_msg, desc=desc_msg), MSG) )

    ### parts
    ONL_iOS = list( filter(lambda l: (l[-1]==1), ONL) )
    ONL_And = list( filter(lambda l: l[-1]==3, ONL) )
    sONL_iOS = parts(ONL_iOS, 2, (3, 10, 30, 180) )
    sONL_And = parts(ONL_And, 2, (3, 10, 30, 180) )

    MSG_iOS = list( filter(lambda l: l[-1]==1, MSG) )
    MSG_And = list( filter(lambda l: l[-1]==3, MSG) )
    sMSG_iOS = parts(MSG_iOS, 3, (2, 5, 10, 30) )
    sMSG_And = parts(MSG_And, 3, (2, 5, 10, 30) )

    ### output
    if TIMS: # (int,None), (datetime,'start'), (datetime,'now')
        timv = time.localtime((int(TIMS[0][2]) + int(TIMS[-1][2])) / 2)
    else:
        timv = time.localtime()
    out_dir = os.path.join(out_dir, time.strftime('%F', timv))
    _of = partial(os.path.join, out_dir)

    os.makedirs(out_dir, exist_ok=True)

    f = partial(ofmt_onl, desc=desc_onl)
    output(map(f,ONL_iOS), _of('ONL.iOS.txt'))
    output(map(f,ONL_And), _of('ONL.And.txt'))

    f = partial(ofmt_msg, desc=desc_msg)
    output(map(f,MSG_iOS), _of('MSG.iOS.txt'))
    output(map(f,MSG_And), _of('MSG.And.txt'))

    for s in 'sONL_iOS', 'sONL_And', 'sMSG_iOS', 'sMSG_And':
        ls = locals().get(s)
        v = [ len(l) for l in ls ]
        v.append( sum(v) )
        output( map(str,v), _of(s+'.txt'), linesep='\t')

    # output_ll(records, os.path.join(out_dir, 'onlmsg.txt'))

def list_f(f_source, sep='\t'):
    l_data = []
    for line in f_source:
        l_data.append( line.strip().split(sep) )
    return l_data # records

def extend(l_data, d_ext, colx=0):
    for l in l_data:
        if not l:
            continue
        l.extend(d_ext.get(l[colx], []))
    return l_data

def join(fn_ext):
    l_data = list_f(sys.stdin)
    d_ext = make_dict( open(fn_ext), 0 )
    for l in extend(l_data, d_ext):
        print('\t'.join(l))

# statis.py -xtimef "[0,(datetime,0),(datetime,0)]"
def timef(desc):
    desc = eval(desc)
    for l in list_f(sys.stdin):
        l = prep_onl(l, desc)
        print(ofmt_onl(l,desc, '%F %T'))

fromtimestamp = lambda x: datetime.fromtimestamp(int(x))

def tr(expr, f):
    f = globals().get(f)
    expr = re.compile(expr)
    for l in list_f(sys.stdin):
        for i, x in enumerate(l):
            if expr.match(x):
                l[i] = str( f(x) )
        print ('\t'.join(l))

### ### ### ### ### ### ### ### ### ### ### ###
if __name__ == '__main__':
    if_source = sys.stdin
    if_ext = None
    out_dir = '.'

    opts, args = getopt(sys.argv[1:],"o:x:")
    for o,v in opts:
        if o == '-o': # output dir
            out_dir = v
        if o == '-x': # exec function
            fx = globals().get(v)
            if fx:
                fx(*args)
            sys.exit()
    if args:
        if_ext = open(args[0])
    if not if_ext:
        sys.exit(2)

    main(if_source, if_ext, out_dir)

def my_connect(cf, sec):
    import configparser as inicfg
    config = inicfg.ConfigParser()
    config.read( cf )
    cnf = dict(config.items( sec ))
    cnf['port'] = int( cnf.get('port', 3306) )
    return mysql.connect( **cnf )

def sql_extra_info(uids):
    mycf = os.getenv('MY_CONF', '/etc/user-info-mysql.conf')
    dbc = my_connect(mycf, 'mysql')
    cursor = dbc.cursor(mysql.cursors.DictCursor) #(cursorclass=mysql.cursors.DictCursor)
    infos = {}

    cond = ' OR '.join('t1.userid={0}'.format(x) for x in uids)
    sql = 'SELECT t1.userid,app_type,nick,app_version FROM token t1 LEFT JOIN users t2 on t1.userid=t2.userid WHERE ' + cond
    # print (sql)
    cursor.execute( sql )
    for r in cursor.fetchall():
        # print (r)
        uid = int( r['userid'] )
        infos[uid] = r;

    cursor.close()
    dbc.close()
    return infos

#_ONLS = (int,None), (int,None), (int,None) #('N:', 'N:', 'N:')
#_MSGS = (int,None), (int,None), (int,None), (int,None), (int,None) #('N:', 'N:', 'N:', 'N:', 'N:')

