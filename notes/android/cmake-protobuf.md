
### protobuf-lite

https://aur.archlinux.org/packages/android-sdk-cmake/
https://dl-ssl.google.com/android/repository/cmake-3.6.4111459-linux-x86_64.zip
https://github.com/taka-no-me/android-cmake

    cd ~/protobuf/protobuf-3.4.1

    ## remove libprotobuf, libprotoc, protoc, leave only libprotobuf-lite
    vim cmake/CMakeLists.txt cmake/install.cmake
    ... ## ---example---
        include(libprotobuf-lite.cmake)
        # include(libprotobuf.cmake)
        # include(libprotoc.cmake)
        # include(protoc.cmake)


    ## armeabi armeabi-v7a arm64-v8a x86 x86_64
    for target_arch in armeabi-v7a arm64-v8a ; do
        if [ -d "$target_arch" ]; then rm -r $target_arch; fi
        mkdir $target_arch && cd $target_arch || exit 1

        NDK=/opt/android-ndk
        CMakeTC=/opt/android-ndk/build/cmake/android.toolchain.cmake
        CMakePFX=/opt/android-sdk/cmake/3.6.4111459

        $CMakePFX/bin/cmake \
        -DANDROID_TOOLCHAIN=gcc \
        -Dprotobuf_BUILD_TESTS=OFF \
        -Dprotobuf_BUILD_EXAMPLES=OFF \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=$CMakeTC \
        -DCMAKE_INSTALL_PREFIX=./dist \
        -DANDROID_NDK=$NDK \
        -DANDROID_ABI=$target_arch \
        -DANDROID_NATIVE_API_LEVEL=9 \
        -DANDROID_LINKER_FLAGS="-landroid -llog" \
        -DANDROID_CPP_FEATURES="rtti exceptions" \
        ../cmake

        cd ..
    done 

    ## Build shared (.so), instead of defaulted static libs (.a)
    #  -Dprotobuf_BUILD_SHARED_LIBS=ON
    ## Need to use gcc instead of clang because we're using gnustl
    #  -DANDROID_TOOLCHAIN=gcc
    #  -DANDROID_STL=gnustl_shared
    ## TODO: Why did libc++ fail out due to lack of TR1 support? PB is picking up the wrong clang version
    #  -DANDROID_CPP_FEATURES="rtti exceptions"

    vim some.proto
    ...
    option optimize_for = LITE_RUNTIME;

