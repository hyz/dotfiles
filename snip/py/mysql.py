# -*- coding: utf-8 -*-

import MySQLdb

db = MySQLdb.connect(host="127.0.0.1", user="lindu", passwd="lindu12345", db="lindu_moon")
c = db.cursor()

c.execute("SELECT * FROM users")

for l in c.fetchall():
    print l

# numrows = int(c.rowcount)
# for x in range(0,numrows):
#     row = c.fetchone()
#     print row

