
PREFIX=/tmp
NDK_UNAME=`uname -s | tr '[A-Z]' '[a-z]'`
NDK_PROCESSOR=x86_64
NDK_PLATFORM_LEVEL=21
NDK_COMPILER_VERSION=4.9
NDK_ABI=arm
NDK_TOOLCHAIN=$NDK_ROOT/toolchains/$NDK_ABI-linux-androideabi-$NDK_COMPILER_VERSION/prebuilt/$NDK_UNAME-$NDK_PROCESSOR
NDK_SYSROOT=$NDK_ROOT/platforms/android-$NDK_PLATFORM_LEVEL/arch-$NDK_ABI

[ -d "$NDK_TOOLCHAIN" -a -d "$NDK_SYSROOT" ] || exit 1

CFLAGS="-O3 -Wall -mthumb -pipe -fpic -fasm \
  -finline-limit=300 -ffast-math \
  -fstrict-aliasing -Werror=strict-aliasing \
  -fmodulo-sched -fmodulo-sched-allow-regmoves \
  -Wno-psabi -Wa,--noexecstack \
  -D__ARM_ARCH_5__ -D__ARM_ARCH_5E__ -D__ARM_ARCH_5T__ -D__ARM_ARCH_5TE__ \
  -DANDROID -DNDEBUG"
EXTRA_CFLAGS="-march=armv7-a -mfpu=vfpv3-d16 -mfloat-abi=softfp -fPIE -pie"
EXTRA_LDFLAGS="-Wl,--fix-cortex-a8 -fPIE -pie"

#./configure --help ; exit
#./configure --list-hwaccels ; exit

./configure \
--target-os=android \
--arch=arm \
--cpu=cortex-a8 \
--enable-runtime-cpudetect \
--enable-pic \
--enable-cross-compile \
--cross-prefix=$NDK_TOOLCHAIN/bin/$NDK_ABI-linux-androideabi- \
--sysroot="$NDK_SYSROOT" \
--extra-cflags="$CFLAGS $EXTRA_CFLAGS" \
--extra-ldflags="$EXTRA_LDFLAGS" \
--disable-shared --enable-static \
--prefix=$PREFIX \
--disable-debug \
\
--disable-doc \
--enable-asm \
--disable-symver \
--enable-gpl --enable-nonfree \
\
--enable-version3 \
--disable-postproc \
--disable-encoders \
--disable-muxers \
--disable-swscale-alpha \
\
--disable-outdevs --enable-outdev=fbdev \
--disable-indevs --enable-indev=fbdev \
--disable-filters  --enable-filter=copy \
--disable-decoders --enable-decoder=h264_mediacodec \
--disable-muxers --enable-muxer=h264 \
--disable-demuxers --enable-demuxer=h264 \
--disable-bsfs --enable-bsf=h264_mp4toannexb \
--disable-protocols --enable-protocol=pipe --enable-protocol=http --enable-protocol=rtp --enable-protocol=file \
--disable-parsers --enable-parser=h264 \
--disable-libfontconfig --disable-libfreetype \
--disable-ffplay --disable-ffprobe --disable-ffserver --enable-ffmpeg \
--enable-jni --enable-mediacodec --enable-hwaccel=h264_mediacodec --enable-hwaccel=hevc_mediacodec \
--enable-logging \

####
exit

# --disable-avfilter \
#--enable-small \

--disable-decoders --enable-decoder=h264 \

--enable-jni --enable-mediacodec --enable-hwaccel=h264_mediacodec --enable-hwaccel=hevc_mediacodec \
--disable-decoders --enable-decoder=h264_mediacodec \

--enable-libx264 \

