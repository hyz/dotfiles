
    https://rustwasm.github.io/

    https://rustwasm.github.io/2018/10/24/multithreading-rust-and-wasm.html
    https://rustwasm.github.io/2018/10/01/this-week-in-rust-wasm-008.html
    https://rustwasm.github.io/2018/09/04/this-week-in-rust-wasm-007.html

### https://rustwasm.github.io/book/
https://yarnpkg.com/en/docs/yarn-workflow

    cargo generate --git https://github.com/rustwasm/wasm-pack-template --name mytest
    cd mytest
    wasm-pack build

### https://www.hellorust.com/

    rustup target add wasm32-unknown-unknown --toolchain nightly
    cargo install --git https://github.com/alexcrichton/wasm-gc

### https://hacks.mozilla.org/2018/04/hello-wasm-pack/

### https://rust-lang-nursery.github.io/rust-wasm/

    cargo +nightly build --target wasm32-unknown-unknown --release
    wasm-gc target/wasm32-unknown-unknown/release/hello_world.wasm -o hello_world.gc.wasm
    wasm-opt -Os hello_world.gc.wasm -o hello_world.gc.opt.wasm


### https://rustwasm.github.io/wasm-pack/book/tutorial/
https://rustwasm.github.io/book/game-of-life/hello-world.html

    cargo generate --git https://github.com/rustwasm/wasm-pack-template

    npm init wasm-app www


wasm-nm -z pkg/dodrio_todomvc_bg.wasm | sort -n -u -r | rustfilt | head


twiggy garbage  pkg/dodrio_todomvc_bg.wasm

