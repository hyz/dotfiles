
for x in 2016010{0..9}; do
    zf=~/../HS/$x.7z

    [ -r "$zf" ] || continue
    [ ! -e "xbs/$x" ] || continue

    if [ ! -e "tmp/$x" ]; then
        7z x -otmp/ $zf
    fi

    if bin/x5 tmp/$x > xbs/$x ; then
        rm -rf tmp/$x
        echo "$x: OK"
    else
        rm -f xbs/$x
        echo "$x: FAIL"
    fi
done

