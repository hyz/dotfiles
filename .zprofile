
ANDROID_ROOT_PREFIX=/opt/android-

NDK_ROOT=/opt/android-ndk
SDK_ROOT=/opt/android-sdk
#SDK_ROOT=/opt/adt-bundle-linux-x86-20130219/sdk

ANDROID_NDK_ROOT=$NDK_ROOT
ANDROID_SDK_ROOT=$SDK_ROOT
ANDROID_HOME=$SDK_ROOT

export NDK_ROOT SDK_ROOT ANDROID_NDK_ROOT ANDROID_SDK_ROOT ANDROID_HOME

#PATH=$PATH:/sbin:/usr/sbin
#if [ -d $HOME/bin ]; then PATH=$HOME/bin:$PATH fi
for d in /opt/bin $NDK_ROOT $NDK_ROOT/tools $SDK_ROOT/platform-tools ; do
    if [ -d "$d" ]; then
        PATH=$PATH:$d
    fi
done
export PATH

export COCOS2DX_ROOT=$HOME/cocos2d
# /opt/adt-bundle-linux-x86-20130219/sdk

# export BOOST_BUILD_PATH=/usr/share/boost/build/v2

### git clone git://github.com/zsh-users/zsh-completions.git
#fpath=($HOME/zsh-completions/src $fpath)

if [ "`uname -o`" = "Cygwin" ]; then
    export GNUPGHOME="F:\cyghome\.gnupg"
    alias er='explorer "`cygpath -w $(pwd)`" &'
fi

export PYTHONSTARTUP=$HOME/.pythonstartup

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

