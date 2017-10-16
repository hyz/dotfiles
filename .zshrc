
[[ -n "$Zdbg" ]] && echo "#zshrc" >> $Zdbg

#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
#RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

#fpath+=~/.zfunc
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


#alias tmux='TERM=xterm-256color tmux'
alias tmux='tmux -2'

##alias for cnpm
#alias cnpm="npm --registry=https://registry.npm.taobao.org \
#  --cache=$HOME/.npm/.cache/cnpm \
#  --disturl=https://npm.taobao.org/dist \
#  --userconfig=$HOME/.cnpmrc"

# limit coredumpsize 0

ANDROID_SDK=/opt/android-sdk
if [[ -d "$ANDROID_SDK" ]] ; then
    #ANDROID_SDK_HOME=$ANDROID_SDK
    ANDROID_HOME=$ANDROID_SDK
    export ANDROID_SDK ANDROID_HOME #ANDROID_SDK_HOME

    PATH=$PATH:$ANDROID_SDK/tools:$ANDROID_SDK/platform-tools
fi

ANDROID_NDK=/opt/android-ndk
if [ -d "$ANDROID_NDK" ]; then
    ANDROID_NDK_HOME=$ANDROID_NDK
    export ANDROID_NDK ANDROID_NDK_HOME

    PATH=$PATH:$ANDROID_NDK #:$ANDROID_NDK/standalone/toolchain/android-12/bin
fi

# /usr/local/bin 
for x in /opt/bin $HOME/.cargo/bin $HOME/go/bin $HOME/.yarn/bin ; do
    [ -d "$x" ] || continue
    PATH=$PATH:$x
done
if [ -d "$HOME/go" ]; then
    export GOPATH=$HOME/go
fi

#export PATH

date

