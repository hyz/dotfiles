
# export VARIANT=release PLATS=k400,cvk350c,cvk350t
# make prepare
# make rbuild
# make rar RARPWD=XXX

REPO    ?= svnchina/androidbarcode
VARIANT ?= release
PLATS   ?= k400,cvk350c,cvk350t

prepare init:
	hzmake.sh prepare $(REPO) $(VARIANT) -P $(PLATS)

rbuild:
	hzmake.sh sync-up $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS)
rar:
	hzmake.sh rar $(REPO) $(VARIANT) -P $(PLATS) -p $(RARPWD)

rbuildx:
	hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS)

showvars:
	@echo $(REPO) $(VARIANT) $(PLATS) $(RARPWD)

.PHONY: showvars rbuild rbuildx prepare init sync-up rar


