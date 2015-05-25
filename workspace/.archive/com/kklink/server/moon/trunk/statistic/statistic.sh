#!/bin/sh
if [ "$1" = "pair" ]
then
    template="SELECT count(*) FROM relation t1,relation t2 WHERE t1.user_phone=t2.lover_phone AND t1.lover_phone=t2.user_phone and t1.attitude=17 and t2.attitude=17"
else
    case $2 in
        -n)
        begin=$3
        end=`date -d "$begin $4 days" +%Y-%m-%d`
        echo $begin  $end
        ;;
        -p)
        if [ $3 = "now" ]
        then
            tmp=`date +%Y-%m-%d`
            echo $tmp
        else
            tmp=$3
        fi
        end=$tmp
        begin=`date -d "$end $4 days ago" +%Y-%m-%d`
        echo $begin  $end
        ;;
        -i)
        begin=$3
        if [ $4 = "now" ]
        then
            tmp=`date +%Y-%m-%d`
            echo $tmp
        else
            tmp=$4
        fi
        end=$tmp
        echo $begin  $end
        ;;
        *)
        echo "usage: statistic [sms|regist|live|all|pair] [-n|-p begin_time|end_time|now n]"
        ;;
    esac
    case $1 in
        sms)
        table=sms_records
        field=send_time
        echo ${table} $field;
        ;;
        regist)
        table=users
        field=regist_time
        echo ${table} $field;
        ;;
        live)
        table=users
        field=login_time
        echo ${table} $field;
        ;;
        all)
        sh statistic.sh sms -i $begin $end
        sh statistic.sh regist -i $begin $end
        sh statistic.sh live -i $begin $end
        ;;
        *)
        echo "usage: statistic [sms|regist|live|all|pair] [-n|-p begin_time|end_time|now n]"
	;;
	esac
	if [ -n "$table" ] && [ -n "$field" ] && [ -n "$begin" ] && [ -n "$end" ];then
	template="select count(*) from $table where $field>'$begin' and $field<'$end'"
	fi
fi
#sh statistic.sh live -i 2013-05-10 now
#sh statistic.sh live -p now  60
echo $template
if [ -n "$template" ];then
/home/lindu/server-2/statistic/bin/select "$template"
fi
