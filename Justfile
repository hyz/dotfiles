
hello:
    @echo hello

default:
    xsearch-dbup a.xlsx 2>/dev/null |tee a.txt |wc

help:
    @ echo 'export LS_COLORS="$(vivid generate molokai)"'

xsearch-dbup FILE:
    #xf-parse -f code-name {{FILE}}
    xf-parse -f code-name --host tyun {{FILE}}
    ssh tyun 'psql myt1 -c "SELECT COUNT(*) FROM warelis"'

update_code-name +FILES:
    update_code-name {{FILES}} >code-name.$(date +%m%d) 2>/tmp/update_code-name.log
    ln -sf code-name.$(date +%m%d) code-name
    bat code-name
    @rg '^Updated' /tmp/update_code-name.log || true

stone-story:
    curlftpfs 192.168.9.55 ~/mnt/Music
    ln -sf ~/mnt/Music/foobar2000\ Music\ Folder/stone-story .

bin BIN_F:
    RUSTFLAGS="-Ctarget-feature=-crt-static" RUSTFLAGS="-L/usr/lib/musl/lib" CC="musl-gcc -fPIC -pie" \
    OPENSSL_STATIC=1 OPENSSL_INCLUDE_DIR=/usr/lib/musl/include OPENSSL_LIB_DIR=/usr/lib/musl/lib PKG_CONFIG_ALLOW_CROSS=1 \
    SODIUM_BUILD_STATIC=yes \
        cargo build --release --target x86_64-unknown-linux-musl --bin {{BIN_F}}
    ln -f target/x86_64-unknown-linux-musl/release/{{BIN_F}} .

youtube-formats v:
    youtube-dl --proxy=socks5://127.0.0.1:21080 -F "{{v}}"

youtube URL:
    # 249          webm       audio only tiny   56k , opus @ 50k (48000Hz), 18.86MiB
    # 250          webm       audio only tiny   73k , opus @ 70k (48000Hz), 24.39MiB
    # 140          m4a        audio only tiny  133k , m4a_dash container, mp4a.40.2@128k (44100Hz), 48.62MiB
    # 251          webm       audio only tiny  147k , opus @160k (48000Hz), 48.50MiB
    youtube-dl --proxy=socks5://127.0.0.1:21080 -f 250 "{{URL}}"

