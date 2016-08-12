### https://z3ntu.github.io/2015/12/12/Cross-compiling-native-linux-applications-for-android.html

###
export CROSS_COMPILE=arm-linux-androideabi
export CC=${CROSS_COMPILE}-gcc
export CXX=${CROSS_COMPILE}-g++
export TOOLC=$HOME/android/standalone/toolchain
#export NDK=$HOME/android/standalone/toolchain/bin
export SYSROOT=$TOOLC/sysroot
export PATH=$PATH:$TOOLC/bin
# ./configure --build=x86_64-unknown-linux-gnu --host=arm-linux-androideabi --target=arm-linux-androideabi

./configure --host=arm-linux-androideabi --target=arm-linux-androideabi
  --prefix=/home/kbox \
  --enable-gpg \
  --disable-gpgsm \
  --disable-agent \
  --disable-scdaemon \
  --disable-tools \
  --disable-doc \
  --disable-symcryptrun \
  --disable-gpgtar \
  --disable-selinux-support \
  --disable-bzip2 \
  --disable-photo-viewers \
  --disable-keyserver-helpers \
  --disable-ldap \
  --disable-hkp \
  --disable-finger \
  --disable-mailto \
  --disable-keyserver-path \
  --disable-ccid-driver \
  --disable-largefile \
  --disable-dns-srv \
  --disable-dns-pka \
  --disable-dns-cert \
  --disable-rpath \
  --disable-nls         \
  --disable-regex \
\
  CFLAGS="--sysroot=$SYSROOT" \
  CPPFLAGS="--sysroot=$SYSROOT" \
  CXXFLAGS="--sysroot=$SYSROOT" \



  #CFLAGS="-std=c99" \


  #--disable-maintainer-mode \
#


  #--disable-endian-check \
#\
#  --without-agent-pgm \
#  --without-pinentry-pgm \
#  --without-scdaemon-pgm \
#  --without-dirmngr-pgm \
#  --without-protect-tool-pgm \
#  --without-photo-viewer \
#  --without-capabilities \
#  --without-tar \
#  --without-libgpg-error-prefix \
#  --without-libgcrypt-prefix \
#  --without-libassuan-prefix \
#  --without-ksba-prefix \
#  --without-adns \
#  --without-ldap \
#  --without-libcurl \
#  --without-mailprog \
#  --without-libiconv-prefix \
#  --without-libiconv-prefix \
#  --without-libintl-prefix \
#  --without-libintl-prefix \
#  --without-regex \
#  --without-zlib \
#  --without-bzip2 \
#  --without-readline \


#$ HOSTCONF=arm-eabi-linux
#$ TOOLCHAIN=$HOME/android/toolchain
#$ export ARCH=armv7-a
#$ export SYSROOT=$TOOLCHAIN/sysroot
#$ export PATH=$PATH:$TOOLCHAIN/bin:$SYSROOT/usr/local/bin
#$ export CROSS_COMPILE=arm-linux-androideabi
#$ export CC=${CROSS_COMPILE}-gcc
#$ export CXX=${CROSS_COMPILE}-g++
#$ export CFLAGS="-DANDROID -mandroid -fomit-frame-pointer --sysroot $SYSROOT -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -mthumb"
#$ export CXXFLAGS=$CFLAGS
#$ ./configure --host=$HOSTCONF --build=i686-pc-linux-gnu  --with-sysroot=$SYSROOT --prefix=$SYSROOT/usr/local --disable-joystick
#$ make
#$ make install

