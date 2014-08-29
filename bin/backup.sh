#!/bin/sh

exec >/tmp/backup.sh.log 2>&1

which svn

for repo in moon im-trunk ; do
    cd $HOME/backup/$repo
    if svn up |grep '^U\>' >/dev/null ; then
        cd $HOME
        find backup/$repo \( -name '.svn' -o -name 'mongo' \) -prune -o -print \
            | cpio -o | gzip > Dropbox/backup/$(date +%W)-$repo.cpio.gz
    fi
done

