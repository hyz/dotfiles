# -*- coding: utf-8 -*-
# -*- coding: utf-8 -*-
import os , sys , MySQLdb

def userid_diff(c):
    c.execute("SELECT UserId FROM users")
    id_users = set( l['UserId'] for l in c.fetchall() )

    c.execute("SELECT user_id FROM clients")
    id_clients = set( l['user_id'] for l in c.fetchall() )

    # for id in id_users - id_clients: print id
    for id in id_clients:
        if id not in id_users:
            print id
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

main = init_clis
#
##################################################################
if __name__ == '__main__':
    import ConfigParser

    cfgfile = "db.conf"
    if len(sys.argv) > 1:
        cfgfile = sys.argv[1]

    config = ConfigParser.ConfigParser()
    config.read(cfgfile)

    dbcfg  = dict(config.items('database'))
    host   = dbcfg.get('host')
    port   = dbcfg.get('port')
    user   = dbcfg.get('user')
    passwd = dbcfg.get('password')
    db     = dbcfg.get('db')

    dbc = MySQLdb.connect(host=host, user=user, passwd=passwd, db=db)
    main(dbc.cursor(cursorclass=MySQLdb.cursors.DictCursor))
    dbc.commit()

    # numrows = int(c.rowcount)
    # for x in range(0,numrows):
    #     row = c.fetchone()
    #     print row

    # "UPDATE users SET UserName='weibo%d' where UserId=%d" % (id, id))


