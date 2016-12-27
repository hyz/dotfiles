
REPO    ?= BarcodeAndroid
PLATS   ?= k400,cvk350c,cvk350t
VARIANT ?= release

help:
	@echo Example:
	@echo '  ' make prepare '#' VARIANT=test
	@echo '  ' make PLATS=k400,cvk350c,cvk350t rbuild
	@echo '  ' make PLATS=k400,cvk350c,cvk350t rar RARPWD=XXX
	@echo '  ' make version-commit
	@echo
	@$(MAKE) showvars
	@echo
	@hzmake.sh info $(REPO) $(VARIANT)
	@grep word howto.txt
	@cp -vt /samba/release1/doc/ $(REPO)/doc/*.xls

prepare:
	svn up $(REPO)
	hzmake.sh prepare $(REPO) $(VARIANT)
	@grep word howto.txt

release:
	@cp -vt /samba/release1/doc/ $(REPO)/doc/*.xls
	$(MAKE) sync-up
	$(MAKE) rbuild
	$(MAKE) sync-down
	test -z "$(RARPWD)" || $(MAKE) rar

sync-up:
	hzmake.sh $@ $(REPO) $(VARIANT)
rbuild:
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS)
sync-down:
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS)
rar:
	test -n "$(RARPWD)"
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS) -p $(RARPWD)

version-commit:
	hzmake.sh version-commit $(REPO) $(VARIANT)

showvars:
	@echo Current variables:
	@echo '  ' REPO=$(REPO)
	@echo '  ' PLATS=$(PLATS)
	@echo '  ' VARIANT=$(VARIANT)
	@echo '  ' RARPWD=$(RARPWD)

ndk-build:
	hzmake.sh $@ $(REPO) $(VARIANT)

#hzmake.sh version-up $(REPO) $(VARIANT)
#version-up:
#	hzmake.sh $@ $(REPO) $(VARIANT)
#version-reset:
#	hzmake.sh $@ $(REPO) $(VARIANT)
# rbuild:
#	hzmake.sh sync-up $(REPO) $(VARIANT) \
#		&& hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
#		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS) \
#		&& [ -n "$(RARPWD)" ] && $(MAKE) rar
.PHONY: rbuild release prepare sync-up sync-down rar version-up version-reset version-commit help showvars

### NewVer=1.3.42 Vertag=GZEnUniqueV make prepare
###

