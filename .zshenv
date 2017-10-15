
Zdbg="/tmp/zsh.$USER"

[ -z "$Zdbg" -o -e $Zdbg ] || /bin/env > $Zdbg

[[ -n "$Zdbg" ]] && echo >> $Zdbg
[[ -n "$Zdbg" ]] && echo "#zshenv $USER" >> $Zdbg

## https://unix.stackexchange.com/questions/110737/how-would-i-detect-a-non-login-shell-in-zsh
#if [[ -o login ]]; then fi
[[ -n "$Zdbg" ]] && [[ -o interactive ]] && echo "-o interactive" >> $Zdbg
[[ -n "$Zdbg" ]] && [[ -o login ]] && echo "-o login" >> $Zdbg

