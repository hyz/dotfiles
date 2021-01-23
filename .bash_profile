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

ANDROID_SDK=/xhome/sdk/android-sdk
if [[ -d "$ANDROID_SDK" ]] ; then
    #ANDROID_SDK_HOME=$ANDROID_SDK
    ANDROID_HOME=$ANDROID_SDK
    export ANDROID_SDK ANDROID_HOME #ANDROID_SDK_HOME

    PATH=$PATH:$ANDROID_SDK/tools/bin:$ANDROID_SDK/platform-tools:$ANDROID_SDK/tools
fi
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk

ANDROID_NDK=/xhome/sdk/android-ndk
if [ -d "$ANDROID_NDK" ]; then
    ANDROID_NDK_HOME=$ANDROID_NDK
    export ANDROID_NDK ANDROID_NDK_HOME

    PATH=$PATH:$ANDROID_NDK #:$ANDROID_NDK/standalone/toolchain/android-12/bin
fi


for x in /usr/local/bin /opt/bin $HOME/.yarn/bin $HOME/.config/yarn/global/node_modules/.bin ; do
    [ -d "$x" ] || continue
    PATH=$PATH:$x
done

XHOME=/xhome
if uname -r | /bin/grep Microsoft 2>/dev/null ; then
    XHOME=/mnt/e/home
elif [ -d /xhome ] ; then
    XHOME=/xhome
else
    XHOME=$HOME/xhome
fi
XSDK=$XHOME/sdk
DENO_DIR=$XHOME/deno
DENO_INSTALL_ROOT=$DENO_DIR
CARGO_HOME=$XHOME/cargo
RUSTUP_HOME=$XHOME/rustup
WASMER_DIR=$XHOME/wasmer
GOPATH=$XHOME/go
FLUTTER_HOME=$XSDK/flutter

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

PATH=$PATH:$FLUTTER_HOME/bin:$XHOME/bin:$CARGO_HOME/bin:$DENO_DIR/bin

if [ -x /opt/google/chrome/chrome ] ; then PATH=$PATH:/opt/google/chrome ; fi

export XHOME GOPATH CARGO_HOME RUSTUP_HOME DENO_DIR DENO_INSTALL_ROOT
export PUB_HOSTED_URL=https://pub.flutter-io.cn
export FLUTTER_STORAGE_BASE_URL=https://storage.flutter-io.cn
export PATH

source $HOME/.config/broot/launcher/bash/br

