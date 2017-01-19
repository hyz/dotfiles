
REPO    ?= $(HOME)/release/Game14
PLATS   ?= k400,cvk350c,cvk350t
VARIANT ?= release

help:
	@$(MAKE) -f Game14.mk showvars
	@echo
	@hzmake.sh info $(REPO) $(VARIANT)

release:
	$(MAKE) -f Game14.mk sync-up
	$(MAKE) -f Game14.mk rbuild
	$(MAKE) -f Game14.mk sync-down
	#test -z "$(RARPWD)" || $(MAKE) rar

sync-up:
	hzmake.sh $@ $(REPO) $(VARIANT)
rbuild:
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS)
sync-down:
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS)
#rar:
#	test -n "$(RARPWD)"
#	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS) -p $(RARPWD)

version-commit:
	hzmake.sh version-commit $(REPO) $(VARIANT)

#prepare:
#	svn up $(REPO)
#	hzmake.sh prepare $(REPO) $(VARIANT)
#	@grep word howto.txt

showvars:
	@echo Current variables:
	@echo '  ' REPO=$(REPO)
	@echo '  ' PLATS=$(PLATS)
	@echo '  ' VARIANT=$(VARIANT)
	@echo '  ' RARPWD=$(RARPWD)

#ndk-build:
#	hzmake.sh $@ $(REPO) $(VARIANT)

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

