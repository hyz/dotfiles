
### https://github.com/sureshjoshi/mobile-protobuf-libs
### https://tech.pic-collage.com/how-to-cross-compile-google-protobuf-lite-for-android-977df3b7f20c

    mkdir b && cd b

    TC=/opt/android-ndk/build/cmake/android.toolchain.cmake
    NDK=/opt/android-ndk #/Users/$USER/Library/Android/sdk/ndk-bundle
    CMakePFX=/opt/android-sdk/cmake/3.6.4111459 # /Users/$USER/Library/Android/sdk/cmake/3.6.4111459/bin/cmake

    $CMakePFX/bin/cmake \
      -Dprotobuf_BUILD_TESTS=OFF \
      -Dprotobuf_BUILD_EXAMPLES=OFF \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$TC \
      -DCMAKE_INSTALL_PREFIX=./dist \
      -DANDROID_NDK=$NDK \
      -DANDROID_ABI=$target_arch \
      -DANDROID_NATIVE_API_LEVEL=21 \
      -DANDROID_LINKER_FLAGS="-landroid -llog" \
      -DANDROID_CPP_FEATURES="rtti exceptions" \
      -DCMAKE_INSTALL_DO_STRIP=1 \
      ../cmake

    #-Dprotobuf_BUILD_SHARED_LIBS=ON                # Build shared (.so), instead of defaulted static libs (.a)
    #-DANDROID_TOOLCHAIN=gcc                        # Need to use gcc instead of clang because we're using gnustl
    #-DANDROID_STL=gnustl_shared \                   # TODO: Why did libc++ fail out due to lack of TR1 support? PB is picking up the wrong clang version

    cmake -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_BUILD_EXAMPLES=OFF -DCMAKE_INSTALL_PREFIX=/opt -DCMAKE_BUILD_TYPE=Release ../protobuf-3.4.1/cmake

