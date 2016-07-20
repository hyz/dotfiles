# make BUILDDIR=/mnt/hgfs/home/svnchina/build VARIANT=release REPO=androidbarcode
# make BUILDDIR=/mnt/hgfs/home/svnchina/build VARIANT=release REPO=androidbarcode upload
# make BUILDDIR=/mnt/hgfs/home/svnchina/build VARIANT=test REPO=androidbarcode
#

prepare:
	cvkbuild.sh -a $(VARIANT) $(REPO) $(BUILDDIR)

upload:
	cvkbuild.sh --upload -a $(VARIANT) $(REPO) $(BUILDDIR)

.PHONY: upload prepare

