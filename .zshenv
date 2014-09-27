
###! exec >/tmp/zsh.txt 2>&1

echo "zshenv $HOME" >> /tmp/zsh.txt

source $HOME/.zprofile

