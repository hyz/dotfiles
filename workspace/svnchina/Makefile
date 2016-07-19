# make BUILDDIR=/mnt/hgfs/home/svnchina VARIANT=release REPO=androidbarcode
# make BUILDDIR=/mnt/hgfs/home/svnchina VARIANT=release REPO=androidbarcode upload
#

prepare:
	cvkbuild.sh -a $(VARIANT) $(REPO) $(BUILDDIR)

upload:
	cvkbuild.sh --upload -a $(VARIANT) $(REPO) $(BUILDDIR)

