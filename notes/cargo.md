
    LIBGL_ALWAYS_SOFTWARE=1 cargo run --release
    LIBGL_ALWAYS_SOFTWARE=1 cargo run -p stylish_webrender --example demo --release

    cargo run --features="winit glium" --release --example old_demo

### mingw-w64

    cargo build -vv --target x86_64-pc-windows-gnu --release

### cargo web

    cargo web build --release --target wasm32-unknown-unknown
    ## cargo web start --target-webasm --release --host 0

### musl static linked

    OPENSSL_STATIC=1 OPENSSL_INCLUDE_DIR=/opt/musl/include OPENSSL_LIB_DIR=/opt/musl/lib PKG_CONFIG_ALLOW_CROSS=1 SODIUM_STATIC=yes SODIUM_LIB_DIR=/opt/musl/lib cargo build --release --target x86_64-unknown-linux-musl

