
### https://github.com/rust-lang/rustup/blob/master/doc/src/installation/index.md

Choosing where to install

    CARGO_HOME
    RUSTUP_HOME

    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- --default-toolchain none -y

    rustup toolchain install nightly --allow-downgrade --profile minimal --component clippy

### https://github.com/rust-lang-nursery/rustup.rs

    rustup self update
    rustup update nightly
    rustup update
    rustup show

    rustup toolchain list
    rustup target list
    rustup component list
    rustup toolchain list

    rustup target add x86_64-pc-windows-gnu
    rustup target add arm-linux-androideabi
	rustup target add wasm32-wasi
	# rustup target add --toolchain nightly wasm32-unknown-wasi
    rustup target remove ...
    rustup component remove --toolchain nightly --target x86_64-unknown-linux-gnu rls

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


rustup toolchain remove nightly-2022-08-23-x86_64-pc-windows-msvc
