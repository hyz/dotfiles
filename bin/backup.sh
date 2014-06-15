#!/bin/sh

for repo in moon im-trunk ; do
    cd $HOME/backup/$repo
    rev=`svn info |awk '/^Revision/{print $2}'`
    svn up
    revup=`svn info |awk '/^Revision/{print $2}'`
    if [ "$rev" = "$revup" ]; then
        exit 0
    fi

    cd $HOME
    find backup/$repo -name '.svn' -prune -o -print |cpio -o |gzip > Dropbox/backup/$(date +%W)-$repo.cpio.gz
done

