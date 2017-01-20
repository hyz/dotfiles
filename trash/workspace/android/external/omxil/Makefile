
all:
	ONE_SHOT_MAKEFILE=external/foo/Android.mk make -C ~/01work/mt6580/alps -f build/core/main.mk all_modules

zip:
	cd ../.. && tar czf ~/01work/mt6580-alps-external-foo-$(shell date +%Y%m%d).tgz \
		external/foo/*.cpp external/foo/Android.mk external/foo/Makefile

