
### http://www.commandlinefu.com/commands/browse/sort-by-votes

### http://stackoverflow.com/questions/369758/how-to-trim-whitespace-from-a-bash-variable

### http://tldp.org/LDP/abs/html/process-sub.html
### http://en.wikipedia.org/wiki/Process_substitution

    echo >(true) <(true)
    date |tee >(cat) 

### find

    find trunk -name ".svn" -prune -o -print 
    find trunk \( -name ".svn" -o -name ".git" \) -prune -o -print 
    find * -name .repo -prune -o -name .git -prune -o -name out -prune -o -type f \( -name '*.c' -o -name '*.cc' -o -name '*.cpp' \) -print0 > cxx.files

    find * ! -writable |xargs chmod u+w
    find * ! -readable

### chown/setfacl

    sudo setfacl -m u:${USER}:rw /dev/kvm

### tcpdump

    tcpdump -nn -w pcap -i eth0 \(port 9001 or port 9000\) and host 127.0.0.1
    tcpdump -nn -X -r pcap
    tcpdump -nn -A -r pcap

### pgrep

    pgrep -lf yxim

    top -p $(echo `pgrep -f yxim` |tr ' ' ',')
    top -cp $(echo `pgrep -f yxim` |tr ' ' ',')
    # top, hit 'H' to toggle thread listing

### ps

    ps -lF
    ps -F -e
    ps -H -u wood
    ps -F -C yxim-server
    ps -mL -C yxim-server
    ps -mL -p $(pidof yxim-server)

10 basic examples of Linux ps command

    http://www.binarytides.com/linux-ps-command/

### python dd

    echo '{"cmd":71}' |yxim-pack |nc 127.0.0.1 8443 |dd bs=4 skip=1 2>/dev/null |python -mjson.tool
    echo '{"cmd":71}' |yxim-nc 127.0.0.1 8443 |python -mjson.tool

### sort

http://stackoverflow.com/questions/4562341/stable-sort-in-linux

    sort -k1,1 -s t.txt

### awk

    cat 3 |awk 'NF==5 && $NF=="ONLS" && $1==0 {print $0"\t"($2-$3)}' |sort -nk 6 -r

http://www.pement.org/awk/awk1line.txt
http://www.catonmat.net/blog/ten-awk-tips-tricks-and-pitfalls/

    ls -1 |awk 'BEGIN {printf("%s %8s %8s %8s \n" ,"field1", "field2", "field3", "field4")} {printf("%6.2f %8.2f %8.2f %8.2f\n", $1, $2, $3, $4)}'

### column

    (printf "PERM LINKS OWNER GROUP SIZE MONTH DAY HH:MM/YEAR NAME\n" ; ls -l | sed 1d) | column -t

### sed

http://stackoverflow.com/questions/1251999/sed-how-can-i-replace-a-newline-n?rq=1

    sed ':a;N;$!ba;s/\n/ /g' # sed -e ':a' -e 'N' -e '$!ba' -e 's/\n/ /g' 
    sed -i 's/\x00/\n/g' cxx.files

### dd
### http://www.kwx.gd/CentOSApp/CentOS-dd.html

    dd if=mbr.2T-gpt of=/tmp/mbr-x conv=notrunc

notrunc prevent truncation when writing into a file (replace part-of-file). This has no effect on a block device

    dd if=/dev/zero of=test bs=64k count=4k oflag=dsync

### xxd

    xxd -ps -c32 -l256 mbr.2T-gpt
    xxd -ps -c$((128*128)) 128x128.y |less
    xxd -ps -c40000 400x100.y |cut -c-16 |less
    xxd -g1 -p -c32 out > b

### file-size, du | awk

    du -sm * | awk '$1 > 1000'
    find . -type f -size +1000M

### ###

    echo "echo \$X" | X=hello sh                                                                             ~
    X=hello echo "$X"                                                                                       ~

    export X=hello ; echo "$X"                                                                              ~

    exec 3>out ; date |cat >&3
    cat out

### find & cpio

- http://en.wikipedia.org/wiki/Cpio
- http://www.thegeekstuff.com/2010/08/cpio-utility/

    find 0nodes \( -name '.*' -o -name old -o -name "urdl*" \) -prune -o -name "*.[hc]*" |cpio -o | gzip -c |ssh w243 "cat > 0nodes.cpio.gz-$(date +%F)"
    find 0nodes/* -maxdepth 0 -type f ! -name "*.gch" |cpio -o | gzip -c |ssh w243 "cat > 0nodes.cpio.gz-$(date +%F)"
    find 0nodes -maxdepth 1 -type f ! -name "*.gch" |cpio -o | gzip -c |ssh w243 "cat > 0nodes.cpio.gz-$(date +%F)"

    export T=~/isdk/alpha/i51EMU/WIN32FS/DRIVE_E ; export K=$T/i51/i51KIT

    ;cmdow 'zsh' /MOV 200 -30 /SIZ 1100 808 ;cmdow '-zsh' /MOV -4 -30 /SIZ 1100 808 #CMDOW
    ;cmdow 'zsh' /MOV 364 -30 /SIZ 1100 1196 ;cmdow '-zsh' /MOV -4 -30 /SIZ 1100 1196 #CMDOW

    find etc -type f |while read f ; do diff "$f" "/$f"; echo "$? $f"; done
    find usr -type f |while read f ; do diff "$f" "/$f"; echo "$? $f"; done

    diff -r --brief dir1 dir2 |grep -v '^Only' |sed -e 's/^Files /cp /' -e 's/ and / /' -e 's/ differ//'
    diff -r --brief dir1 dir2 |grep -v '^Only' |sed -e 's/^Files //' -e 's/ and / /' -e 's/ differ//' |while read l; do vimdiff $l; done

    find . -depth -print | cpio -o > /path/archive.cpio
    cpio -i -t < archive.cpio
    cpio -i -vd < archive.cpio
    cpio -i -vd etc/fstab < archive.cpio
    find . -depth -print | cpio -p -vdum new-path

    gzip -dc < 0nodes.cpio.gz-2015-02-03 |cpio -t
    gzip -dc < 0nodes.cpio.gz-2015-02-03 |cpio -di

###

    file --write-out                                                                                                               ~

    file: unrecognized option '--write-out'
    Usage: file [-bcEhikLlNnprsvz0] [--apple] [--mime-encoding] [--mime-type]
                [-e testname] [-F separator] [-f namefile] [-m magicfiles] file ...
           file -C [-m magicfiles]
           file [--help]

    file -- --write-out                                                                                                        ~

    --write-out: ASCII text, with CRLF line terminators

### 

    tpx="`date +'%F %T'`"
    tps="`perl -MURI::Escape -e 'print uri_escape($ARGV[0]);' "$tpx"`"

###

    history 0 |awk '{print $2}' | sort | uniq -c | sort -gr | head -30

###

    useradd -Um -u 1000 -G wheel -s `which zsh` wood

###

    7z e /mnt/sources/install.wim 2/Windows/Boot/EFI/bootmgfw.efi
    awk -F'\t' '{print $(NF-2)}' lis |less

http://stackoverflow.com/questions/1037365/unix-sort-with-tab-delimiter

    sort -t$'\t' -k3 -nr file.txt

### ctags

    ctags --c-kinds=+px `find $(pwd) -type f -name "*.[ch]*" |grep -v win`

    find *.* dzpk /usr/include/qt/ -path "3rdparty" -prune -o \( -name "*.h" -o -name "*.cpp" \) -print > tags.files
    /usr/bin/arduino-ctags --c++-kinds=+px -Ltags.files

    ctags -f tags/java -R --language-force=java /opt/java/src

    rg -t java --files |grep -vE '(test|3rdparty|Crypto)' > java.files
    rg -t cpp --files |grep -vE '(test|3rdparty|Crypto)' > cpp.files

