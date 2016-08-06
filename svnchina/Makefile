
# make prepare VARIANT=test
# make prepare
# make PLATS=k400,cvk350c,cvk350t rbuild
# make PLATS=k400,cvk350c,cvk350t rar RARPWD=XXX
# make version-up

REPO    ?= androidbarcode
VARIANT ?= release
PLATS   ?= k400,cvk350c,cvk350t

prepare init:
	svn up $(REPO)
	hzmake.sh prepare $(REPO) $(VARIANT)
	@grep word howto.txt

rbuild:
	hzmake.sh sync-up $(REPO) $(VARIANT) \
		&& hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS)
rar:
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS) -p $(RARPWD)

version-up:
	hzmake.sh $@ $(REPO) $(VARIANT)

rbuild-1:
	hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS)

showvars:
	@echo $@ $(REPO) $(VARIANT) $(PLATS) $(RARPWD)

.PHONY: showvars rbuild rbuildx prepare init sync-up rar


