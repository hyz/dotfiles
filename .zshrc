# Enable Powerlevel10k instant prompt. Should stay close to the top of ~/.zshrc.
# Initialization code that may require console input (password prompts, [y/n]
# confirmations, etc.) must go above this block; everything else may go below.
if [[ -r "${XDG_CACHE_HOME:-$HOME/.cache}/p10k-instant-prompt-${(%):-%n}.zsh" ]]; then
  source "${XDG_CACHE_HOME:-$HOME/.cache}/p10k-instant-prompt-${(%):-%n}.zsh"
fi


[[ -n "$Zdbg" ]] && echo "#zshrc" >> $Zdbg

#PROMPT='%{[36m%}%~%{[m%} %% '
#RPROMPT='%(0?..(%?%)) %{[36m%}%n%{[35m%}@%{[34m%}%M %{[33m%}%T%{[m%} '
#PROMPT=$'%{\e[36m%}%~%{\e[0m%} %% '
#RPROMPT=$'%(0?..%?%)) %{\e[36m%}%n%{\e[35m%}@%{\e[34m%}%M %{\e[33m%}%T%{\e[0m%}'

fpath+=~/.zfunc
autoload -U compinit promptinit
compinit
promptinit
 
prompt walters
# cat /usr/share/zsh/functions/Prompts/prompt_walters_setup
if [[ "$TERM" = "dumb" ]]; then
    export PROMPT="%(?..[%?] )%n@%m:%~> "
elif [[ -n "$SSH_CONNECTION" ]] ; then
    IPAddr=`echo $SSH_CONNECTION | cut -d' ' -f3 | cut -d. -f3-`
    export  PROMPT="%(?..[%?] )%n@${IPAddr}> "
    export RPROMPT="%~ %(t.Ding!.%D{%L:%M})"
else
    IPAddr=`ip addr show dev $(ip route |/bin/grep -Po '^default.*dev \K\w+') |/bin/grep -Po '\s+inet \d+.\d+.\d+.\K\d+'`
    # SSH_CONNECTION=192.168.11.24 51538 192.168.11.121 22
    #IPAddr=`localhost`
    export  PROMPT="%(?..[%?] )%n@${IPAddr}> "
    export RPROMPT="%~ %(t.Ding!.%D{%L:%M})"
fi

## https://github.com/NerdyPepper/pista
#autoload -Uz add-zsh-hook
#_pista_prompt() {
# PROMPT="$(pista -z)"   # `pista -zm` for the miminal variant
#}
#add-zsh-hook precmd _pista_prompt

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

## http://zshwiki.org/home/zle/bindkeys
bindkey -v
autoload -U        edit-command-line
zle -N             edit-command-line
bindkey -M vicmd v edit-command-line
export KEYTIMEOUT=20 # Fri, Nov 21, 2014  8:36:52 AM

setopt interactive_comments
bindkey "\e#" vi-pound-insert
# bindkey "\eq" push-line
bindkey "^J" self-insert

export SDCV_HISTSIZE=10000
#unalias run-help
#autoload run-help

# alias cp='cp -i'
alias mv='mv -i'
alias rm='rm -i'
alias grep='grep --color=auto'
alias df='df -Th'

if uname -r |grep Microsoft &>/dev/null ; then
    true
fi

if which lsd &>/dev/null ; then
    true #alias ls='lsd'
fi
if ls -d --color=auto &>/dev/null ; then
    alias ls='ls -F --color=auto'
else
    # export CLICOLOR=1
    alias ls='ls -F' # alias ls='ls -F --color=auto'
fi


alias l='lsd --timesort -r --tail 50 -l' #'eza -l --sort modified'
alias ll='lsd --timesort -r -l'
#alias ll-tail='eza --sort modified -1 | tail -50 | eza -d -l --sort modified --stdin'
#alias ll='ls -trl'
alias feh='feh -.F'

alias svndiff='svn diff --diff-cmd wsvndiff'

#if which curl 2>/dev/null ; then
#    which wget 2>/dev/null || alias wget='curl -O'
#fi

###########################################################
### git clone git://github.com/zsh-users/zsh-completions.git
#fpath=($HOME/zsh-completions/src $fpath)

#which todo 2>/dev/null && alias todo='todo --database-loaders binary'

#alias ydcv='ydcv-rs'
#alias dict='sdcv -0'
dict() {
    echo $* >> $HOME/.dict_history
    if which sdcv &>/dev/null ; then
        sdcv --color --utf8-output --non-interactive $* 2>/dev/null
    fi
    charcoal query $*
    echo ">=< bing >=< `bing-dict $*`"
    
    #if which ydcv-rs &>/dev/null ; then
    #    ydcv-rs "$1"
    #elif which sdcv &>/dev/null ; then
    #    sdcv -0 "$1"
    #elif which dioxionary &>/dev/null ; then
    #    dioxionary -L "$1"
    #elif which xdg-open &>/dev/null ; then
    #    xdg-open "http://dict.cn/$1"
    #else
    #    grep "$1" /usr/share/dict/*
    #fi
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


alias tmux='TERM=xterm-256color tmux -2'
#alias tmux='tmux -2'

##alias for cnpm
#alias cnpm="npm --registry=https://registry.npm.taobao.org \
#  --cache=$HOME/.npm/.cache/cnpm \
#  --disturl=https://npm.taobao.org/dist \
#  --userconfig=$HOME/.cnpmrc"

# limit coredumpsize 0

alias gc='crate-patches clone' gp='crate-patches pull' # ='git-clonepull '
alias clone='proxychains crate-patches clone' pull='proxychains crate-patches pull'
fdx() {
    # gp=git-pull.sh gc='git-https2git  clone --depth 1'
    destdir=`fdxhome $*` && cd "$destdir"
    #fd --min-depth 2 -d2 -iI $* /up  | sk -i -c 'rg --color=always --line-number -i "{}"'
}
#alias b='cargo build' r='cargo run' rex='cargo run --example'
#alias v=view.sh
function v() { nvim "$( fd -j1 -iI $* | sk -i )" }

#[ -f ~/.fzf.zsh ] && source ~/.fzf.zsh

function ghc() { git clone --depth 1 "https://gh.api.99988866.xyz/$1" $2 } 

alias jq-package-scripts='jql \"scripts\" package.json'

dependencies() {
    cargo outdated -R --manifest-path "$( fd -j1 -iI Cargo.toml | sk -i )"
}
#fd -j1 Cargo.toml -x cargo outdated -R --manifest-path $*

#eval "`mcfly init zsh`"
#export MCFLY_KEY_SCHEME=vim MCFLY_RESULTS=30 # MCFLY_FUZZY=true
#eval "$(atuin init zsh)"

#source /usr/share/zsh/plugins/zsh-syntax-highlighting/zsh-syntax-highlighting.zsh
#source /usr/share/zsh-theme-powerlevel10k/powerlevel10k.zsh-theme
# To customize prompt, run `p10k configure` or edit ~/.p10k.zsh.

#source /home/wood/.config/broot/launcher/bash/br

# bun completions
[ -s "/home/wood/.bun/_bun" ] && source "/home/wood/.bun/_bun"
#export BUN_INSTALL="$HOME/.bun"
#export PATH="$BUN_INSTALL/bin:$PATH"

## pnpm
#export PNPM_HOME="$_CACHE/pnpm"
#case ":$PATH:" in
#  *":$PNPM_HOME:"*) ;;
#  *) export PATH="$PATH:$PNPM_HOME" ;;
#esac
## pnpm end
PATH="$PATH:$DENO_INSTALL/bin"

[[ ! -f ~/.p10k.zsh ]] || source ~/.p10k.zsh

local FOUND_ATUIN=$+commands[atuin]
if [[ $FOUND_ATUIN -eq 1 ]]; then
      source <(atuin init zsh)
fi
eval "$(zoxide init zsh)"

