# make BUILDDIR=/mnt/hgfs/home/svnchina/build REPO=androidbarcode VARIANT=release
# make BUILDDIR=/mnt/hgfs/home/svnchina/build REPO=androidbarcode VARIANT=release rbuild
# make BUILDDIR=/mnt/hgfs/home/svnchina/build REPO=androidbarcode VARIANT=test
#

prepare:
	hzmake.sh -a $(VARIANT) $(REPO) $(BUILDDIR)

rbuild:
	hzmake.sh --rbuild -a $(VARIANT) $(REPO) $(BUILDDIR)

.PHONY: rbuild prepare

