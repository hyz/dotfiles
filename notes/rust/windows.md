
	#![windows_subsystem = "windows"]

### rust-sdl2

    cargo build --release --target x86_64-pc-windows-gnu --features "bundled static-link" --example renderer-yuv
    cargo rustc --release --target x86_64-pc-windows-gnu --features "bundled static-link" --example renderer-yuv -- -C link-args=-Wl,--subsystem,windows

### https://github.com/rust-lang/cargo/issues/2986


### toolchains

    I:\home\rust\rustup\toolchains\stable-x86_64-pc-windows-msvc\lib\rustlib\x86_64-pc-windows-msvc\lib

