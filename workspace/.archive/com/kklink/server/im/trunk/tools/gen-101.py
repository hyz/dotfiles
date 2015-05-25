import sys
import string
import getopt
from random import *

fmt='{uid}\t101\t{{"version":"1.0.0","appid":"yx","cmd":101,"auth":"0c4707dab8c186a7bcaffa58abeb2310","body":{{"to":[{{"uid":' \
'{uid},"name":"{name}"}}],"sid":"21/{uid}","data":{{"msg":"{msg}"}}}}}}'

def rands():
    s = []
    for x in range(randint(1,10)):
        s += sample(string.letters, 10)
    return ''.join(s)

def get_users(fn):
    users = []
    with open(fn) as f:
        for lin in f:
            lin = lin.strip()
            v = lin.split('\t')
            users.append( dict(zip(['uid','name'], v[:2])) )
    return users

if __name__ == '__main__':
    if len(sys.argv) > 1:
        users = []
        n_msg = 10

        opts, args = getopt.getopt(sys.argv[1:],"n:")
        for o,v in opts:
            n_msg = int(v)
        users = get_users(args[0])

        for x in range(n_msg):
            i = randint(0, len(users)-1)
            d = dict(users[i])
            d["msg"] = str(x) + rands()
            print fmt.format(**d)

