#!/bin/sh

for d in moon im-trunk ; do
    cd $HOME/backup/$d
    rev=`svn info |awk '/^Revision/{print $2}'`
    svn up
    revup=`svn info |awk '/^Revision/{print $2}'`
    if [ "$rev" = "$revup" ]; then
        exit 0
    fi

    cd $HOME
    find backup/$d -name '.svn' -prune -o -print |cpio -o |gzip > Dropbox/backup/$(date +%W)-$d.cpio.gz
done

