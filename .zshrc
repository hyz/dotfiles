
PROMPT='%{[36m%}%/%(!.#.)%{[m%} %% '
RPROMPT='%{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '

export LANG=en_US.UTF-8
bindkey -v
export PATH=$PATH:$HOME/bin

#PROMPT='%{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T %{[32m%}%/%{[31m%}$%{[m%} '
#RPROMPT='%{[31m%}$%{[m%}'

export HISTSIZE=1000
export SAVEHIST=1000
export HISTFILE=$HOME/.zsh_history

alias cp='cp -i'
alias mv='mv -i'
alias rm='rm -i'
alias ls='ls -F --color=auto'
alias ll='ls -l'
alias grep='grep --color=auto'

limit coredumpsize 0

# function zle-line-init zle-keymap-select {
#     RPS1="${${KEYMAP/vicmd/-- NORMAL --}/(main|viins)/-- INSERT --}"
#     RPS2=$RPS1
#     zle reset-prompt
# }
# zle -N zle-line-init
# zle -N zle-keymap-select

