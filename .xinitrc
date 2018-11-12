
export LANG=en_US.UTF-8

#export GTK_IM_MODULE=ibus QT_IM_MODULE=ibus XMODIFIERS="@im=ibus"
export GTK_IM_MODULE=fcitx QT_IM_MODULE=fcitx XMODIFIERS="@im=fcitx"

#export GDK_SCALE=2 # 4k GTK3
#export QT_AUTO_SCREEN_SCALE_FACTOR=0 # QT5 no auto scale
#export QT_SCALE_FACTOR=2 # QT5 manual x2

#setxkbmap -option terminate:ctrl_alt_bksp
xrdb -merge .Xresources
xset -b
xset +fp ~/.local/share/fonts

unclutter -root -idle 2 &

fcitx -d
#ibus-daemon -d -x

#mpd

pgrep -x redshift &>/dev/null || redshift -r &>/dev/null &

urxvtd -q -f -o

#xcompmgr -fF -I-.002 -O-.003 -D1 &
# xcompmgr -Ss -n -Cc -fF -I-10 -O-10 -D1 -t-3 -l-4 -r4
#devilspie &
#sh ./.fehbg &

#gnome-settings-daemon &
#gnome-volume-manager &
#conky &

## http://vim.wikia.com/wiki/Map_caps_lock_to_escape_in_XWindows
# ~/.keymap
#$ xmodmap -pk | grep Lock
# 9    	0xff1b (Escape)	0x0000 (NoSymbol)	0xff1b (Escape)	
# 66    	0xffe5 (Caps_Lock)	0x0000 (NoSymbol)	0xffe5 (Caps_Lock)	
# 77    	0xff7f (Num_Lock)	0x0000 (NoSymbol)	0xff7f (Num_Lock)	
# 78    	0xff14 (Scroll_Lock)	0x0000 (NoSymbol)	0xff14 (Scroll_Lock)	
# xmodmap -e "remove Lock = Caps_Lock" -e "keysym Caps_Lock = Escape"
## https://wiki.archlinux.org/index.php/Xmodmap
# xmodmap -pke > ~/.Xmodmap
# xmodmap -pm
# xmodmap -e "remove Lock = Caps_Lock" -e "keysym Caps_Lock = Escape" -e "pointer = 1 2 3 5 4"
[[ -f ~/.Xmodmap ]] && xmodmap ~/.Xmodmap

for x in fvwm enlightenment_start dwm awesome xmonad ; do
    which $x 2>/dev/null && exec $x
done
# exec ck-launch-session gnome-session

#rox -p default
# tint2 &
#fbpanel &

#xloadimage -onroot -fullscreen Desktop/Untitled/100_0210.JPG
#[ which feh 2>/dev/null >&2 ] && feh --bg-center Pictures/Wallpapers/vladstudio_rainbows_1920x1440.jpg

#exec $HOME/snip/2bwm/src/2bwm

#exec subtle
#exec 2bwm
exec gnome-session
exec openbox-session
# exec compiz
exec sawfish
exec wmaker
exec startfluxbox

#export OOO_FORCE_DESKTOP=gnome
#export LANG="en_US.UTF-8"
#export LC_ALL="en_US.UTF-8"
#export LANGUAGE="en_US.UTF-8"
#export LC_CTYPE="en_US.UTF-8"

exec $your_window_manager

