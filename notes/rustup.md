
### https://github.com/rust-lang-nursery/rustup.rs

    rustup self update
    rustup update
    rustup show

    rustup target list
    rustup toolchain list

    rustup target add x86_64-pc-windows-gnu
    rustup target add arm-linux-androideabi
    rustup target remove ...

    rustup install nightly-x86_64-pc-windows-gnu

### https://wiki.archlinux.org/index.php/Rust

    rustup toolchian uninstall stable-x86_64-unknown-linux-gnu
    rustup default nightly-x86_64-pc-windows-gnu

### ~/.cargo/config

    [target.x86_64-pc-windows-gnu]
    linker = "/usr/bin/x86_64-w64-mingw32-gcc"
    ar = "/usr/x86_64-w64-mingw32/bin/ar"

