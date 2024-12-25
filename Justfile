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

unftp DIR:
	#unftp -v --root-dir=/home/wood/Incoming --bind-address=0.0.0.0:2121 # --auth-type=json --auth-json-path=credentials.json --usr-json-path
	unftp -v --root-dir {{DIR}} --bind-address 192.168.11.11:2121 # --auth-pam-service=wood

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


shutdown-scheduled At="22:35":
	#!/bin/bash
	fd timer /lib/systemd
	fd power /lib/systemd
	bat /etc/systemd/system/poweroff.* # {timer,service}
	systemctl list-timers --all
	# systemctl enable /etc/systemd/system/poweroff.timer
	# systemctl soft-reboot # poweroff
	# systemd-run --on-active=30 touch /tmp/hello-world
	# systemd-run --on-active="12h 30m" --unit poweroff.service
	#
	if shutdown --show 2>/dev/null ; then
		shutdown -c
	else
		true # shutdown --no-wall -h {{At}}
	fi
	# https://wiki.archlinux.org/title/Systemd/Timers
	# https://wiki.archlinux.org/title/systemd#Power_management


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

rust-analyzer-vscode-server:
	@ln -sf "`fd -tf '^rust-analyzer$' ~/.vscode-server | sk`" /up/_local/cargo/bin/rust-analyzer
	@readlink `which rust-analyzer`

latest-rust-analyzer:
	#curl -L https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz | gunzip -c - > ~/.local/bin/rust-analyzer
	curl -L https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-pc-windows-msvc.zip --output-dir ~/Incoming
latest-rust-analyzer-linux:
	wget -c https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz -O /tmp/rust-analyzer.gz
	gunzip --to-stdout /tmp/rust-analyzer.gz > /tmp/rust-analyzer
	chmod +x /tmp/rust-analyzer
	file /tmp/rust-analyzer
	#curl -L https://github.com/rust-analyzer/rust-analyzer/releases/latest/download/rust-analyzer-x86_64-unknown-linux-gnu.gz | gunzip -c - > /tmp/rust-analyzer

aria2 *uri:
	crate-patches aria2 --rpc-secret `rg '\brpc-secret\b' ~/.aria2/aria2.conf | grep -Po '=\K[^\s]+'` {{uri}}

analyzer-from11:
	rsync 11:/opt/bin/rust-analyzer /opt/bin

nvim-listen DIR='/xhome/scripts/gitconfig-https2git':
	nvim --headless --listen 192.168.11.11:6666 +cd\ {{DIR}}

static-web-server root="/tmp":
	static-web-server -g debug --host 0.0.0.0 --port 8080 --root $1

dufs Dir="`date +%y%b`" BaseDir="/home/edu/Aria2":
	test -d "{{BaseDir}}/{{Dir}}" || mkdir "{{BaseDir}}/{{Dir}}"
	dufs -A "$( fd -d1 -iI {{Dir}} . {{BaseDir}} | sk -i )"
	#@echo {{Dir}}
	#@echo {{Dir}}

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

ffmpeg-ca Pat='' Ext='mp4':
	# just ffmpeg-ca 《复杂
	fd -d1 -tf -Ie {{Ext}} '{{Pat}}' -x ffmpeg-ca-copy

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


save_page output page_url:
	#--no-css --no-images
	#--no-audio --no-video --no-frames --no-fonts --no-js --no-metadata
	monolith --no-images --no-js --no-frames --no-metadata --no-fonts --no-audio --no-video --isolate -o {{output}} {{page_url}}

# ffmpeg -i Copy.mp4 -c:v h264_nvenc -c:a copy  output.mkv


get-nushell:
	# https://github.com/nushell/nushell/releases/download/0.88.1/nu-0.88.1-x86_64-pc-windows-msvc.zip

vdhcoapp:
	wget -c https://github.com/aclap-dev/vdhcoapp/releases/download/v2.0.19/vdhcoapp-windows-x86_64-installer.exe

step_certificate:
	#!/bin/bash


secret := "CTRKew35ltwdWhGv9WF10lJ06oYBZKzACYhANx7QXPZpvBvCNZbq161xHg2rKhcp"

@create-token secret=secret:
    jwt encode --kid "1209109290" --secret={{secret}} "$(cat payload.json)"

prometheus-grafana-start:
	#!/bin/sudo /bin/bash
	systemctl start prometheus-node-exporter.service
	systemctl start prometheus.service
	systemctl start grafana.service
prometheus-grafana:( prometheus-grafana-start)
	systemctl status prometheus-node-exporter.service
	systemctl status prometheus.service
	systemctl status grafana.service

github-release-latest Href:
	xh {{Href}}/releases/latest | tee /tmp/ |rg ^location

join a b:
	echo {{join("hello/foo", a,b)}}

_rust-script-cmd_lib *va:
	#!/usr/bin/env rust-script
	//! [dependencies]
	//! cmd_lib = "1"
	//! clap = { version="4", features=[ "derive" ] }
	//! tracing-subscriber = { version = "0.3", features = ["env-filter"] }
	//! time = { version = "0", features = ["formatting", "macros", "parsing"] }
	//! convert_case = "0.6"
	//
	use clap::{arg, command, ArgAction, Parser, Subcommand, ValueEnum};
	use cmd_lib::*;
	use std::io::{BufRead, BufReader};
	use std::path::{Path};
	//
	const PKG_NAME: &str = env!("CARGO_PKG_NAME");
	//const LOG_FILTER: &str = const_format::formatcp!("{PKG_NAME}=debug");
	//
	macro_rules! log_filter {
		() => {
			log_filter("debug").into()
		};
	}
	fn log_filter(lev:&str) -> String {
		use convert_case::{Case, Casing};
		let parts = PKG_NAME.rsplit_once('_').map(|(a, b)| (a, "_", b));
		let (name, sep, tail) = parts.unwrap_or((PKG_NAME, "", ""));
		format!("{}{sep}{tail}={lev}", name.to_case(Case::Snake))
	}
	//
	#[derive(Parser, Debug)]
	#[command(version, author, about, long_about = None)]
	struct CommandArgs {
		#[arg(short, long, value_enum, default_value = "fast")]
		mode: Mode,
		#[arg(short, long, action = ArgAction::Count, default_value = "0")]
		verbose: u8,
		#[clap(default_value = ".")] //#[clap(short, long, required = false, default_value = ".")]
		target_dir: PathBuf, //#std::borrow::Cow<'static, str>
		#[command(subcommand)]
		sub: Option<Sub>,
	}
	#[derive(Copy, Clone, ValueEnum, Debug)]
	enum Mode {
		Fast,
		Slow,
	}
	#[derive(Subcommand, Debug)]
	enum Sub {
		Add {
			#[arg( value_parser = clap::value_parser!(u16).range(1..))]
			nums: Vec<u16>,
		},
		Sub {
			#[arg(short, long)]
			num: u16,
		},
	}
	//
	#[cmd_lib::main]
	fn main() -> CmdResult {
		let clap = dbg!(CommandArgs::parse());
		match clap.sub {
			Some(Sub::Add { .. }) => {}
			Some(Sub::Sub { .. }) => {}
			None => {}
		}
		use tracing_subscriber::layer::SubscriberExt;
		use tracing_subscriber::util::SubscriberInitExt;
		use tracing_subscriber::{fmt::layer, registry, EnvFilter};
		let env_filter = EnvFilter::try_from_default_env().unwrap_or_else(|_| log_filter!());
		registry().with(env_filter).with(layer()).init();
		//
		let target_dir = clap.target_dir.as_path();
		info!("Top 10 biggest DirEntry in: {}", target_dir.display());
		let top_n = run_fun!(du -ah $target_dir | sort -hr | head -n 10)?;
		println!("{}", top_n);
		println!();
		//
		#[rustfmt::skip]
		let mut output = spawn_with_output!(journalctl --no-tail --no-pager)?;
		output.wait_with_pipe(&mut |pipe| {
			let lines = BufReader::new(pipe).lines().take(10);
			for line in lines.filter_map(Result::ok) {
				if line.contains("usb") {
					println!("{}", line);
				}
			}
			println!();
		})?;
		//
		// datetime!(2020-01-02 03:04:05 +06:07:08).format(&format)?,
		let fmt = time::macros::format_description!(
			"[year]-[month]-[day] [hour]:[minute]:[second] [offset_hour sign:mandatory]:[offset_minute]:[offset_second]"
		);
		let time_now = time::OffsetDateTime::now_utc().format(fmt).unwrap();
		//
		#[cfg_attr(any(), rustfmt::skip)]
		run_cmd!(fortune-kind ; echo; echo $PKG_NAME $time_now)?;
		Ok(())
	}
rust-script-cmd_lib *va="/tmp": ( _rust-script-cmd_lib va )

