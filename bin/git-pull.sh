#!/bin/bash

die() {
    echo "$*" >&2
    exit 1
}

if [ $# -eq 0 ] ; then
    exec /bin/bash $0 .
fi

cd "$1" || die "cd: $1"
while [ ! -d .git ] ; do
    cd .. || die "not found: .git"
done

#test -d .git || die "not found: .git"
echo "+++ `pwd`: `git remote get-url origin`"
git pull --rebase && git submodule update --init --recursive
echo "--- `pwd`"
echo
exit

Found=
for d in `find $* -type d -name .git -prune`; do
    cat <<_EoF |/bin/bash
        cd "$d"/..
        /bin/bash $0
_EoF
    Found=y
done
[ -z "$Found" ] 
