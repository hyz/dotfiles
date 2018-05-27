
https://github.com/rust-lang/rust/issues/32859

    apt install mingw-w64

    rustup target add x86_64-pc-windows-gnu

    rustc -v -C linker=x86_64-w64-mingw32-gcc --target x86_64-pc-windows-gnu main.rs

### arch mingw-w64

    # remove all
    yaourt -Rscnd mingw-w64 --noconfirm || echo "already clean"
    # update
    yaourt -Syu --noconfirm

    yaourt --noconfirm -S mingw-w64-binutils
    yaourt --noconfirm -S mingw-w64-headers
    yaourt --noconfirm -S mingw-w64-headers-bootstrap
    yaourt --noconfirm -S mingw-w64-gcc-base
    yaourt --noconfirm -S mingw-w64-crt
    sudo pacman -Rdd --noconfirm mingw-w64-headers-bootstrap
    yaourt --noconfirm -S mingw-w64-winpthreads
    sudo pacman -Rdd --noconfirm mingw-w64-gcc-base
    yaourt --noconfirm -S mingw-w64-gcc --tmp $PWD

