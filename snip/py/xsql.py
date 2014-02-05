# -*- coding: utf-8 -*-

import os , sys , re, time , MySQLdb

def userid_diff(c):
    c.execute("SELECT UserId FROM users")
    id_users = set( l['UserId'] for l in c.fetchall() )

    c.execute("SELECT user_id FROM clients")
    id_clients = set( l['user_id'] for l in c.fetchall() )

    # for id in id_users - id_clients: print id
    for x in id_clients:
        if x not in id_users:
            print x
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
    print ' |', date
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
        print "{0}|{1}|{2}".format(l['UserId'], l['nick'].encode('utf-8'), l['Count'])

def del_messages(c, args):
    gid = args.get('group')
    sql = "SELECT id" " FROM messages" " WHERE sessionid='{0}'" .format(gid)
    print sql
    c.execute(sql)
    for l in c.fetchall():
        c.execute('delete from UnpushedMessages where MsgId={0}'.format(l['id']))

def resetpwd_xDDDD(c, args):
    sql = "SELECT UserName,password FROM users WHERE username LIKE 'x%' AND password NOT LIKE '1234x%'"
    print sql
    c.execute(sql)
    for l in c.fetchall():
        if re.match('x\d\d\d\d', l['UserName']):
            sql = "UPDATE users SET password='1234{UserName}' WHERE UserName='{UserName}'".format(**l)
            print sql
            c.execute( sql )

def guest_to_guser(c, args):
    sql = "SELECT UserId,UserName,sex FROM users WHERE username LIKE '#%' AND sex='M'"
    print sql
    c.execute(sql)
    for l in c.fetchall():
        sql = "UPDATE users SET UserName='g{UserId}', password='1234g{UserId}' WHERE UserId='{UserId}'".format(**l)
        print sql
        c.execute( sql )

def dump_guser(c, args):
    sql = "SELECT UserId,UserName,password,sex FROM users WHERE username LIKE 'g%' AND sex='M'"
    print sql
    c.execute(sql)
    for l in c.fetchall():
        if '1234'+l['UserName'] == l['password'] and re.match('g\d{5}', l['UserName']):
            print '{UserName} {password}'.format(**l)

default_func = lambda x,y: sys.stdout.write( '{0} {1}'.format(x,y) )

### ### ### ### ### ### ### ### ### ### ### ###
hello = default_func

if __name__ == '__main__':
    def find_func(fn):
        fp = globals().get(fn)
        if not fp:
            if fn == '0':
                fp = default_func
        return fp

    if len(sys.argv) < 2:
        sys.exit(2)
    func = find_func(sys.argv[1])
    if not func:
        sys.exit(3)
    args = {}
    if len(sys.argv) > 2:
        args = dict(p.split('=', 1) for p in sys.argv[2:])

    import ConfigParser

    config = ConfigParser.ConfigParser()
    config.read(os.environ.get("MY_CONF", "/etc/moon.conf"))

    dbcfg  = dict(config.items('database'))
    host   = dbcfg.get('host')
    port   = dbcfg.get('port')
    user   = dbcfg.get('user')
    passwd = dbcfg.get('password')
    db     = dbcfg.get('db')

    dbc = MySQLdb.connect(host=host, user=user, passwd=passwd, db=db, charset='utf8')
    func(dbc.cursor(cursorclass=MySQLdb.cursors.DictCursor), args)
    dbc.commit()

    # numrows = int(c.rowcount)
    # for x in range(0,numrows):
    #     row = c.fetchone()
    #     print row

    # "UPDATE users SET UserName='weibo%d' where UserId=%d" % (id, id))



