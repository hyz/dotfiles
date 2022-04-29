
    -DCMAKE_TOOLCHAIN_FILE=I:\home\vcpkg\scripts\buildsystems\vcpkg.cmake
    -DVCPKG_TARGET_TRIPLET=x64-windows
    -DVCPKG_TARGET_TRIPLET=x64-windows-static

### vcpkg

    .\bootstrap-vcpkg.bat -disableMetrics
    .\vcpkg integrate install

    vcpkg depend-info sdl2-image:x64-windows
    vcpkg install libpng:x64-windows

# flare-engine

x64 Native Tools Command Prompt for VS 2019

    I:\home\...\flare-engine

    del CMakeCache.txt
    cmake . -G Ninja -DCMAKE_TOOLCHAIN_FILE=I:\...\vcpkg\scripts\buildsystems\vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows
    cmake --build .

### dirent.h

    C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Tools\MSVC\14.23.28105\include

### openssl:x64-windows-static

    vcpkg install openssl:x64-windows-static

    find_package(OpenSSL REQUIRED)
    target_link_libraries(main PRIVATE OpenSSL::SSL OpenSSL::Crypto)

### sqlite3:x64-windows-static

    vcpkg install sqlite3:x64-windows-static

    find_package(unofficial-sqlite3 CONFIG REQUIRED)
    target_link_libraries(main PRIVATE unofficial::sqlite3::sqlite3)

### sdl2:x64-windows

    .\vcpkg.exe install sdl2:x64-windows sdl2-image:x64-windows sdl2-mixer:x64-windows sdl2-ttf:x64-windows




