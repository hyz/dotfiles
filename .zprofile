
#locale >> /tmp/zsh.$USER
#env >> /tmp/zsh.$USER
echo "zprofile" >> /tmp/zsh.$USER

export LANG=en_US.UTF-8
export EDITOR=vim

if [[ -d "$HOME/bin" ]] ; then
    PATH=$HOME/bin:/bin:/usr/bin
else
    PATH=/bin:/usr/bin
fi
for x in /usr/local/bin /opt/bin $HOME/.cargo/bin ; do
    if [[ -d "$x" ]] ; then
        PATH=$PATH:$x
    fi
done

PATH=$PATH:/sbin:/usr/sbin
export PATH

