
[ -d "/tmp/._$USER" ] || mkdir "/tmp/._$USER"
Zdbg="/tmp/._$USER/zsh.log" ; [ -z "$Zdbg" -o -e $Zdbg ] || /bin/env > $Zdbg
[[ -n "$Zdbg" ]] && echo "#zlogin" >> $Zdbg

if which keychain ; then
    eval `keychain --agents ssh --eval $(find .ssh/*id_[dr]sa)`
    ## find .ssh/id_rsa* .ssh/*id_rsa ! -name "*pub"
    #for x in .ssh/*.id_[dr]sa ; do
    #    [ -r ".ssh/$x" ] && eval `keychain --eval $x`
    #done
elif which ssh-agent ; then
    # SSHAGENT=/usr/sbin/ssh-agent ; SSHAGENTARGS="-s"
    if [ -z "$SSH_AUTH_SOCK" ]; then
        eval `ssh-agent -s`
        trap "kill $SSH_AGENT_PID" 0
        ssh-add $(find .ssh/*id_[dr]sa)
        #[ -r ".ssh/office.id_rsa" ] && ssh-add .ssh/office.id_rsa
    fi
fi

# Start the GnuPG agent and enable OpenSSH agent emulation
# if which gpg-agent ; then
#     gnupginf="${HOME}/.gpg-agent-info"
# 
#     if pgrep -x -u "${USER}" gpg-agent >/dev/null 2>&1; then
#         eval `cat $gnupginf`
#         eval `cut -d= -f1 $gnupginf | xargs echo export`
#     else
#         eval `gpg-agent -s --enable-ssh-support --daemon --write-env-file "$gnupginf"`
#     fi
# fi

# [[ "$(tty)" = /dev/tty1 ]]
if [ -z "$DISPLAY" ] && [ -n "$XDG_VTNR" ] && [ "$XDG_VTNR" -eq 1 ]; then
    export PATH
    if which startx 2>/dev/null ; then
        exec startx
    elif which xinit 2>/dev/null ; then
        exec xinit
    fi
fi

