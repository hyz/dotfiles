
REPO    ?= androidbarcode
VARIANT ?= release
PLATS   ?= k400,cvk350c,cvk350t

help:
	@echo Example:
	@echo '  ' make prepare '#' VARIANT=test
	@echo '  ' make PLATS=k400,cvk350c,cvk350t rbuild
	@echo '  ' make PLATS=k400,cvk350c,cvk350t rar RARPWD=XXX
	@echo '  ' make version-up '&&' make version-commit
	@echo
	@hzmake.sh info $(REPO) $(VARIANT)

prepare:
	svn up $(REPO)
	hzmake.sh prepare $(REPO) $(VARIANT)
	@grep word howto.txt

rbuild:
	hzmake.sh sync-up $(REPO) $(VARIANT) \
		&& hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS)
rar:
	hzmake.sh $@ $(REPO) $(VARIANT) -P $(PLATS) -p $(RARPWD)

version-commit:
	hzmake.sh version-up $(REPO) $(VARIANT) \
		&& hzmake.sh version-commit $(REPO) $(VARIANT)
#version-up:
#	hzmake.sh $@ $(REPO) $(VARIANT)
#version-reset:
#	hzmake.sh $@ $(REPO) $(VARIANT)

rbuild-1:
	hzmake.sh rbuild $(REPO) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(VARIANT) -P $(PLATS)

showvars:
	@echo $@ $(REPO) $(VARIANT) $(PLATS) $(RARPWD)

.PHONY: showvars rbuild rbuildx prepare sync-up rar version-up version-reset version-commit help


