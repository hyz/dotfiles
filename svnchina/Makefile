
init prepare:
	hzmake.sh init $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS)

rebuild:
	hzmake.sh rbuild $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS)

rbuild:
	hzmake.sh sync-up $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh rbuild $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS) \
		&& hzmake.sh sync-down $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS)
rar:
	hzmake.sh rar $(REPO) $(BUILDDIR) $(VARIANT) -P $(PLATS) -p $(RARPWD)

.PHONY: rbuild rebuild prepare init sync-up rar

# export REPO=svnchina/androidbarcode BUILDDIR=$HOME/build RARPWD=XXX PLATS=k400,cvk350c,cvk350t
# hzmake.sh init      $REPO $BUILDDIR release -P $PLATS
# hzmake.sh sync-up   $REPO $BUILDDIR release -P $PLATS
# hzmake.sh rbuild    $REPO $BUILDDIR release -P $PLATS
# hzmake.sh sync-down $REPO $BUILDDIR release -P $PLATS
# hzmake.sh rar       $REPO $BUILDDIR release -P $PLATS -p $RARPWD

