# ~/.bash_profile: executed by bash(1) for login shells.
# see /usr/share/doc/bash/examples/startup-files for examples.
# the files are located in the bash-doc package.

export LANG=en_US.UTF-8

# the default umask is set in /etc/login.defs
#umask 022

if [ -z "$DISPLAY" ] && [ -n "$XDG_VTNR" ] && [ "$XDG_VTNR" -eq 1 ]; then
    export PATH
    if which sway 2>/dev/null ; then
        export MOZ_ENABLE_WAYLAND=1 SDL_VIDEODRIVER=wayland
        exec sway
    elif which startx 2>/dev/null ; then
    exec startx
    elif which xinit 2>/dev/null ; then
        exec xinit
    fi
fi

export TERM=xterm-256color
export SHELL=/bin/bash
#export EDITOR=vim

# include .bashrc if it exists
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi

# set PATH so it includes user's private bin if it exists
if [ -d ~/bin ] ; then
    PATH="${PATH}":$HOME/bin
fi

#export PATH="$HOME/.cargo/bin:$PATH"

XHOME=
if uname -r | /bin/grep Microsoft 2>/dev/null ; then
    XHOME=/mnt/i/home
elif [ -d /xhome/bin ] ; then
    XHOME=/xhome
elif [ -d /opt/x/bin ] ; then
    XHOME=/opt/x
elif [ -d $HOME/xhome ] ; then
    XHOME=$HOME/xhome
fi
if [ -n "$XHOME" ] ; then
    XSDK=$XHOME/sdk
    DENO_DIR=$XHOME/deno/.cache
    DENO_INSTALL=$XHOME/deno
    DENO_INSTALL_ROOT=$DENO_INSTALL
    CARGO_HOME=$XHOME/cargo
    RUSTUP_HOME=$XHOME/rustup
    WASMER_DIR=$XHOME/wasmer
    GOPATH=$XHOME/go
    FLUTTER_HOME=/opt/flutter # $XSDK/flutter
    ANDROID_SDK=$XHOME/sdk/android-sdk
    ANDROID_NDK=$XHOME/sdk/android-ndk
else
    alias rustup=/bin/echo
    alias cargo=/bin/echo
    alias go=/bin/echo
    alias flutter=/bin/echo
    alias deno=/bin/echo
fi

#export RUST_SRC_PATH=$RUSTUP_HOME/rust-src
###
#if [ -d "$HOME/cargo" ]; then
#    export CARGO_HOME=$HOME/cargo
#    PATH=$PATH:$CARGO_HOME/bin
#fi
#if which rustc ; then
    #export RUST_SRC_PATH="$(rustc --print sysroot)/lib/rustlib/src/rust/src"
#fi

#export NOTION_HOME="$XHOME/notion"
#[ -s "$NOTION_HOME/load.sh" ] && \. "$NOTION_HOME/load.sh"
#PATH="${NOTION_HOME}/bin:$PATH"

if [[ -d "$ANDROID_SDK" ]] ; then
    export JAVA_HOME=/usr/lib/jvm/java-8-openjdk
    #ANDROID_SDK_HOME=$ANDROID_SDK
    ANDROID_HOME=$ANDROID_SDK
    export ANDROID_SDK ANDROID_HOME #ANDROID_SDK_HOME
    PATH=$PATH:$ANDROID_SDK/tools/bin:$ANDROID_SDK/platform-tools:$ANDROID_SDK/tools
fi
if [ -d "$ANDROID_NDK" ]; then
    ANDROID_NDK_HOME=$ANDROID_NDK
    export ANDROID_NDK ANDROID_NDK_HOME
    PATH=$PATH:$ANDROID_NDK #:$ANDROID_NDK/standalone/toolchain/android-12/bin
fi

for x in /usr/local/bin $HOME/.local/bin /opt/bin $HOME/.yarn/bin $HOME/.config/yarn/global/node_modules/.bin ; do
    [ -d "$x" ] || continue
    PATH=$PATH:$x
done
PATH=$PATH:$FLUTTER_HOME/bin:$XHOME/bin:$CARGO_HOME/bin:$DENO_DIR/bin

if [ -x /opt/google/chrome/chrome ] ; then PATH=$PATH:/opt/google/chrome ; fi

export XHOME GOPATH CARGO_HOME RUSTUP_HOME DENO_DIR DENO_INSTALL DENO_INSTALL_ROOT
export PUB_HOSTED_URL=https://pub.flutter-io.cn
export FLUTTER_STORAGE_BASE_URL=https://storage.flutter-io.cn
export PATH

# source $HOME/.config/broot/launcher/bash/br

export DVM_DIR="/home/wood/.dvm"
export PATH="$DENO_INSTALL/bin:$PATH"
export PATH="$DVM_DIR/bin:$PATH"

