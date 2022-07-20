

    gpg --keyserver hkp://pgp.mit.edu   --recv-keys <key>
    gpg --keyserver pgp.mit.edu         --recv-keys <key>
    gpg --keyserver keys.gnupg.net      --recv-keys <key>

    gpg2 --recv-key 93BDB53CD4EBC740

    makepkg --skippgpcheck

### https://wiki.archlinux.org/index.php/AUR_helpers

https://github.com/rmarquis/pacaur

rslsync-2.4.2-1-x86_64.pkg.tar.xz
kcptun-git-v20161111.r1.g754b4a7-1-x86_64.pkg.tar.xz


### PKGBUILD

    source=("$pkgname"::'git+file:///xhome/windows.desktop/vcpkg/.git'      ...)
    source=("$pkgname::git+ssh://localhost/xhome/tcpip.dns.email/trust-dns")

### aur.archlinux.../rua example

PKGBILD

    #url='https://github.com/vn971/rua'
    #source=("${pkgname}-${pkgver}.tar.gz::https://github.com/vn971/rua/archive/v${pkgver}.tar.gz")
    ...
    prepare() {
        echo "`pwd` == $srcdir"
        git clone -ls --depth 1 -b main .../sozu/ "${pkgname}-${pkgver}"
        # rsync -r .../rua/ "${pkgname}-${pkgver}"
    }

makepkg

    makepkg --skippgpcheck --skipinteg 

