
    fd -Id4 up.sql -p migrations
    fd up.sql -p migrations
    fd up.sql migrations
    fd . migrations
    fd -e sql . migrations
    fd -e rs . src

    fd -d4 -tf -IF zsh
    fd -aI -em4a >> .../opt/mpd/playlists/qh.m3u 

    fd -Id3 -e rs |xargs rg bezier # -x rg bezier
    fd -Id3 -tf -g Cargo.toml |xargs rg tile
    fd -Id3 -td -g target

    find * -type d -name node_modules -prune
    find * -type d -prune
    find . -maxdepth 3 \( -name '.??*' \) -prune -false -o -name rust-by-examples -print

### find

    find jni -type f ! -path "*/Crypto/*" \( -name "*.h" -o -name "*.cpp" \)

    find * \( -name .repo -o -name .git -o -name out \) -prune -o -type f \( -name '*.h' \) -print >> h.files
    find * \( -name .repo -o -name .git -o -name out \) -prune -o -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) -print >> cxx.files
    find * \( -path "*/protobuf" -o -path "build" \) -prune -o -print

### http://stackoverflow.com/questions/4210042/exclude-directory-from-find-command

    find -name "*.js" -not -path "./directory/*"

### http://stackoverflow.com/questions/762348/how-can-i-exclude-all-permission-denied-messages-from-find?noredirect=1&lq=1

    find . ! -readable -prune -o -print

    sudo find out/ ! -type l -user 1000 -exec chown build '{}' \;
    sudo find mt6580 -type l -user 1000 -exec chown -h build: '{}' \;

    sudo find . ! -user 1007 -exec chown -h build: '{}' \;

### find src -type f -perm \/111




fd -Id3 -e ts . ../ |xargs rg -w export
