
# echo "61 {\"uid\":$uid,\"token\":\"$tok\",\"bid\":1}"
# echo "63 {\"speed\":10}"

export LD_LIBRARY_PATH=/opt/lib64

bid=${B:-1}

fn=$1
n=$2

tail -$n $fn | while read uid tok; do
fn2=tmp/shk.uid.$uid
    cat >$fn2 <<EoF
/bin/echo "61 {\\"uid\\":$uid,\\"token\\":\\"$tok\\",\\"bid\\":$bid}"

while true; do
    n=\$((\$RANDOM % 6 + 5))
    #n=\$((\$RANDOM % 17 + 1))
    /bin/echo "63 {\\"speed\\":\$n}" || break
    sleep 1
done

EoF

usr/local/bin/shk-racing-proxy -x usr/local/bin/shk-racing-protocol -h 58.67.160.243 -p 8080 $fn2 &

done

control_c() {
    echo End
    killall usr/local/bin/shk-racing-proxy
    exit $?
}
trap control_c SIGINT
while true; do sleep 6 ; done

