if which ssh-agent ; then
    # SSHAGENT=/usr/sbin/ssh-agent ; SSHAGENTARGS="-s"
    if [ -z "$SSH_AUTH_SOCK" ]; then
      eval `ssh-agent -s`
      trap "kill $SSH_AGENT_PID" 0
      [ -r ".ssh/office.id_rsa" ] && ssh-add .ssh/office.id_rsa
    fi
fi
env > /tmp/w

