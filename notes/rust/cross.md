
    rustup target add x86_64-pc-windows-gnu
    rustup target add armv7-linux-androideabi

    rustc -v -C linker=x86_64-w64-mingw32-gcc --target x86_64-pc-windows-gnu main.rs

    cargo +nightly build --verbose --release --target x86_64-pc-windows-gnu

### ubuntu https://github.com/rust-lang/rust/issues/32859

    apt-get install mingw-w64

### arch aur mingw-w64

    # remove all
    yaourt -Rscnd mingw-w64 --noconfirm || echo "already clean"

    mingw-w64-binutils
    mingw-w64-headers
    mingw-w64-headers-bootstrap
    mingw-w64-gcc-base
    mingw-w64-crt
    pacman -Rdd --noconfirm mingw-w64-headers-bootstrap
    mingw-w64-winpthreads
    pacman -Rdd --noconfirm mingw-w64-gcc-base
    mingw-w64-gcc

### ref

https://github.com/fluffyemily/cross-platform-rust
https://github.com/kennytm/rust-ios-android
https://github.com/Dushistov/rust_swig

