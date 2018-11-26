
https://rustwasm.github.io/book/

### https://www.hellorust.com/

    rustup target add wasm32-unknown-unknown --toolchain nightly
    cargo install --git https://github.com/alexcrichton/wasm-gc

### https://hacks.mozilla.org/2018/04/hello-wasm-pack/

### https://rust-lang-nursery.github.io/rust-wasm/

    cargo +nightly build --target wasm32-unknown-unknown --release
    wasm-gc target/wasm32-unknown-unknown/release/hello_world.wasm -o hello_world.gc.wasm
    wasm-opt -Os hello_world.gc.wasm -o hello_world.gc.opt.wasm

### https://rustwasm.github.io/book/game-of-life/hello-world.html

    cargo generate --git https://github.com/rustwasm/wasm-pack-template

