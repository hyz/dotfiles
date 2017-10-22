
### Mingw and gtk Installation # http://gtk-rs.org/tuto/cross 

    pacaur -S mingw-w64-gcc mingw-w64-freetype2-bootstrap mingw-w64-cairo-bootstrap
    pacaur -S mingw-w64-harfbuzz
    pacaur -S mingw-w64-pango
    pacaur -S mingw-w64-poppler
    pacaur -S mingw-w64-gtk3

    #![windows_subsystem = "windows"]
    export PKG_CONFIG_ALLOW_CROSS=1
    export PKG_CONFIG_PATH=/usr/i686-w64-mingw32/lib/pkgconfig
    cargo build --target=x86_64-pc-windows-gnu --release

    mkdir /wherever/release
    cp target/x86_64-pc-windows-gnu/release/*.exe /wherever/release
    cp /usr/x86_64-w64-mingw32/bin/*.dll /wherever/release
    mkdir -p /wherever/release/share/glib-2.0/schemas
    mkdir /wherever/release/share/icons
    cp /usr/x86_64-w64-mingw32/share/glib-2.0/schemas/* /wherever/release/share/glib-2.0/schemas
    cp -r /usr/x86_64-w64-mingw32/share/icons/* /wherever/release/share/icons

