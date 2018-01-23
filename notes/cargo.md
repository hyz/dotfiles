
    cargo web start --target-webasm --release --host 0

    LIBGL_ALWAYS_SOFTWARE=1 cargo run --release
    LIBGL_ALWAYS_SOFTWARE=1 cargo run -p stylish_webrender --example demo --release

    cargo run --features="winit glium" --release --example old_demo

### mingw-w64

    cargo build -vv --target x86_64-pc-windows-gnu --release

