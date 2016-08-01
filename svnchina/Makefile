
repo    := $(REPO)
repo    ?= svnchina/androidbarcode
variant := $(VARIANT)
variant ?= release
plats   := $(PLATS)
plats   ?= k400,cvk350c,cvk350t

prepare init:
	hzmake.sh prepare $(repo) $(variant) -P $(plats)

rebuild:
	hzmake.sh rbuild $(repo) $(variant) -P $(plats) \
		&& hzmake.sh sync-down $(repo) $(variant) -P $(plats)

rbuild:
	hzmake.sh sync-up $(repo) $(variant) -P $(plats) \
		&& hzmake.sh rbuild $(repo) $(variant) -P $(plats) \
		&& hzmake.sh sync-down $(repo) $(variant) -P $(plats)
rar:
	hzmake.sh rar $(repo) $(variant) -P $(plats) -p $(RARPWD)

showvars:
	@echo $(repo) $(variant) $(plats) $(RARPWD)

.PHONY: showvars rbuild rebuild prepare init sync-up rar

# export REPO=svnchina/androidbarcode VARIANT=release RARPWD=XXX PLATS=k400,cvk350c,cvk350t
# hzmake.sh init      $REPO release -P $PLATS
# hzmake.sh sync-up   $REPO release -P $PLATS
# hzmake.sh rbuild    $REPO release -P $PLATS
# hzmake.sh sync-down $REPO release -P $PLATS
# hzmake.sh rar       $REPO release -P $PLATS -p $RARPWD

