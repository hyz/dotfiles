
init prepare:
	hzmake.sh init androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS)

rbuild:
	hzmake.sh rbuild androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS)
upbuild:
	hzmake.sh sync-up androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh rbuild androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS)
rar:
	hzmake.sh rar androidbarcode $(BUILDDIR) $(VARIANT) -P $(PLATS) -p $(RARPWD)

.PHONY: rbuild prepare init sync-up rar

# make BUILDDIR=$HOME/build REPO=androidbarcode VARIANT=release
# make BUILDDIR=$HOME/build REPO=androidbarcode VARIANT=release rbuild
# make BUILDDIR=$HOME/build REPO=androidbarcode VARIANT=test
#
# export BUILDDIR=$HOME/build RARPWD=XXX PLATS=k400,cvk350c,cvk350t
# hzmake.sh init      androidbarcode $BUILDDIR -v release --plats $PLATS
# hzmake.sh sync-up   androidbarcode $BUILDDIR -v release --plats $PLATS
# hzmake.sh rbuild    androidbarcode $BUILDDIR -v release --plats $PLATS
# hzmake.sh sync-down androidbarcode $BUILDDIR -v release --plats $PLATS
# hzmake.sh rar       androidbarcode $BUILDDIR -v release --plats $PLATS -p $RARPWD

