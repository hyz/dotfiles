# /etc/skel/.bashrc
#
# This file is sourced by all *interactive* bash shells on startup,
# including some apparently interactive shells such as scp and rcp
# that can't tolerate any output.  So make sure this doesn't display
# anything or bad things will happen !


# Test for an interactive shell.  There is no need to set anything
# past this point for scp and rcp, and it's important to refrain from
# outputting anything in those cases.
if [[ $- != *i* ]] ; then
	# Shell is non-interactive.  Be done now!
	return
fi

HISTCONTROL=ignorespace:erasedups

# EDITOR=vi
LANG=en_GB.UTF-8
# LC_CTYPE=zh_CN.UTF-8
# LC_COLLATE=posix
export HISTCONTROL LANG
export PATH=$PATH:/sbin:/usr/sbin:/usr/local/sbin:/usr/games/bin

if [ -d "$HOME/lib" ]; then
	LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/lib
	export LD_LIBRARY_PATH
	if [ -d "$HOME/lib/python" ]; then
		PYTHONPATH=$PYTHONPATH:$HOME/lib/python
		export PYTHONPATH
	fi
fi

alias df='df -h'
alias lynx='lynx -vikeys'
# alias screen='screen -U -e^jj'
export LIBGL_DEBUG=verbose

export BOOST_ROOT=$HOME/view/boost
export BOOST_BUILD_PATH=$BOOST_ROOT/tools/build/v2

### ! ps aux | grep -q fetchmail && fetchmail &

#eval `ssh-agent`
#keychain id_rsa
. $HOME/.keychain/tux-sh

