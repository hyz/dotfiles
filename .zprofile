export EDITOR=vim

if [ "`uname -o`" = "Cygwin" ]; then
    export GNUPGHOME="F:\cyghome\.gnupg"
fi

for x in $HOME/bin /opt/bin /usr/local/bin ; do
    if [ -d "$x" ]; then
        PATH=$PATH:$x
    fi
done
export PATH

if which keychain ; then
    for x in office.id_rsa ; do
        [ -r ".ssh/$x" ] && eval `keychain --eval $x`
    done
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

# limit coredumpsize 0

