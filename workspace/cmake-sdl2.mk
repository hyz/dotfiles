
$(warning $(CURDIR))
$(warning $(MAKEFILE_LIST))
$(warning $(lastword $(MAKEFILE_LIST)))

#where-am-i = $(CURDIR)/$(word ($words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)
#CUR_MAKEFILE := $(call where-am-i)
CUR_MAKEFILE := $(lastword $(MAKEFILE_LIST))
$(warning $(CUR_MAKEFILE))
$(warning "$(MAKE) $(MAKECMDGOALS) MyArg=$(MyArg)")

#test:
#	$(MAKE) -f $(CUR_MAKEFILE) Next
#Next:

sdl2: ANDROID_ABI ?= armeabi-v7a
sdl2: ANDROID_PLATFORM ?= android-17
sdl2:
	rm -rf SDL2-2.0.8/armeabi-v7a/build ; mkdir -p SDL2-2.0.8/armeabi-v7a/build
	cd SDL2-2.0.8/armeabi-v7a/build \
		&& $(MAKE) -f ../../../$(CUR_MAKEFILE) sdl2-cmake ANDROID_ABI=$(ANDROID_ABI) ANDROID_PLATFORM=$(ANDROID_PLATFORM)

sdl2-cmake:
	/opt/android-sdk/cmake/3.6.4111459/bin/cmake \
	-H../.. \
	-B. \
	-G"Android Gradle - Ninja" \
	-DANDROID_ABI=$(ANDROID_ABI) \
	-DANDROID_NDK=/opt/android-ndk \
	-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=.. \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_MAKE_PROGRAM=/opt/android-sdk/cmake/3.6.4111459/bin/ninja \
	-DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk/build/cmake/android.toolchain.cmake \
	-DANDROID_PLATFORM=$(ANDROID_PLATFORM) \
	-DCMAKE_CXX_FLAGS= \
	-DSDL_SHARED=ON -DSDL_STATIC=OFF

	/opt/android-sdk/cmake/3.6.4111459/bin/cmake --build . --target SDL2

.PHONY: default build

# #./target/aarch64-linux-android/release/build/sdl2-sys-f8f9587704070371/out/SDL2-2.0.8
# # aarch64-linux-android
# # cfg.define("SDL_SHARED", "OFF"); cfg.define("SDL_STATIC", "ON");
# 
# /opt/android-sdk/cmake/3.6.4111459/bin/cmake \
# -H/home/wood/workspace/SDL2-2.0.8 \
# -B/tmp/sdl2/cmake/armeabi-v7a \
# -G"Android Gradle - Ninja" \
# -DANDROID_ABI=armeabi-v7a \
# -DANDROID_NDK=/opt/android-ndk \
# -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=/tmp/sdl2/armeabi-v7a \
# -DCMAKE_BUILD_TYPE=Release \
# -DCMAKE_MAKE_PROGRAM=/opt/android-sdk/cmake/3.6.4111459/bin/ninja \
# -DCMAKE_TOOLCHAIN_FILE=/opt/android-ndk/build/cmake/android.toolchain.cmake \
# -DANDROID_PLATFORM=android-17 \
# -DCMAKE_CXX_FLAGS= \
# 
# cmake --build . --target SDL2

