
echo "zshrc" >> /tmp/zsh.$USER

#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
#RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

autoload -U compinit promptinit
compinit
promptinit
 
prompt walters
# cat /usr/share/zsh/functions/Prompts/prompt_walters_setup
if [[ "$TERM" = "dumb" ]]; then
    export PROMPT="%(?..[%?] )%n@%m:%~> "
else
    IPAddr=`ip addr show dev $(ip route |/bin/grep -Po '^default.*dev \K\w+') |/bin/grep -Po '\s+inet \d+.\d+.\d+.\K\d+'`
    export  PROMPT="%(?..[%?] )%n@${IPAddr}> "
    export RPROMPT="%~ %(t.Ding!.%D{%L:%M})"
fi

export HISTSIZE=10000
export SAVEHIST=1000
export HISTFILE=$HOME/.zhistory
export HISTIGNORE="&:ls:pwd:[bf]g:exit:reset:clear"
# export HISTIGNORE="&:ls:[bf]g:exit:reset:clear:cd*"
# export HISTFILE=~/.zsh_history
# export SAVEHIST=250
setopt APPEND_HISTORY
setopt INC_APPEND_HISTORY
setopt HIST_IGNORE_ALL_DUPS
setopt HIST_IGNORE_SPACE
setopt HIST_REDUCE_BLANKS
setopt HIST_SAVE_NO_DUPS
setopt HIST_VERIFY

bindkey -v
autoload -U        edit-command-line
zle -N             edit-command-line
bindkey -M vicmd v edit-command-line
export KEYTIMEOUT=20 # Fri, Nov 21, 2014  8:36:52 AM

setopt interactive_comments
bindkey "\e#" vi-pound-insert
# bindkey "\eq" push-line

#unalias run-help
#autoload run-help

# alias cp='cp -i'
alias mv='mv -i'
alias rm='rm -i'
alias grep='grep --color=auto'
alias df='df -Th'

if ls -d --color=auto >/dev/null 2>&1 ; then
    alias ls='ls -F --color=auto'
else
    # export CLICOLOR=1
    alias ls='ls -F' # alias ls='ls -F --color=auto'
fi
alias ll='ls -l'

#
#
alias svndiff='svn diff --diff-cmd wsvndiff'

#if which curl 2>/dev/null ; then
#    which wget 2>/dev/null || alias wget='curl -O'
#fi

###########################################################
### git clone git://github.com/zsh-users/zsh-completions.git
#fpath=($HOME/zsh-completions/src $fpath)

#which todo 2>/dev/null && alias todo='todo --database-loaders binary'

# # export BOOST_BUILD_PATH=/usr/share/boost/build/v2
if [ -d /opt/share/boost-build ] ; then
    export BOOST_BUILD_PATH=/opt/share/boost-build
fi
#if x=`which b2 2>/dev/null` ; then
#    alias b2="$x -sBOOST_ROOT=/BOOST_ROOT -j5"
#fi

[ -d "/tmp/.home/$USER" ] || mkdir -p "/tmp/.home/$USER"
[ -d "/tmp/.$USER" ] || mkdir "/tmp/.$USER"

case "`uname -s`" in
Darwin)
    true
    ;;
CYGWIN*)
    PATH=$PATH:$HOME/bin/cygwin
    /bin/ls -d1 /cygdrive/[cdef]/Program*/GNU/GnuPG |while read x ; do
        PATH=$PATH:"$x"
        #which gpg2
        break
    done
    for x in /cygdrive/{c,d,e,f,g,h,i}/.gnupg ; do
        [ -r "$x/secring.gpg" ] || continue
        export GNUPGHOME=`cygpath -w "$x"`
        break
    done

    _WinDir=`cygpath "$WINDIR"`
    PATH=$PATH:$_WinDir/system32:$_WinDir

    #alias er='explorer "`cygpath -w $(pwd)`" &'
    er() {
        if x=`/bin/ls -1d "$1" || /bin/ls -1d "$(pwd)/$1"` ; then
            "$_WinDir/explorer" "`cygpath -w "$x"`" &
        fi
        #`cygpath $WINDIR`/explorer "`cygpath -w $(pwd)/$1`" &
    }
    ;;
*)
    # if [ ! -e "$HOME/.gnupg/secring.gpg" ] ; then fi
    #which pass 
    gpghome() {
        [ -r "$1" ] && dirname "$1" || false
    }
    for x in "/media/wood/*" "/mnt/*" "$HOME/mnt" ; do
        if x=`eval gpghome $x/.gnupg/secring.gpg 2>/dev/null` ; then
            export GNUPGHOME="$x"
            break
        fi
    done
    ;;
esac

###########################################################

for x in /opt/bin /opt/local/bin ; do
    [ -d "$x" ] || continue
    PATH=$PATH:$x
done

#alias dict='sdcv -0'
dict() {
    echo $* >> $HOME/.dict_history
    if which sdcv >/dev/null 2>&1; then
        sdcv -0 "$1"
    elif which ydcv >/dev/null 2>&1; then
        ydcv "$1"
    elif which xdg-open >/dev/null 2>&1; then
        xdg-open "http://dict.cn/$1"
    else
        grep "$1" /usr/share/dict/*
    fi
}

google() {
    # echo "Googling: $@"
    [ -n "$*" ] || return 1
    h="173.194.14.53" # 210.242.125.83 # www.google.com
    q=""
    for x in $@; do
        q="$q%20$x"
    done
    xdg-open "http://$h/search?q=$q"
}

[ ! -f "$HOME/.pythonstartup" ] || export PYTHONSTARTUP=$HOME/.pythonstartup

# Add environment variable ANT_ROOT for cocos2d-x
#export ANT_ROOT=/usr/bin

ANDROID_SDK=/opt/android-sdk
if [[ -d "$ANDROID_SDK" ]] ; then
    ANDROID_SDK_HOME=$ANDROID_SDK
    # ANDROID_HOME=$ANDROID_SDK
    export ANDROID_SDK ANDROID_SDK_HOME # ANDROID_HOME

    PATH=$PATH:$ANDROID_SDK/tools:$ANDROID_SDK/platform-tools
fi

ANDROID_NDK=/opt/android-ndk
if [ -d "$ANDROID_NDK" ]; then
    ANDROID_NDK_HOME=$ANDROID_NDK
    export ANDROID_NDK ANDROID_NDK_HOME

    PATH=$PATH:$ANDROID_NDK #:$ANDROID_NDK/standalone/toolchain/android-12/bin
fi

# build/tools/make_standalone_toolchain.py -v --arch=arm --api=17 --stl=gnustl --install-dir=/opt/android/17-arm-gcc-4.9
#if x=`/bin/ls -1d /opt/android/[0-9][0-9]-*-gcc-[0-9].[0-9]/bin |head -1` ; then
#    PATH=$PATH:$x
#fi

# Add environment variable COCOS_CONSOLE_ROOT for cocos2d-x
#COCOS2DX_ROOT=$HOME/cocos2d
#if [ -d "$COCOS2DX_ROOT" ] ; then
#    export COCOS2DX_ROOT
#fi

#COCOS_CONSOLE_ROOT=$HOME/cocos2d-x/cocos2d-x-3.2/tools/cocos2d-console/bin
#if [ -d "$COCOS_CONSOLE_ROOT" ] ; then
#    PATH=$PATH:$COCOS_CONSOLE_ROOT
#    export COCOS_CONSOLE_ROOT
#fi

#if [ -d "/opt/android/studio/bin" ]; then
#    PATH=$PATH:/opt/android/studio/bin
#fi
#if [ -d "/opt/gradle-2.12/bin" ]; then
#    export GRADLE_HOME=/opt/gradle-2.12
#    PATH=$PATH:/opt/gradle-2.12/bin
#fi

# Add environment variable ANT_ROOT for cocos2d-x
#export ANT_HOME=/usr/share/apache-ant
#export ANT_ROOT=/bin

###
if [ -d "$HOME/go" ]; then
    export GOPATH=$HOME/go
    PATH=$PATH:$GOPATH/bin
fi

export PATH

#alias tmux='TERM=xterm-256color tmux'
alias tmux='tmux -2'

##alias for cnpm
#alias cnpm="npm --registry=https://registry.npm.taobao.org \
#  --cache=$HOME/.npm/.cache/cnpm \
#  --disturl=https://npm.taobao.org/dist \
#  --userconfig=$HOME/.cnpmrc"

# limit coredumpsize 0
date
