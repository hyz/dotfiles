#http://vim.wikia.com/wiki/Map_caps_lock_to_escape_in_XWindows
# ~/.keymap
#$ xmodmap -pk | grep Lock
# 9    	0xff1b (Escape)	0x0000 (NoSymbol)	0xff1b (Escape)	
# 66    	0xffe5 (Caps_Lock)	0x0000 (NoSymbol)	0xffe5 (Caps_Lock)	
# 77    	0xff7f (Num_Lock)	0x0000 (NoSymbol)	0xff7f (Num_Lock)	
# 78    	0xff14 (Scroll_Lock)	0x0000 (NoSymbol)	0xff14 (Scroll_Lock)	
# xmodmap -e "remove Lock = Caps_Lock" -e "keysym Caps_Lock = Escape"

export LANG=en_US.UTF-8
# export LANG=en_HK.UTF-8

#export XMODIFIERS="@im=ibus"
#export GTK_IM_MODULE="ibus"
#export QT_IM_MODULE="ibus"
export GTK_IM_MODULE=fcitx
export QT_IM_MODULE=fcitx
export XMODIFIERS=@im=fcitx

ibus-daemon -d -x
# fcitx &

#xcompmgr -fF -I-.002 -O-.003 -D1 &
#devilspie &
#sh ./.fehbg &

#gnome-settings-daemon &
#gnome-volume-manager &
#conky &

#/home/wjy/bin/x-session-rc &

xrdb -merge .Xresources
# xcompmgr -Ss -n -Cc -fF -I-10 -O-10 -D1 -t-3 -l-4 -r4
xset -b
xset +fp /home/wood/.local/share/fonts

#mpd

#xterm -geometry +96-28 &
#xterm -geometry +480+0 &
#xterm -geometry -0-28 &

xmodmap -e "remove Lock = Caps_Lock" -e "keysym Caps_Lock = Escape" -e "pointer = 1 2 3 5 4"

for x in enlightenment_start dwm fvwm awesome xmonad ; do
    which $x 2>/dev/null && exec $x
done
# exec ck-launch-session gnome-session

#rox -p default
# tint2 &
#fbpanel &

#xloadimage -onroot -fullscreen Desktop/Untitled/100_0210.JPG
#[ which feh 2>/dev/null >&2 ] && feh --bg-center Pictures/Wallpapers/vladstudio_rainbows_1920x1440.jpg

# urxvtd -q -f -o
# urxvt -e screen -e^jj &
# xterm -e screen -e^jj &

#exec $HOME/snip/2bwm/src/2bwm

#exec subtle
#exec 2bwm
exec gnome-session
exec openbox-session
# exec compiz
exec echinus
exec fluxbox
exec sawfish
exec wmaker
exec startfluxbox

#export OOO_FORCE_DESKTOP=gnome
#export LANG="en_US.UTF-8"
#export LC_ALL="en_US.UTF-8"
#export LANGUAGE="en_US.UTF-8"
#export LC_CTYPE="en_US.UTF-8"

# Source scripts in /etc/X11/xinit/xinitrc.d/
if [ -d /etc/X11/xinit/xinitrc.d ]; then
    for f in /etc/X11/xinit/xinitrc.d/*; do
        [ -x "$f" ] && . "$f"
    done
    unset f
fi

exec $your_window_manager
