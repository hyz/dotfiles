# ~/.bashrc: executed by bash(1) for non-login shells.
# see /usr/share/doc/bash/examples/startup-files (in the package bash-doc)
# for examples

# If not running interactively, don't do anything
[ -z "$PS1" ] && return

# don't put duplicate lines in the history. See bash(1) for more options
export HISTCONTROL=ignoredups
# ... and ignore same sucessive entries.
export HISTCONTROL=ignoreboth

# check the window size after each command and, if necessary,
# update the values of LINES and COLUMNS.
shopt -s checkwinsize

# make less more friendly for non-text input files, see lesspipe(1)
[ -x /usr/bin/lesspipe ] && eval "$(lesspipe)"

# set variable identifying the chroot you work in (used in the prompt below)
if [ -z "$debian_chroot" ] && [ -r /etc/debian_chroot ]; then
    debian_chroot=$(cat /etc/debian_chroot)
fi

#tux # set a fancy prompt (non-color, unless we know we "want" color)
#tux case "$TERM" in
#tux xterm-color)
#tux     PS1='${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
#tux     ;;
#tux *)
#tux     PS1='${debian_chroot:+($debian_chroot)}\u@\h:\w\$ '
#tux     ;;
#tux esac
#tux 
#tux # Comment in the above and uncomment this below for a color prompt
#tux # PS1='${debian_chroot:+($debian_chroot)}\A@\w\$ '
#tux # PS1='${debian_chroot:+($debian_chroot)}\[\033[01;32m\]\u@\h\[\033[00m\]:\[\033[01;34m\]\w\[\033[00m\]\$ '
#tux 
#tux # If this is an xterm set the title to user@host:dir
#tux case "$TERM" in
#tux xterm*|rxvt*)
#tux     PROMPT_COMMAND='echo -ne "\033]0;${USER}@${HOSTNAME}: ${PWD/$HOME/~}\007"'
#tux     ;;
#tux *)
#tux     ;;
#tux esac

# Alias definitions.
# You may want to put all your additions into a separate file like
# ~/.bash_aliases, instead of adding them here directly.
# See /usr/share/doc/bash-doc/examples in the bash-doc package.

#if [ -f ~/.bash_aliases ]; then
#    . ~/.bash_aliases
#fi

# enable color support of ls and also add handy aliases
if [ "$TERM" != "dumb" ]; then
	#eval "`dircolors -b`"
	#[ -e "${HOME}/.dir_colors" ] && eval "`dircolors -b ${HOME}/.dir_colors`"
    alias ls='ls --color=auto'
    #alias dir='ls --color=auto --format=vertical'
    #alias vdir='ls --color=auto --format=long'
fi

# some more ls aliases
alias ll='ls -l'
#alias la='ls -A'
#alias l='ls -CF'
alias df='df -h'
alias mv='mv -i'

# enable programmable completion features (you don't need to enable
# this, if it's already enabled in /etc/bash.bashrc and /etc/profile
# sources /etc/bash.bashrc).
if [ -f /etc/bash_completion ]; then
    . /etc/bash_completion
fi

# -------- # -------- #
export LANG=en_US.UTF-8

export PATH=$PATH:/sbin:/usr/sbin:/usr/local/sbin:/usr/games/bin

export EDITOR=vi

if [ -d "$HOME/lib" ]; then
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/lib
	export LD_LIBRARY_PATH
	if [ -d "$HOME/lib/python" ]; then
		PYTHONPATH=$PYTHONPATH:$HOME/lib/python
		export PYTHONPATH
	fi
fi

alias bjam='bjam --v2 --toolset=gcc'
# alias mplayer='mplayer -fs -ao pulse'

BOOST_ROOT=${HOME}/view/boost
BOOST_BUILD_PATH=${HOME}/view/boost/tools/build/v2
export BOOST_ROOT BOOST_BUILD_PATH

### ! ps aux | grep -q fetchmail && fetchmail &

#eval `ssh-agent`
#keychain id_rsa
. $HOME/.keychain/home-sh

if uname -r |grep gentoo; then
	alias bittorrent-curses='bittorrent-curses --save_in ~/Downloads'
fi

