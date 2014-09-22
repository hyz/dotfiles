#!/bin/sh

exec >/tmp/backup.sh.log 2>&1

which svn

for repo in moon im-trunk ; do
    cd $HOME/backup/$repo
    if svn up |grep '^[UA]\>' >/dev/null ; then
        cd $HOME
        fp=Dropbox/backup/$((`date +%W` % 10))-$repo.cpio.gz
        find backup/$repo \( -name '.svn' -o -name 'mongo' \) -prune -o -print | cpio -o | gzip > $fp
        fpx=Dropbox/backup/`date +%w`-$repo.cpio.gz
        rm -f $fpx
        ln $fp $fpx
    fi
done

