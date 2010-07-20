
if [[ -z "$DISPLAY" ]] && [[ $(tty) = /dev/tty1 ]]; then
    if ls .mldonkey/incoming/files/* >/dev/null; then
        if ! pidof mlnet; then
            [ -e .mldonkey/mlnet.pid ] && rm -f .mldonkey/mlnet.pid
            mlnet &
        fi
    fi
    xinit ; mlnet -exit ; logout
fi

