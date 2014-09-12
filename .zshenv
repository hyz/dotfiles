
###! exec >/tmp/zsh.txt 2>&1

echo "zshenv $HOME" >> /tmp/zsh.txt

export LANG=en_US.UTF-8
export EDITOR=vim

source $HOME/.zprofile

