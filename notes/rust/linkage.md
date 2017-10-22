
### https://internals.rust-lang.org/t/static-binary-support-in-rust/2011/61

### http://rust-dev.1092773.n5.nabble.com/Compiling-static-binary-td11340.html#a11341

    rustc -C link-args=-static -Z print-link-args hello-rust.rs
    rustc -Z print-link-args hello-rust.rs \
        | sed -e 's/ /\n/g' | grep rlib

    x=`rustc -Z print-link-args hello-rust.rs`
    y=`rustc -C link-args=-static -Z print-link-args hello-rust.rs`
    diff <(tr ' ' $'\n' <<< $x) <(tr ' ' $'\n' <<< $y)

### https://doc.rust-lang.org/reference/linkage.html


### https://stackoverflow.com/questions/15852677/static-and-dynamic-shared-linking-with-mingw

gcc object1.o object2.o -lMyLib2 -Wl,-Bstatic -lMyLib1 -Wl,-Bdynamic -o output

The above snippet guarantees that the default linking priority of -l flag is overridden for MyLib1, i.e. even if MyLib1.dll is present in the search path, LD will choose libMyLib1.a to link against. Notice that for MyLib2 LD will again prefer the dynamic version.

NOTE: If MyLib2 depends on MyLib1, then MyLib1 is dynamically linked too, regardless of -Wl,-Bstatic (i.e. it is ignored in this case). To prevent this you would have to link MyLib2 statically too.

### https://wiki.archlinux.org/index.php/Rust#Windows
