
export XMODIFIERS="@im=ibus"
export GTK_IM_MODULE="ibus"
export QT_IM_MODULE="xim"

# ibus-daemon -d -x
# fcitx &

#xcompmgr -fF -I-.002 -O-.003 -D1 &
#devilspie &
#sh ./.fehbg &

#gnome-settings-daemon &
#gnome-volume-manager &
#conky &

#xmodmap -e "remove Lock = Caps_Lock" -e "keysym Caps_Lock = Escape" -e "keysym Escaps = Caps_Lock" -e "add Lock = Caps_Lock"
#/home/wjy/bin/x-session-rc &

xrdb -merge .Xresources
# xcompmgr -Ss -n -Cc -fF -I-10 -O-10 -D1 -t-3 -l-4 -r4

#mpd

exec xmonad
#exec fvwm-crystal
#exec fvwm-themes-start

#rox -p default
#tint2 &
fbpanel &

#xloadimage -onroot -fullscreen Desktop/Untitled/100_0210.JPG
#feh --bg-scale Desktop/Untitled/100_0210.JPG

# urxvtd -q -f -o
# urxvt -e screen -e^jj &
# xterm -e screen -e^jj &

# exec compiz
exec openbox-session
exec fvwm
exec awesome
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

