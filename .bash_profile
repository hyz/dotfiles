# ~/.bash_profile: executed by bash(1) for login shells.
# see /usr/share/doc/bash/examples/startup-files for examples.
# the files are located in the bash-doc package.

export LANG=en_US.UTF-8
# the default umask is set in /etc/login.defs
#umask 022

# include .bashrc if it exists
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi

# set PATH so it includes user's private bin if it exists
if [ -d ~/bin ] ; then
    PATH="${PATH}":$HOME/bin
fi

if [ -z "$DISPLAY" ] && [ -n "$XDG_VTNR" ] && [ "$XDG_VTNR" -eq 1 ]; then
    exec startx
fi

#export PATH="$HOME/.cargo/bin:$PATH"

ANDROID_SDK=/opt/android-sdk
if [[ -d "$ANDROID_SDK" ]] ; then
    #ANDROID_SDK_HOME=$ANDROID_SDK
    ANDROID_HOME=$ANDROID_SDK
    export ANDROID_SDK ANDROID_HOME #ANDROID_SDK_HOME

    PATH=$PATH:$ANDROID_SDK/tools/bin:$ANDROID_SDK/platform-tools:$ANDROID_SDK/tools
fi

XHOME=/xhome

# [[ -n "$CARGO_HOME" ]] && [[ -d "$CARGO_HOME/bin" ]] ; then
if [[ -d "$XHOME" ]] ; then
    CARGO_HOME=$XHOME/cargo
    RUSTUP_HOME=$XHOME/rustup
    PATH=$PATH:$CARGO_HOME/bin
fi
export XHOME CARGO_HOME RUSTUP_HOME

export JAVA_HOME=/usr/lib/jvm/java-8-openjdk

