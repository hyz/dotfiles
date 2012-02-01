
T ?= /cygdrive/g

Kitd = $(T)/i51/i51KIT

all: Tcard
	@echo "Tcard: $(T)"
	@[ -e "$(Kitd)/i51TEST" ] && echo "i51TEST"
	@[ -e "$(T)/i51/i51.log" ] && echo "i51.log"
	@echo "make <log|test|clean>"

log: Tcard
	cp -t. $(T)/i51/i51.log && echo -n > $(T)/i51/i51.log

test: Tcard
	touch $(T)/i51/i51.log
	touch $(T)/i51/i51KIT/i51TEST
	l=$Kitd/i51TEST ; [ -e $l ] && echo "$l YES" || echo "$l NO"

Tcard: $(T)
	@[ -d $(Kitd) ] || mkdir -p $(Kitd)
	@case "$(toggle)" in log) toggle-file-exist $T/i51/i51.log ;; server) toggle-file-exist $T/i51/i51TEST ;; esac

# find $$d -name "*.kit" -o -name "*.i51" -o -name "*.dll" -exec cp -vft "$(Kitd)/$$d" '{}' \; ; 
Dirs=../i51SYS PKiGAPI 51pkpay 51pkGame paydo app-boot
sync: Tcard
	for d in $(Dirs); do \
		[ -d "$$d" ] || continue ; \
		[ -d "$(Kitd)/$$d" ] || mkdir -p "$(Kitd)/$$d" ; \
		find $$d | grep -E "(\.kit|\.i51|\.re|\.dll)$$" |xargs cp -vt "$(Kitd)/$$d" ; \
	done
	cp -vRt $(Kitd) Fonts

clean: Tcard
	for x in g h i; do [ -e /cygdrive/$$x/i51sn ] && rm -rf /cygdrive/$$x/i51sn ; done ; true
	rm -rf $(T)/i51/i51KIT/51pkpay ; true
	rm -rf $(T)/i51/i51SYS ; true
	rm -vf $t/i51/i51SYS/*.anti
	find $(T)/i51

#-- #!/bin/sh
#-- 
#-- t=$T
#-- if [ -n "$1" ]; then t=$1; fi
#-- [ -z "$t" ] && exit 1
#-- 
#-- echo $t ; [ -d "$t" ] || exit 2
#-- 
#-- ls -v $t/i51/i51SYS 2>/dev/null
#-- rm -vf $t/i51/i51SYS/*.anti
#-- cp -vR ../../i51SYS $t/i51
#-- cp -vR ../PKiGAPI $t/i51/i51KIT
#-- 
#-- d=$t/i51/i51KIT/51pkpay ; [ -d $d ] || mkdir $d || exit 9
#-- for s in 51pkpay kitpay; do
#--     cp -vR ../$s/*.{i51,kit,re} $d 2>/dev/null
#--     cp -vR ../$s/Debug/*.dll $d 2>/dev/null
#-- done
#-- 
#-- s=paydo
#-- d=$t/i51/i51KIT/paydo ; [ -d $d ] || mkdir $d || exit 9
#-- cp -vR ../$s/*.{i51,kit,re} $d 2>/dev/null
#-- cp -vR ../$s/Debug/*.dll $d 2>/dev/null
#-- 
#-- cp -vR ../Fonts $d/..
#-- 
#-- d=$t/i51/i51KIT/app-boot
#-- [ -d "$d" ] || mkdir $d
#-- cp -vR ../app-boot/*.i51 $d
#-- 
#-- l=$t/i51/i51.log ; cp -vt. $l; date > $l
#-- 
#-- touch "$t/i51/`date +%F`"
#-- 
#-- l=$t/i51/i51KIT/i51TEST ; [ -e $l ] && echo "$l YES" || echo "$l NO"

