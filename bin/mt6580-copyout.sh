#!/bin/bash

out="$1"
outdb="$1/database"

[ ! -z "$out" ] || exit 1
[ ! -d "$out" ] || rm -rf "$out"
mkdir -p "$outdb" || exit 2

BPLGUInf=BPLGUInfoCustomAppSrcP_MT6580*

lunch=out/target/product/ckt6580_we_l
objCGEN=$lunch/obj/CGEN/APDB_MT6580*
objETC=$lunch/obj/ETC/$BPLGUInf/$BPLGUInf

#echo    "copy $objCGEN to $outdb"
cp -vt $outdb $objCGEN
#echo    "copy $objETC to $outdb"
cp -vt $outdb $objETC
#echo "copy $lunch to $out" 
find $lunch -maxdepth 1 -type f | xargs cp -vt $out 

