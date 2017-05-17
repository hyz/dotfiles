# http://mathslinux.org/?cat=48
#
export CROSS_COMPILE=arm-linux-androideabi
export CC=${CROSS_COMPILE}-gcc
export CXX=${CROSS_COMPILE}-g++
export TOOLC=/opt/android/standalone-api17
#export NDK=$HOME/android/standalone/toolchain/bin
export SYSROOT=$TOOLC/sysroot
export PATH=$PATH:$TOOLC/bin

#gl_cv_header_working_stdint_h=yes \
./configure --host=arm-linux-androideabi --target=arm-linux-androideabi \
  --prefix=/tmp/iperf \
  --enable-static \
  --disable-shared --disable-fast-install \
\
  --with-sysroot=$SYSROOT \
\
  CPPFLAGS="--sysroot=$SYSROOT -fPIC" \
  CXXFLAGS="--sysroot=$SYSROOT -fPIC" \
  CFLAGS="--sysroot=$SYSROOT -static -fPIC" \
  LDFLAGS="-static -pie" \

#You can compile and link a PIE executable in one of two ways. First, compile everything with -fPIE and link with -pie. The second is to compile everything with -fPIC and link with -pie.
#If you are building both a shared object and a program, then compile everything with -fPIC. Link the shared object with -shared, and link the program with -pie.

### stackoverflow.com/questions/40975003/iperf3-execution-from-android-7-doesnt-work
# Tip: iperf3 uses an folder to cache the results which is inaccessible for Android. You'll need to change this folder to work:
# https://github.com/esnet/iperf/blob/670c18584bcf7a285f3561eb7ea38cc53600d0ab/src/iperf_api.c#L2621
#. android 7.0, has no `/tmp' directory, use `/data/local/tmp' instead

#
# iperf3 -c 192.168.0.20 -u -M 1456 -b 100M
#
# adb -d shell /data/iperf3 -c 192.168.0.1 -u -M 1456 -b 100M                  
#

###1 vim src/Makefile --- remove flags: -pg -g

