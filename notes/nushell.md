

    czkawka_cli dup -s HASHMB --directories $env.PWD -f ([$env.PWD, '..', 'czdup.txt'] | str collect '\')

    echo $env."PATH"

    env -h
    env | where name == PATH
    env | where name == PWD

    let-env RUST_BACKTRACE = 1
    env | where name == RUST_BACKTRACE

    (env).name

