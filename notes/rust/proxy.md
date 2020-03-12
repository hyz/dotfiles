

### cargo/config

    [http]
    proxy = "127.0.0.1:1080"
    [https]
    proxy = "127.0.0.1:1080"

### PowerShell

    $proxy='http://127.0.0.1:1080'
    $proxy='http://192.168.9.24:8118'
    $ENV:HTTP_PROXY=$proxy
    $ENV:HTTPS_PROXY=$proxy

### https://www.cnblogs.com/a208606/p/9929400.html

    [source.crates-io]
    registry = "https://github.com/rust-lang/crates.io-index"
    replace-with = 'ustc'
    [source.ustc]
    registry = "git://mirrors.ustc.edu.cn/crates.io-index"

    export RUSTUP_DIST_SERVER=https://mirrors.ustc.cn/rust-static
    export RUSTUP_UPDATE_ROOT=https://mirrors.ustc.cn/tust-static/rustup
    export RUSTUP_UPDATE_ROOT=https://mirrors.ustc.cn/tust-static/rustup

