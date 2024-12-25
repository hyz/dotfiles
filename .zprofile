
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

#if [ -d "/opt/gradle-2.12/bin" ]; then
#    export GRADLE_HOME=/opt/gradle-2.12
#    PATH=$PATH:/opt/gradle-2.12/bin
#fi

# Add environment variable ANT_ROOT for cocos2d-x
#export ANT_HOME=/usr/share/apache-ant
#export ANT_ROOT=/bin

#PATH=$PATH:/sbin:/usr/sbin

#if [ -d "$HOME/.config/yarn/global/node_modules" ]; then
#    #if [ x"$NODE_PATH" = x ] ; then
#    #NODE_PATH="$HOME/.config/yarn/global/node_modules"
#    #else
#    #    NODE_PATH="$NODE_PATH:$HOME/.config/yarn/global/node_modules"
#    #fi
#    #export NODE_PATH
#    true
#fi

XHOME=
if uname -r | /bin/grep Microsoft &>/dev/null ; then
    XHOME=/mnt/i/home
elif [ -d /up/_local ] ; then
    XHOME=/up
elif [ -d /opt/x/bin ] ; then
    XHOME=/opt/x
elif [ -d $HOME/up ] ; then
    XHOME=$HOME/up
fi
export _CACHE="$XHOME/_cache" _LOCAL="$XHOME/_local" _CONFIG="$XHOME/_config"

if [ -n "$_LOCAL" ] ; then
    # https://deno.land/manual/getting_started/setup_your_environment
    DENO_DIR=$_LOCAL/deno
    DENO_INSTALL=$DENO_DIR
    DENO_INSTALL_ROOT=$DENO_DIR
    #
    CARGO_HOME=$_LOCAL/cargo
    RUSTUP_HOME=$_CACHE/rustup
    #
    WASMER_DIR=$_LOCAL/wasmer
    #
    GOPATH=$_LOCAL/go
    #
    FLUTTER_HOME=/opt/flutter # $_LOCAL/flutter
    #
    _SDK=$_LOCAL/sdk
    ANDROID_SDK=$_SDK/android-sdk
    ANDROID_NDK=$_SDK/android-ndk
else
    XHOME=$HOME
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

#export NOTION_HOME="$XHOME/node/notion"
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

PNPM_HOME="$_CACHE/pnpm"
PATH=$PATH:$_LOCAL/bin:$CARGO_HOME/bin:$PNPM_HOME:$FLUTTER_HOME/bin:$DENO_DIR/bin
#:$FLUTTER_HOME/bin
#:$DENO_DIR/bin



#if [ -x "/opt/android/studio/bin/studio.sh" ]; then
#    PATH=$PATH:/opt/android/studio/bin
#fi

if [ -x /opt/google/chrome/chrome ] ; then PATH=$PATH:/opt/google/chrome ; fi

export PNPM_HOME
export XHOME GOPATH CARGO_HOME RUSTUP_HOME DENO_DIR DENO_INSTALL DENO_INSTALL_ROOT
export PUB_HOSTED_URL=https://pub.flutter-io.cn
export FLUTTER_STORAGE_BASE_URL=https://storage.flutter-io.cn
export PATH


