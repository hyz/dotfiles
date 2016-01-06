
for x in 2016010{0..9}; do
    fn=~/../HS/$x.7z
    [ -r "$fn" ] || continue
    [ -e "xdetail/$x" ] && continue

    7z x -otmp/ $fn && bin/x1 tmp/$x > xdetail/$x
done

