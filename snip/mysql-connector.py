# -*- coding: utf-8 -*-
import os , sys , re, time , functools # , MySQLdb
import datetime
import mysql.connector

def userid_diff(c):
    c.execute("SELECT UserId FROM users")
    id_users = set( l['UserId'] for l in c.fetchall() )

    c.execute("SELECT user_id FROM clients")
    id_clients = set( l['user_id'] for l in c.fetchall() )

    # for id in id_users - id_clients: print id
    for x in id_clients:
        if x not in id_users:
            print(x)
            # c.execute("delete from clients where user_id=%s" % id)

    # user_eq_dict[user] =n= user_eq_dict.setdefault(user, 0) + 1

def init_clis(c):
    c.execute("SELECT user_id as userid,id as token,mac,spot,act_time as atime,cache FROM clients")
    for l in c.fetchall():
        c.execute('delete from clis')
        x="INSERT INTO clis(userid,token,mac,spot,cache,atime)" \
                " VALUES(%(userid)s, '%(token)s', '%(mac)s', '%(spot)s', '%(cache)s', '%(atime)s')" % l
        c.execute(x)
    c.execute('rename table clients to oldclients')
    c.execute('rename table clis to clients')

def rename_guest(c):
    import re
    c.execute("SELECT UserName FROM users where username like 'guest%'")
    for l in c.fetchall():
        rv = re.match('guest(\d{4,7})', l['UserName'])
        if not rv:
            continue
        c.execute("update users set UserName='#{0}' where UserName='{1}'".format(rv.group(1), l['UserName']))

def count_messages(c, args):
    # if len(sys.argv) <= 1: sys.exit(1)
    # select M.id,M.UserId,U.nick,count(M.id) as Count from messages M inner join users U on M.UserId=U.UserId where M.msgtime>'2013-11-27 00:00:00' group by M.userid order by Count desc
    #"select UserId,count(id) as Count from messages where msgtime>'{0} 00:00:00' group by userid order by Count desc"
    date = args.get('date', time.strftime("%F", time.localtime(time.time() - 60*60)))
    print (' |' + date)
    c.execute("SELECT M.UserId,U.nick,count(M.id) AS Count"
            " FROM messages M INNER JOIN users U ON M.UserId=U.UserId"
            " WHERE U.UserName NOT LIKE '#%' AND DATE_FORMAT(M.msgtime,'%Y-%m-%d')='{0}'"
            " GROUP BY M.userid ORDER BY Count DESC"
            .format(date)
            )
            #" M.msgtime>'{0} 00:00:00' group by M.userid order by Count desc"
    for l in c.fetchall():
        if l['Count'] < 10 or l['UserId'] == 1000:
            continue
        print ("{0}|{1}|{2}".format(l['UserId'], l['nick'].encode('utf-8'), l['Count']))

def del_messages(c, args):
    gid = args.get('group')
    sql = "SELECT id" " FROM messages" " WHERE sessionid='{0}'" .format(gid)
    print (sql)
    c.execute(sql)
    for l in c.fetchall():
        c.execute('delete from UnpushedMessages where MsgId={0}'.format(l['id']))

def resetpwd_xDDDD(c, args):
    sql = "SELECT UserName,password FROM users WHERE username LIKE 'x%' AND password NOT LIKE '1234x%'"
    print (sql)
    c.execute(sql)
    for l in c.fetchall():
        if re.match('x\d\d\d\d', l['UserName']):
            sql = "UPDATE users SET password='1234{UserName}' WHERE UserName='{UserName}'".format(**l)
            print (sql)
            c.execute( sql )

def guest_to_guser(c, args):
    args.setdefault('min-uid', 0)
    sql = "SELECT UserId,UserName,sex FROM users WHERE username LIKE '#%' AND sex='M' AND UserId>{min-uid}".format(**args)
    print (sql)
    c.execute(sql)
    for l in c.fetchall():
        sql = "UPDATE users SET UserName='g{UserId}', password='1234g{UserId}' WHERE UserId='{UserId}'".format(**l)
        print (sql)
        c.execute( sql )

def dump_guser(c, args):
    args.setdefault('min-uid', 0)
    sql = "SELECT UserId,UserName,password,sex FROM users WHERE username LIKE 'g%' AND sex='M' AND UserId>{min-uid}".format(**args)
    print (sql)
    c.execute(sql)
    for l in c.fetchall():
        if '1234'+l['UserName'] == l['password'] and re.match('g\d{5}', l['UserName']):
            print ('{UserName} {password}'.format(**l))

def del_users_except(c, args):
    args.setdefault('min-uid', 0)
    sql = "SELECT UserId,UserName,password,sex FROM users WHERE username LIKE 'g%' AND sex='M' AND UserId>{min-uid}".format(**args)
    print (sql)
    c.execute(sql)
    for l in c.fetchall():
        if '1234'+l['UserName'] == l['password'] and re.match('g\d{5}', l['UserName']):
            print ('{UserName} {password}'.format(**l))

def bar_activity_transfer(c, args):
    f = open('bar_activities')
    ks = f.readline().strip().split('\t')
    for i, x in enumerate(ks):
        if x == 'flag':
            ks[i] = 'flags'
        elif x in ('imgs', 'shows', 'is_exhibition'):
            ks[i] = None
    for l in f.readlines():
        vals = [ x.strip(' \r\n') for x in l.split('\t') ]
        for i, x in enumerate(vals):
            if x != 'NULL':
                vals[i] = "'%s'" % x
                if ks[i] == 'flags':
                    vals[i] = '1'
        #id  activity_name   post_img    brief   imgs    tm_start    tm_end  result  flag
        m = dict( [ (x,y) for x,y in zip(ks, vals) if x != None ] )
        s_keys = ','.join(['%s' % x for x in m.keys()])
        fmt = ','.join(['{%s}' % x for x in m.keys()])
        s_vals = fmt.format(**m)
        c.execute('INSERT INTO bar_activity({0}) VALUES({1})'.format(s_keys, s_vals) )

def bar_activity2_transfer(c, args):
    bars = ListCSV('my/data.bars')
    acts = MapCSV('my/data.bar_activity', 'id')
    for bar in bars:
        la = bar['activitie_id_list'].strip()
        if not la:
            continue
        for x in la.split():
            cols = acts[x].copy()
            del cols['id']
            cols['SessionId'] = bar['SessionId']
            #print cols
            ExecSql(c, 'INSERT INTO bar_activity2({0}) VALUES({1})', cols)

def ExecSql(cursor, fmt, maps):
    def red(p, kv):
        if kv:
            k,v = kv
            #if v != 'NULL': v = "'%s'" % v
            p[1].append(v)
            p[0].append(k)
        return p
    keys,vals = functools.reduce(red, maps.items(), ([],[]))
    sql = fmt.format(','.join(keys) , '%%(%s)s' % ')s,%('.join(keys))
    print (maps)
    cursor.execute( sql, maps )

def ListCSV(filename, keyfield=None):
    lm = []
    f = open(filename)
    ks = f.readline().strip().split('\t')
    for l in f.readlines():
        vals = [ x.strip(' \r\n') for x in l.split('\t') ]
        for i, x in enumerate(vals):
            if x == 'NULL':
                vals[i] = None
        lm.append( dict( zip(ks, vals) ) )
    if not keyfield:
        return lm
    md = {}
    for x in lm:
        md[x[keyfield]] = x
    return md
def MapCSV(filename, keyfield):
    return ListCSV(filename, keyfield)
def rename_k(m, fr, to):
    val = m[fr]
    del m[fr]
    m[to] = val

def transfertab_clients(args, cursor, dbc):
    clients = ListCSV('my/data.clients')
    #acts = ListCSV('my/data.bar_activity', 'id')
    for m in clients:
        cursor.execute('select 1 from users where UserId=%(user_id)s', m)
        if cursor.fetchone():
            rename_k(m, 'user_id', 'UserId')
            rename_k(m, 'id', 'Token')
            rename_k(m, 'act_time', 'ATime')
            from datetime import datetime
            m['ATime'] = datetime.now().strftime("%F %T")
            import json
            if m['cache']:
                jd = json.loads(m['cache'])
                x = jd.get('aps_token',None)
                if x:
                    m['AppleDevToken'] = x
            del m['cache']
            ExecSql(dbc.cursor(), 'INSERT INTO client({0}) VALUES({1})', m)

def _function(fn):
    def not_found(*a):
        print ('Not found {}'.format(fn))
    return globals().get(fn, not_found)

def mcnx():
    cnx = mysql.connector.connect(user='scott', database='employees')
    cursor = cnx.cursor()

    query = ("SELECT first_name, last_name, hire_date FROM employees "
             "WHERE hire_date BETWEEN %s AND %s")

    hire_start = datetime.date(1999, 1, 1)
    hire_end = datetime.date(1999, 12, 31)

    cursor.execute(query, (hire_start, hire_end))

    for (first_name, last_name, hire_date) in cursor:
      print("{}, {} was hired on {:%d %b %Y}".format(
        last_name, first_name, hire_date))

    cursor.close()
    cnx.close()

    # cursor.execute("SELECT last_name, first_name, hire_date "
    #                "FROM employees WHERE emp_no = %s", (123,))
    # row = dict(zip(cursor.column_names, cursor.fetchone())
    # print("{last_name}, {first_name}: {hire_date}".format(row))

def mysql_connector(args, cursor, dbc):
    cursor.execute("select * from users")
    for a in cursor:
        print (a)

### ### ### ### ### ### ### ### ### ### ### ###
if __name__ == '__main__':
    if len(sys.argv) < 2:
        sys.exit(2)

    args = {}
    if len(sys.argv) > 2:
        args = dict(p.split('=', 1) for p in sys.argv[2:])

    import configparser

    config = configparser.ConfigParser()
    config.read(os.getenv("MY_CONF", "/etc/moon.conf"))

    dbcfg  = dict(config.items('database'))
    host   = dbcfg.get('host')
    port   = dbcfg.get('port')
    user   = dbcfg.get('user')
    passwd = dbcfg.get('password')
    db     = dbcfg.get('db')

    dbc = mysql.connector.connect(user=user, passwd=passwd, database=db, host=host, charset='utf8')
    cursor = dbc.cursor()

    _function(sys.argv[1])(args, cursor, dbc)

    dbc.commit()
    cursor.close()
    dbc.close()

    # numrows = int(c.rowcount)
    # for x in range(0,numrows):
    #     row = c.fetchone()
    #     print row
    # "UPDATE users SET UserName='weibo%d' where UserId=%d" % (id, id))


