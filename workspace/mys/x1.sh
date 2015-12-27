
for x in 2015121{0..9}; do
    fn=~/../HS/$x.7z
    [ -r "$fn" ] || continue

    7z x -otmp/ $fn && ../bin/x1 tmp/$x > $x
done

