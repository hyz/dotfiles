# ~/.bash_profile: executed by bash(1) for login shells.
# see /usr/share/doc/bash/examples/startup-files for examples.
# the files are located in the bash-doc package.

# the default umask is set in /etc/login.defs
#umask 022

# include .bashrc if it exists
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi

# set PATH so it includes user's private bin if it exists
if [ -d ~/bin ] ; then
    PATH=~/bin:"${PATH}"
fi

#QEMU_AUDIO_DRV=alsa
QEMU_AUDIO_DRV=pa
SDL_AUDIODRIVER=pulse
export QEMU_AUDIO_DRV SDL_AUDIODRIVER

if [[ -z "$DISPLAY" ]] && [[ $(tty) = /dev/tty1 ]]; then
    date
    xinit
    logout
fi

