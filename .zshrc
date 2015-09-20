
echo "zshrc" >> /tmp/zsh.txt

#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
#RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

autoload -U compinit promptinit
compinit
promptinit
 
prompt walters

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
###########################################################
### git clone git://github.com/zsh-users/zsh-completions.git
#fpath=($HOME/zsh-completions/src $fpath)

alias todo='todo --database-loaders binary'

alias svndiff='svn diff --diff-cmd wsvndiff'

alias b2='b2 -j5'
# export BOOST_BUILD_PATH=/usr/share/boost/build/v2

if [ "`uname -s`" = "Darwin" ] ; then
    true
elif [ "`uname -o`" = "Cygwin" ] ; then
    PATH=$PATH:$HOME/bin/cygwin
    /bin/ls -1d /cygdrive/[cd]/*/GNU/GnuPG | while read x ; do
        PATH=$PATH:$x
    done
    export GNUPGHOME=`cygpath -w $HOME/.gnupg`
    #alias er='explorer "`cygpath -w $(pwd)`" &'
    er() {
        if x=`/bin/ls -1d "$1" || /bin/ls -1d "$(pwd)/$1"` ; then
            "`cygpath "$WINDIR"`/explorer" "`cygpath -w "$x"`" &
        fi
        #`cygpath $WINDIR`/explorer "`cygpath -w $(pwd)/$1`" &
    }
fi

###########################################################

PYTHONSTARTUP=$HOME/.pythonstartup
if [ -f "$PYTHONSTARTUP" ] ; then export PYTHONSTARTUP ; fi

# Add environment variable ANT_ROOT for cocos2d-x
#export ANT_ROOT=/usr/bin

SDK_ROOT=/opt/android/sdk
if [[ -d "$SDK_ROOT" ]] ; then
    ANDROID_SDK_ROOT=$SDK_ROOT
    ANDROID_HOME=$SDK_ROOT
    export SDK_ROOT ANDROID_SDK_ROOT ANDROID_HOME

    PATH=$PATH:$SDK_ROOT/tools:$SDK_ROOT/platform-tools
fi

NDK_ROOT=/opt/android/ndk
if [ -d "$NDK_ROOT" ]; then
    ANDROID_NDK_ROOT=$NDK_ROOT
    export NDK_ROOT ANDROID_NDK_ROOT

    PATH=$PATH:$NDK_ROOT
fi

# Add environment variable COCOS_CONSOLE_ROOT for cocos2d-x
#COCOS2DX_ROOT=$HOME/cocos2d
#if [ -d "$COCOS2DX_ROOT" ] ; then
#    export COCOS2DX_ROOT
#fi

COCOS_CONSOLE_ROOT=$HOME/cocos2d-x/cocos2d-x-3.2/tools/cocos2d-console/bin
if [ -d "$COCOS_CONSOLE_ROOT" ] ; then
    PATH=$PATH:$COCOS_CONSOLE_ROOT
    export COCOS_CONSOLE_ROOT
fi

if [ -d "/opt/local/bin" ]; then
    PATH=$PATH:/opt/local/bin:/opt/local/sbin
fi

# Add environment variable ANT_ROOT for cocos2d-x
export ANT_HOME=/usr/share/apache-ant
export ANT_ROOT=/bin

export PATH

# limit coredumpsize 0

#alias dict='sdcv -0'
dict() {
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

if [ ! -d "$HOME/.gnupg" -a ! -d "$GNUPGHOME" ] ; then
    PASS=`which pass`
    pass() {
        _gnupg=$HOME/mnt$HOME/.gnupg
        if [ -d "$_gnupg" ]; then
            GNUPGHOME=$_gnupg $PASS $*
        fi # else $PASS $*
        #export PASSWORD_STORE_DIR=~/mnt/wood/.password-store
        #export PASSWORD_STORE_GIT=$PASSWORD_STORE_DIR
    }
fi

