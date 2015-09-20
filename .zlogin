
echo "zlogin" >> /tmp/zsh.txt

if which keychain ; then
    eval `keychain --eval $(find .ssh/*id_[dr]sa)`
    #for x in .ssh/*.id_[dr]sa ; do
    #    [ -r ".ssh/$x" ] && eval `keychain --eval $x`
    #done
fi

#if which ssh-agent ; then
#    # SSHAGENT=/usr/sbin/ssh-agent ; SSHAGENTARGS="-s"
#    if [ -z "$SSH_AUTH_SOCK" ]; then
#      eval `ssh-agent -s`
#      trap "kill $SSH_AGENT_PID" 0
#      [ -r ".ssh/office.id_rsa" ] && ssh-add .ssh/office.id_rsa
#    fi
#fi

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

if [[ -z "$DISPLAY" ]] && [[ "$(tty)" = /dev/tty1 ]]; then
    if which startx 2>/dev/null ; then
        startx #; logout
    elif which xinit 2>/dev/null ; then
        xinit #; logout
    fi
fi

