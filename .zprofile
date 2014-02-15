# if which ssh-agent ; then
#     # SSHAGENT=/usr/sbin/ssh-agent ; SSHAGENTARGS="-s"
#     if [ -z "$SSH_AUTH_SOCK" ]; then
#       eval `ssh-agent -s`
#       trap "kill $SSH_AGENT_PID" 0
#       [ -r ".ssh/office.id_rsa" ] && ssh-add .ssh/office.id_rsa
#     fi
# fi

# Start the GnuPG agent and enable OpenSSH agent emulation
if which gpg-agent ; then
    gnupginf="${HOME}/.gpg-agent-info"

    if pgrep -x -u "${USER}" gpg-agent >/dev/null 2>&1; then
        eval `cat $gnupginf`
        eval `cut -d= -f1 $gnupginf | xargs echo export`
    else
        eval `gpg-agent -s --enable-ssh-support --daemon --write-env-file "$gnupginf"`
    fi
fi

