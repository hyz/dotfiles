
if which xinit 2>/dev/null 1>&2 ; then
    if [[ -z "$DISPLAY" ]] && [[ $(tty) = /dev/tty1 ]]; then
        # if ls .mldonkey/incoming/files/* >/dev/null 2>&1; then
        #     if ! pidof mlnet; then
        #         [ -e .mldonkey/mlnet.pid ] && rm -f .mldonkey/mlnet.pid
        #         mlnet &
        #     fi
        # fi
        xinit ; which mlnet 2>/dev/null && mlnet -exit ; logout
    fi
fi

export PYTHONSTARTUP=~/.pythonstartup

