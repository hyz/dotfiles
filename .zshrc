export LANG=en_US.UTF-8

#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
#RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

autoload -U compinit promptinit
compinit
promptinit
 
prompt walters

export PATH=$PATH:$HOME/bin:/sbin:/usr/sbin

export HISTSIZE=1000
export SAVEHIST=1000
export HISTFILE=$HOME/.zsh/history

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

# limit coredumpsize 0

