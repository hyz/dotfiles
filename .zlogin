
if [[ -z "$DISPLAY" ]] && [[ $(tty) = /dev/tty1 ]]; then
    if ls .mldonkey/incoming/files/* >/dev/null; then
        mlnet >/dev/null &
    fi
    xinit ; logout
fi

