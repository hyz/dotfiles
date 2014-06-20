# export HOSTNAME=devel

#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
#RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

export LANG=en_US.UTF-8

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

setopt interactive_comments
bindkey "\e#" vi-pound-insert
# bindkey "\eq" push-line

#unalias run-help
#autoload run-help

# alias cp='cp -i'
alias mv='mv -i'
alias rm='rm -i'
alias ls='ls -F --color=auto'
alias ll='ls -l'
alias grep='grep --color=auto'
alias df='df -h'

alias todo='todo --database-loaders binary'

alias svndiff='svn diff --diff-cmd wsvndiff'

alias b2='b2 -j5'

export EDITOR=vim

# limit coredumpsize 0

#export PATH=$PATH:"/cygdrive/c/Documents and Settings/wood/Local Settings/Application Data/Android/android-sdk/tools":"/cygdrive/c/Documents and Settings/wood/Local Settings/Application Data/Android/android-sdk/platform-tools"
# /cygdrive/c/Documents and Settings/wood/Local Settings/Application Data/Android/android-sdk

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
for d in /opt/bin $SDK_ROOT/tools $SDK_ROOT/platform-tools $NDK_ROOT $NDK_ROOT/tools ; do
    if [ -d "$d" ]; then
        PATH=$PATH:$d
    fi
done
export PATH

export COCOS2DX_ROOT=$HOME/cocos2d
# /opt/adt-bundle-linux-x86-20130219/sdk

# export BOOST_BUILD_PATH=/usr/share/boost/build/v2


