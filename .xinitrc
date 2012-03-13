
export XMODIFIERS="@im=ibus"
export GTK_IM_MODULE="ibus"
export QT_IM_MODULE="xim"

ibus-daemon -d -x
# fcitx &

#xcompmgr -fF -I-.002 -O-.003 -D1 &
#devilspie &
#sh ./.fehbg &

#gnome-settings-daemon &
#gnome-volume-manager &
#conky &

xmodmap -e 'clear Lock' -e 'keycode 0x42 = Escape'
#xmodmap -e "remove Lock = Caps_Lock" -e "keysym Caps_Lock = Escape" -e "keysym Escaps = Caps_Lock" -e "add Lock = Caps_Lock"
#/home/wjy/bin/x-session-rc &

xrdb -merge .Xresources
# xcompmgr -Ss -n -Cc -fF -I-10 -O-10 -D1 -t-3 -l-4 -r4
xset -b

#mpd

#xterm -geometry +96-28 &
#xterm -geometry +480+0 &
#xterm -geometry -0-28 &

#exec ck-launch-session gnome-session
exec xmonad

#rox -p default
# tint2 &
#fbpanel &

#xloadimage -onroot -fullscreen Desktop/Untitled/100_0210.JPG
feh --bg-center Pictures/wallpapers/vladstudio_rainbows_1920x1440.jpg

# urxvtd -q -f -o
# urxvt -e screen -e^jj &
# xterm -e screen -e^jj &

#exec subtle
exec fvwm
exec awesome
exec openbox-session
# exec compiz
exec echinus
exec fluxbox
exec sawfish
exec wmaker
exec gnome-session
exec startfluxbox

#export OOO_FORCE_DESKTOP=gnome
#export LANG="en_GB.UTF-8"
#export LC_ALL="en_GB.UTF-8"
#export LANGUAGE="en_GB.UTF-8"
#export LC_CTYPE="en_GB.UTF-8"


# Source scripts in /etc/X11/xinit/xinitrc.d/
if [ -d /etc/X11/xinit/xinitrc.d ]; then
    for f in /etc/X11/xinit/xinitrc.d/*; do
        [ -x "$f" ] && . "$f"
    done
    unset f
fi

exec $your_window_manager

