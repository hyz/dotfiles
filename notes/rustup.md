
### https://github.com/rust-lang-nursery/rustup.rs/blob/master/README.md#installation

Choosing where to install

    CARGO_HOME
    RUSTUP_HOME

### https://github.com/rust-lang-nursery/rustup.rs

    rustup self update
    rustup update nightly
    rustup update
    rustup show

    rustup target list
    rustup toolchain list

    rustup target add x86_64-pc-windows-gnu
    rustup target add arm-linux-androideabi
	rustup target add --toolchain nightly wasm32-unknown-wasi
    rustup target remove ...

    rustup install nightly-x86_64-pc-windows-gnu

    RUSTUP_DIST_SERVER=https://dev-static.rust-lang.org rustup update stable

### https://wiki.archlinux.org/index.php/Rust

    rustup toolchian uninstall stable-x86_64-unknown-linux-gnu

    rustup default stable
    rustup default nightly
    rustup default nightly-x86_64-pc-windows-gnu

    rustup component add rust-src
    cargo install racer

### ~/.cargo/config

    [target.x86_64-pc-windows-gnu]
    linker = "/usr/bin/x86_64-w64-mingw32-gcc"
    ar = "/usr/x86_64-w64-mingw32/bin/ar"

