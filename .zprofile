
[ -d "/tmp/._$USER" ] || mkdir "/tmp/._$USER"
Zdbg="/tmp/._$USER/zsh.log" ; [ -z "$Zdbg" -o -e $Zdbg ] || /bin/env > $Zdbg
[[ -n "$Zdbg" ]] && echo "#zprofile" >> $Zdbg

export LANG=en_US.UTF-8
export EDITOR=vim

if [[ -d "$HOME/bin" ]] ; then
    PATH=$HOME/bin:/bin:/usr/bin
else
    PATH=/bin:/usr/bin
fi

## https://wiki.archlinux.org/index.php/Xinit#Autostart_X_at_login
#if [ -z "$DISPLAY" ] && [ -n "$XDG_VTNR" ] && [ "$XDG_VTNR" -eq 1 ]; then
  #exec startx
#fi
if [[ -o login ]]; then
    true
fi

case "`uname -s`" in
Darwin)
    true
    ;;
CYGWIN*)
    PATH=$PATH:$HOME/bin/cygwin
    /bin/ls -d1 /cygdrive/[cdef]/Program*/GNU/GnuPG |while read x ; do
        PATH=$PATH:"$x"
        #which gpg2
        break
    done
    for x in /cygdrive/{c,d,e,f,g,h,i}/.gnupg ; do
        [ -r "$x/secring.gpg" ] || continue
        export GNUPGHOME=`cygpath -w "$x"`
        break
    done

    _WinDir=`cygpath "$WINDIR"`
    PATH=$PATH:$_WinDir/system32:$_WinDir

    #alias er='explorer "`cygpath -w $(pwd)`" &'
    er() {
        if x=`/bin/ls -1d "$1" || /bin/ls -1d "$(pwd)/$1"` ; then
            "$_WinDir/explorer" "`cygpath -w "$x"`" &
        fi
        #`cygpath $WINDIR`/explorer "`cygpath -w $(pwd)/$1`" &
    }
    ;;
*)
    # if [ ! -e "$HOME/.gnupg/secring.gpg" ] ; then fi
    #which pass 
    gpghome() {
        [ -r "$1" ] && dirname "$1" || false
    }
    for x in "/media/wood/*" "/mnt/*" "$HOME/mnt" ; do
        if x=`eval gpghome $x/.gnupg/secring.gpg 2>/dev/null` ; then
            export GNUPGHOME="$x"
            break
        fi
    done
    ;;
esac

# # export BOOST_BUILD_PATH=/usr/share/boost/build/v2
if [ -d /opt/share/boost-build ] ; then
    export BOOST_BUILD_PATH=/opt/share/boost-build
fi
#if x=`which b2 2>/dev/null` ; then
#    alias b2="$x -sBOOST_ROOT=/BOOST_ROOT -j5"
#fi

###

# Add environment variable ANT_ROOT for cocos2d-x
#export ANT_ROOT=/usr/bin

# build/tools/make_standalone_toolchain.py -v --arch=arm --api=17 --stl=gnustl --install-dir=/opt/android/17-arm-gcc-4.9
#if x=`/bin/ls -1d /opt/android/[0-9][0-9]-*-gcc-[0-9].[0-9]/bin |head -1` ; then
#    PATH=$PATH:$x
#fi

# Add environment variable COCOS_CONSOLE_ROOT for cocos2d-x
#COCOS2DX_ROOT=$HOME/cocos2d
#if [ -d "$COCOS2DX_ROOT" ] ; then
#    export COCOS2DX_ROOT
#fi

#COCOS_CONSOLE_ROOT=$HOME/cocos2d-x/cocos2d-x-3.2/tools/cocos2d-console/bin
#if [ -d "$COCOS_CONSOLE_ROOT" ] ; then
#    PATH=$PATH:$COCOS_CONSOLE_ROOT
#    export COCOS_CONSOLE_ROOT
#fi

#if [ -d "/opt/android/studio/bin" ]; then
#    PATH=$PATH:/opt/android/studio/bin
#fi
#if [ -d "/opt/gradle-2.12/bin" ]; then
#    export GRADLE_HOME=/opt/gradle-2.12
#    PATH=$PATH:/opt/gradle-2.12/bin
#fi

# Add environment variable ANT_ROOT for cocos2d-x
#export ANT_HOME=/usr/share/apache-ant
#export ANT_ROOT=/bin

#PATH=$PATH:/sbin:/usr/sbin

ANDROID_SDK=/opt/android-sdk
if [[ -d "$ANDROID_SDK" ]] ; then
    #ANDROID_SDK_HOME=$ANDROID_SDK
    ANDROID_HOME=$ANDROID_SDK
    export ANDROID_SDK ANDROID_HOME #ANDROID_SDK_HOME

    PATH=$PATH:$ANDROID_SDK/tools/bin:$ANDROID_SDK/platform-tools:$ANDROID_SDK/tools
fi

ANDROID_NDK=/opt/android-ndk
if [ -d "$ANDROID_NDK" ]; then
    ANDROID_NDK_HOME=$ANDROID_NDK
    export ANDROID_NDK ANDROID_NDK_HOME

    PATH=$PATH:$ANDROID_NDK #:$ANDROID_NDK/standalone/toolchain/android-12/bin
fi

# $HOME/.cargo/bin $HOME/go/bin /usr/local/go/bin
for x in /usr/local/bin $HOME/.yarn/bin $HOME/.config/yarn/global/node_modules/.bin /opt/bin ; do
    [ -d "$x" ] || continue
    PATH=$PATH:$x
done

if [ -d "$HOME/.config/yarn/global/node_modules" ]; then
    #if [ x"$NODE_PATH" = x ] ; then
    #NODE_PATH="$HOME/.config/yarn/global/node_modules"
    #else
    #    NODE_PATH="$NODE_PATH:$HOME/.config/yarn/global/node_modules"
    #fi
    #export NODE_PATH
    true
fi

if uname -r |grep Microsoft 2>/dev/null ; then
    workspace=/mnt/d/workspace
else #elif [ -d "$HOME/workspace" ] ; then
    workspace="$HOME/workspace"
fi
if [ -d "$workspace" ]; then
    GOPATH=$workspace/go
    DENO_DIR=$workspace/deno
    CARGO_HOME=$workspace/cargo
    RUSTUP_HOME=$workspace/rustup
    export GOPATH DENO_DIR CARGO_HOME RUSTUP_HOME
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

PATH="$CARGO_HOME/bin:$GOPATH/bin:$DENO_DIR/bin:$PATH"
export PATH

