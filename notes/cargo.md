
    LIBGL_ALWAYS_SOFTWARE=1 cargo run --release
    LIBGL_ALWAYS_SOFTWARE=1 cargo run -p stylish_webrender --example demo --release

    cargo run --features="winit glium" --release --example old_demo

### mingw-w64

    cargo build -vv --target x86_64-pc-windows-gnu --release

### cargo web

    cargo web build --release --target wasm32-unknown-unknown
    ## cargo web start --target-webasm --release --host 0

### musl static linked

    - pacman -S musl

    - vim ~/.cargo/config
    [target.x86_64-unknown-linux-musl]
    linker = "/usr/bin/musl-gcc"

    - ... build openssl for musl

    CC="musl-gcc -fPIC -pie" SODIUM_BUILD_STATIC=yes OPENSSL_STATIC=1 OPENSSL_INCLUDE_DIR=/usr/lib/musl/include/openssl-1.0 OPENSSL_LIB_DIR=/usr/lib/musl/lib cargo build --release --target x86_64-unknown-linux-musl

    # OPENSSL_STATIC=1 OPENSSL_INCLUDE_DIR=/usr/lib/musl/include OPENSSL_LIB_DIR=/usr/lib/musl/lib PKG_CONFIG_ALLOW_CROSS=1 SODIUM_STATIC=yes SODIUM_LIB_DIR=/usr/lib/musl/lib cargo build --release --target x86_64-unknown-linux-musl

