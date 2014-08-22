#!/bin/sh

exec >/tmp/backup.sh.log 2>&1

for repo in moon im-trunk ; do
    cd $HOME/backup/$repo
    rev=`svn info |awk '/^Revision/{print $2}'`
    /usr/bin/svn up
    revup=`svn info |awk '/^Revision/{print $2}'`
    if [ "$rev" != "$revup" ]; then
        cd $HOME
        find backup/$repo \( -name '.svn' -o -name 'mongo' \) -prune -o -print \
            | cpio -o | gzip > Dropbox/backup/$(date +%W)-$repo.cpio.gz
    fi
done

