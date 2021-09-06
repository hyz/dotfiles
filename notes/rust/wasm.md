


    https://rustwasm.github.io/

    https://rustwasm.github.io/2018/10/24/multithreading-rust-and-wasm.html
    https://rustwasm.github.io/2018/10/01/this-week-in-rust-wasm-008.html
    https://rustwasm.github.io/2018/09/04/this-week-in-rust-wasm-007.html

### https://github.com/rustwasm/wasm-pack/blob/master/docs/src/commands/build.md

    wasm-pack build --target ...

        --target <target>        Sets the target environment. [possible values: bundler, nodejs, web, no-modules]

Option	Usage	Description

not specified or bundler
    Bundler
        Outputs JS that is suitable for interoperation with a Bundler like Webpack. You'll import the JS and the module key is specified in package.json. sideEffects: false is by default.
nodejs
    Node.js
    	Outputs JS that uses CommonJS modules, for use with a require statement. main key in package.json.
web
	Native in browser
        Outputs JS that can be natively imported as an ES module in a browser, but the WebAssembly must be manually instantiated and loaded.
no-modules
	Native in browser
    	Same as web, except the JS is included on a page and modifies global state, and doesn't support as many wasm-bindgen features as web

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


    cargo update -p wasm-bindgen
    cargo build  --target wasm32-unknown-unknown --no-default-features
    wasm-bindgen --out-dir target --target web target/wasm32-unknown-unknown/...

