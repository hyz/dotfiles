

    czkawka_cli dup -s HASHMB --directories $env.PWD -f ([$env.PWD, '..', 'czdup.txt'] | str collect '\')
        # {$env.TMP}/czdup.txt

    $env.Path # echo $env."PATH"

    let-env RUST_BACKTRACE = 1
    env | where name == RUST_BACKTRACE

    (env).name

    #env -h
    #env | where name == PATH
    #env | where name == PWD

