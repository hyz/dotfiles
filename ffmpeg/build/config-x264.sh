
PREFIX=/tmp
NDK_UNAME=`uname -s | tr '[A-Z]' '[a-z]'`
NDK_PROCESSOR=x86_64
NDK_PLATFORM_LEVEL=21
NDK_COMPILER_VERSION=4.9
NDK_ABI=arm
NDK_TOOLCHAIN=$NDK_ROOT/toolchains/$NDK_ABI-linux-androideabi-$NDK_COMPILER_VERSION/prebuilt/$NDK_UNAME-$NDK_PROCESSOR
NDK_SYSROOT=$NDK_ROOT/platforms/android-$NDK_PLATFORM_LEVEL/arch-$NDK_ABI

[ -d "$NDK_TOOLCHAIN" -a -d "$NDK_SYSROOT" ] || exit 1

CFLAGS="-O3 -Wall -pipe -fpic -fPIC -fasm \
  -finline-limit=300 -ffast-math \
  -fstrict-aliasing -Werror=strict-aliasing \
  -fmodulo-sched -fmodulo-sched-allow-regmoves \
  -Wno-psabi -Wa,--noexecstack \
  -DANDROID -DNDEBUG"
EXTRA_CFLAGS="-mthumb -march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -fPIE -pie"
EXTRA_LDFLAGS="-Wl,--fix-cortex-a8 -fPIE -pie"

#-D__ARM_ARCH_5__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5TE__
#-D__ARM_ARCH_7__ -D__ARM_ARCH_7A__

#./configure --help ; exit
#./configure --list-hwaccels ; exit

./configure \
    --host=arm-linux \
    --cross-prefix=$NDK_TOOLCHAIN/bin/$NDK_ABI-linux-androideabi- \
    --sysroot=$NDK_SYSROOT \
    --enable-pic \
    --extra-cflags="$CFLAGS $EXTRA_CFLAGS" \
    --extra-ldflags="$EXTRA_LDFLAGS" \
    --prefix=$PREFIX \
    --enable-static \
    --enable-strip \
\
	--disable-cli \

exit #### exit #### exit #### exit ####

# --disable-avfilter \
#--enable-small \

--enable-hwaccels \
--enable-jni --enable-mediacodec --enable-hwaccel=h264_mediacodec --enable-hwaccel=hevc_mediacodec \

### https://github.com/yixia/x264/blob/master/build_android.sh
EXTRA_CFLAGS="-march=armv7-a -mfloat-abi=softfp -mfpu=neon -D__ARM_ARCH_7__ -D__ARM_ARCH_7A__"
EXTRA_LDFLAGS="-nostdlib"

./configure  --prefix=$PREFIX \
	--cross-prefix=$CROSS_PREFIX \
	--extra-cflags="$EXTRA_CFLAGS" \
	--extra-ldflags="$EXTRA_LDFLAGS" \
	--enable-pic \
	--enable-static \
	--enable-strip \
	--disable-cli \
	--host=arm-linux \
    --sysroot=$SYSROOT
