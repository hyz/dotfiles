build/Game14: src/Game14
	cd src/Game14 && svn up 
	/home/wood/bin/hzmake Apk.copytree2 Game14 src/Game14 build/Game14
	/home/wood/bin/hzmake Apk.edit_build_tree Game14

build/Game14/1.3.67-r817: build/Game14
	mkdir -p $@ 

build/Game14/1.3.67-r817/Game.apk: build/Game14/1.3.67-r817
	/home/wood/bin/hzmake waitfiles build/Game14/1.3.67-r817/Game.apk build/Game14/libs/armeabi-v7a/libBarcode.so build/Game14/libmtkhw.so
	/home/wood/bin/hzmake copyfiles2 build/Game14/1.3.67-r817 build/Game14/libs/armeabi-v7a/libBarcode.so build/Game14/libmtkhw.so
	rsync -vRr build/Game14/1.3.67-r817 root@192.168.2.113:/home/wood
	rsync -v Makefile root@192.168.2.113:/home/wood

clean_Game14: 
	/home/wood/bin/hzmake Apk.clean Game14

