#!/bin/bash

die() {
    echo "$*" >&2
    exit 1
}

if [ $# -eq 0 ] ; then
    exec /bin/bash $0 .
fi

cd "$1" || die "cd: $1"
if [ ! -d .git ] ; then
    if [ ! -d ../.git ] ; then
        die "not found: ...git"
    fi
    cd .. || die "not found: .git"
fi

#test -d .git || die "not found: .git"
origin="`git remote get-url origin`"
echo "#... `pwd`    $origin"
git fetch -p && git merge 
#git fetch -p #--rebase && git submodule update --init --recursive
#git merge # pull --rebase && git submodule update --init --recursive
echo "#~~~ `pwd`    $origin"
echo

exit ###

Found=
for d in `find $* -type d -name .git -prune`; do
    cat <<_EoF |/bin/bash
        cd "$d"/..
        /bin/bash $0
_EoF
    Found=y
done
[ -z "$Found" ] 
