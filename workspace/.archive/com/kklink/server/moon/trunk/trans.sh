#!/bin/sh

# if [ -f  /home/lindu/moon/trunk/moon.gz ]; then
#     rm  /home/lindu/moon/trunk/moon.gz
# fi
# 
# cp  /home/lindu/moon/trunk/bin/gcc-4.4.7/debug/threading-multi/moon /home/lindu/moon/trunk/
# 
# gzip  /home/lindu/moon/trunk/moon
# 
# 
# scp -P 44331 moon.gz root@58.67.160.246:~/ 

# tar vczf - $* |ssh -p 44331 root@58.67.160.246 'tar vxzf - -C / ; killall moon ; moon -d -c /etc/moon.conf'

# tar vczf - $* |ssh -p 44331 root@58.67.160.246 'tar vxzf - -C /'

tmpf="/tmp/`basename $1`-`basename $0 .sh`$$-`date +%F`.tgz"
echo $tmpf

if which pv >/dev/null ; then
    tar vczf "$tmpf" `find $@ -name '.svn' -prune -o -type f -print`
    pv -s `stat -c "%s" "$tmpf"` "$tmpf" | ssh -p 44331 root@58.67.160.244 "cat >\"$tmpf\" && tar xzf \"$tmpf\" -C /"
    rm -f "$tmpf"
    exit 0
fi

# tar vczf - $@ | ssh -p 44331 root@58.67.160.246 'cat >$tmpf && tar xzf $tmpf -C /'

