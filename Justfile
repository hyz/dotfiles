
hello:
    @echo hello

default:
	xsearch-dbup a.xlsx 2>/dev/null |tee a.txt |wc

xsearch-dbup:
	xsearch-dbup a.xlsx tyun
    ssh tyun 'psql myt1 -c "SELECT COUNT(*) FROM warelis"'

help:
    @ echo 'export LS_COLORS="$(vivid generate molokai)"'

