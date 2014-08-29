
if [[ -z "$DISPLAY" ]] && [[ "$(tty)" = /dev/tty1 ]]; then
    if which startx 2>/dev/null ; then
        startx #; logout
    elif which xinit 2>/dev/null ; then
        xinit #; logout
    fi
fi

export PYTHONSTARTUP=~/.pythonstartup

