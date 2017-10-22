
### https://wiki.archlinux.org/index.php/Rust

    rustup install nightly-x86_64-pc-windows-gnu

    # ~/.cargo/config
    [target.x86_64-pc-windows-gnu]
    linker = "/usr/bin/x86_64-w64-mingw32-gcc"
    ar = "/usr/x86_64-w64-mingw32/bin/ar"

