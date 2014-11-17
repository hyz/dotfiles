
#locale >> /tmp/zsh.txt
#env >> /tmp/zsh.txt
echo "zprofile" >> /tmp/zsh.txt

export LANG=en_US.UTF-8
export EDITOR=vim

if [[ -d "$HOME/bin" ]] ; then
    PATH=$HOME/bin:/bin:/usr/bin
else
    PATH=/bin:/usr/bin
fi
for x in /usr/local/bin /opt/bin ; do
    if [[ -d "$x" ]] ; then
        PATH=$PATH:$x
    fi
done
PATH=$PATH:/sbin:/usr/sbin

if [ "`/bin/uname -o`" = "Cygwin" ]; then
    PATH=$PATH:/cygdrive/c/Windows:"/cygdrive/e/Program Files (x86)/GNU/GnuPG"
fi

export PATH

