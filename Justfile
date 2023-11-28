####!/usr/bin/env just --working-directory . --justfile
# vim: set ft=make :

set positional-arguments

zsh-login:
	LS_COLORS="`vivid generate snazzy`" SHELL=`which zsh` exec /bin/zsh --login -i # ; tmux attach || tmux

projclean:
	projclean node_modules@package.json target@Cargo.toml

pacman-clean:
	#!/bin/sudo /bin/bash
	pacman -S --clean
	pacman -R $(pacman -Qdtq) # autoclean
cache-clean:
	#!/bin/sudo /bin/bash
	echo

named:
	#!/bin/sudo /bin/bash
	exec named.trust-dns -z named -c named/named.toml

hello:
	@echo hello

mkdir:
	mkdir "`date +%y%b`"

default:
	xsearch-dbup a.xlsx 2>/dev/null |tee a.txt |wc

help:
	@ echo 'export LS_COLORS="$(vivid generate molokai)"'

# just xsearch-dbup /home/library/yy/dsky库存表/20-1126.xlsx
xsearch-dbup FILE:
	#xf-parse -f code-name {{FILE}}
	xf-parse -f code-name --host dskydb {{FILE}}
	ssh dskydb 'psql myt1 -c "SELECT COUNT(*) FROM warelis"'

update_code-name +FILES:
	update_code-name {{FILES}} >code-name.$(date +%m%d) 2>/tmp/update_code-name.log
	ln -sf code-name.$(date +%m%d) code-name
	bat code-name
	@rg '^Updated' /tmp/update_code-name.log || true

serve-dir DIR:
	static-web-server -a 0.0.0.0 -p 8080 -z=true -d {{DIR}}

ftpfs:
	[ -d /tmp/Music ] || mkdir /tmp/Music
	curlftpfs 192.168.1.14 /tmp/Music
	#ln -sf ~/mnt/Music/foobar2000\ Music\ Folder/stone-story .

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

sshfs:
	#!/bin/bash
	mount -t sshfs n234:/home/samba /home/samba
	mount -t sshfs n234:/home/library /home/library
	mount -t sshfs n234:/xhome /xhome

bluetooth:
	/bin/sudo systemctl restart bluetooth.service
	systemctl --user restart pulseaudio.service
	# bluetoothctl connect 4C:F9:BE:6E:98:F2
	# bluetoothctl disconnect
	# bluetoothctl scan on

####!/usr/bin/env just --working-directory . --justfile
# vim: set ft=make :

env:
	#!/bin/sudo /bin/bash
	env

shutdown Time="23:15":
	#!/bin/sudo /bin/bash
	shutdown --show
	shutdown -c
	shutdown --no-wall -h {{Time}}

initial-setup:
	#!/bin/sudo /bin/bash
	#!/bin/sudo --chdir /home/ftp /bin/bash
	mount /xhome
	##PATH=$PATH:/xhome/cargo/bin cargo mk ftp-mount
	cd /home/ftp
	echo "$SHELL `pwd` `id` $LOGNAME "
	for x in `/bin/find ???* -prune -type d` ; do
		src=`/bin/find ../library/* -prune -type d -name "$x*"`
		echo "mount -o bind $src $x"
		[ -d "$x" -a -d "$src" ] || continue
		mount -o bind "$src" "$x"
	done
	bftpd -d

privoxy:
	#!/bin/sudo /bin/bash
	#WG_QUICK_USERSPACE_IMPLEMENTATION=boringtun WG_SUDO=1 wg-quick up wg0
	systemctl restart wg-quick@wg0
	sslocal -dc .local/sss-config_ext.json
	systemctl restart privoxy.service

ftp-restart:
	#!/bin/sudo /bin/bash
	systemctl restart bftpd.service
	netstat -ntlp

ftp-bind-share:
	#!/bin/sudo /bin/bash
	#mount -o bind /home/library/Music /home/ftp/Music
	#mount -o bind /home/samba/Audience /home/ftp/Audience
	#systemctl restart bftpd.service

mirrorlist:
	rate-mirrors arch

cifs:
	#!/bin/sudo /bin/bash
	mount -t cifs //192.168.11.234/yysmb /mnt/cifs -o gid=1000,uid=1000

schedule:
	#!/bin/sudo /bin/bash
	shutdown -c
	shutdown --no-wall -h 00:10

gerbera DIR:
	#!/bin/sudo -u gerbera /bin/bash
	#sudo -u gerbera -- /bin/gerbera -c /etc/gerbera/config.xml --add-file /home/library/...
	systemctl stop gerbera
	/bin/gerbera -c /etc/gerbera/config.xml --add-file {{DIR}}

capture:
	menyoki capture --mouse jpg --quality 75 save /tmp/menyoki.jpg

#cat /etc/systemd/system/xremap.service

latest-rust-analyzer:
	#curl -L https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz | gunzip -c - > ~/.local/bin/rust-analyzer
	curl -L https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-pc-windows-msvc.zip --output-dir ~/Incoming
latest-rust-analyzer-linux:
	wget -c https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz -O /tmp/rust-analyzer.gz
	gunzip --to-stdout /tmp/rust-analyzer.gz > /tmp/rust-analyzer
	chmod +x /tmp/rust-analyzer
	file /tmp/rust-analyzer
	#curl -L https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz | gunzip -c - > /tmp/rust-analyzer
analyzer-from11:
	rsync 11:/opt/bin/rust-analyzer /opt/bin

nvim-listen DIR='/xhome/scripts/gitconfig-https2git':
	nvim --headless --listen 192.168.11.11:6666 +cd\ {{DIR}}

static-web-server:
	static-web-server -g debug --host 0.0.0.0 --port 8080 --root ~/Incoming
dufs:
	dufs -A

# $ cat filelist.txt
# file 1.mp3
# file 2.mp3
# file 3.mp3
# 
ffmpeg-concat *Lis:
	#ffmpeg -f concat -i filelist.txt -c copy output.mp3
	ffmpeg-concat {{Lis}} && /bin/ls -ld output*

ffmpeg:
	#ffmpeg -ss 00:01:01 -to 00:33:01 -i input.mp3 -c:a copy output.mp3
	ffmpeg -ss 00:00:14 -to 00:22:36 -i input -map 0:a -c:a copy output.m4a

ffmpeg-record-alsa_output:
	#pacmd list-sources
	ffmpeg -vn -f pulse -i alsa_output.pci-0000_00_1f.3.analog-stereo.monitor r1.wav
	ffmpeg -i r1.wav -vn -c:a aac -b:a 128k r1.m4a

ffmpeg-flv-to-mp4 Flv:
	ln -sf {{Flv}} input && ffmpeg -i input -c:a copy -c:v copy {{Flv}}.mp4

# just ffmpeg-ca 《简单的逻辑学》\*.mp4
ffmpeg-ca First *Elses:
	fd -d1 -tf --glob '{{First}}' -x ffmpeg-ca-copy
	#test -r "{{First}}"
	#ffmpeg-ca-copy "{{First}}"
	#just ffmpeg-ca {{Elses}}

ffmpeg-codecs:
	ffmpeg -codecs
ffmpeg-encoders:
	ffmpeg -encoders
	ffmpeg -h encoder=libx264
ffmpeg-decoders:
	ffmpeg -decoders
	ffmpeg -h decoder=aac
ffmpeg-formats:
	ffmpeg -formats # Containers

bili Url:
	you-get '{{Url}}'

vararg *args:
	bash -c 'while (( "$#" )); do echo - $1; shift; done' -- "$@"

rename-file_mv FileName:
	@test -f "{{FileName}}"
	@mv "{{FileName}}" "{{replace_regex(trim(FileName), '\s+', ',')}}"
rename Pat:
	fd -d1 -tf '{{Pat}}' -x just rename-file_mv

___111:
	v2ray -config /xhome/proxy.vpn.tunnel.gfw/vmess2json/usamd.ptuu.tk.json
	python ../vmess2json/vmess2json.py "vmess://..."
	curl -LO https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/geoip.dat
	curl -LO https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/geoip.dat
	curl -LO https://github.com/Loyalsoldier/v2ray-rules-dat/releases/latest/download/geosite.dat
	https://proxy.yiun.xyz/
	wget https://v1.mk/XPdqsDi
	base64 -d XPdqsDi 

sdc6:
	#!/bin/sudo /bin/bash
	mount /dev/sdc6 /home/library

journalctl:
	#journalctl -fxeu smartdns-rs # > /tmp/smartdns.log 
	# uniq
	journalctl -xeu smartdns-rs --no-pager | grep -Po 'query name:\K.*' |awk '{print $1}' | sort-domain-name |tee names-sorted.txt

gc URL:
	git-clonepull clone --prefixurl='https://ghps.cc' {{URL}}

