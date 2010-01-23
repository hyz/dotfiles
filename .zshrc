
#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT='%{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T %{[32m%}%/%{[31m%}$%{[m%} '
#RPROMPT='%{[31m%}$%{[m%}'
PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

export LANG=en_US.UTF-8
bindkey -v
export PATH=$PATH:$HOME/bin:/sbin:/usr/sbin

export HISTSIZE=1000
export SAVEHIST=1000
export HISTFILE=$HOME/.zsh/history

alias cp='cp -i'
alias mv='mv -i'
alias rm='rm -i'
alias ls='ls -F --color=auto'
alias ll='ls -l'
alias grep='grep --color=auto'
alias df='df -h'

limit coredumpsize 0

autoload -U edit-command-line
zle -N edit-command-line
bindkey -M vicmd v edit-command-line

autoload -U compinit; compinit

# function zle-line-init zle-keymap-select {
#     RPS1="${${KEYMAP/vicmd/-- NORMAL --}/(main|viins)/-- INSERT --}"
#     RPS2=$RPS1
#     zle reset-prompt
# }
# zle -N zle-line-init
# zle -N zle-keymap-select

