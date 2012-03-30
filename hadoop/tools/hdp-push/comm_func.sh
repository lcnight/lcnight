#color control
r_b="\e[1;31;40m"   # red
b_b="\e[1;34;40m"   # blue
end="\e[0m";        # escape end
#echo -e "$r_b aaa $end"

function get_day_time() 
{
    if [[ $# < 1 ]]; then return; fi

    if [[ $1 == '.' || ! $1 =~ [[:digit:]]{8}$ ]]; then
        echo ""
    else
        len=`expr length $1`
        beg=$((len - 8));
        str=$1
        echo ${str:beg}
    fi
}
#get_day_time '.'
#get_day_time /home/data
#get_day_time /home/data/20120329

get_prev_days()
{ # input: 120323 / 20120323
    DAYS=1
    if [[ $# > 1 ]]; then DAYS=$2; fi
    day_tmp=$1
    day_arg_len=`expr length $1`
    if [[ $day_arg_len == 6 ]]; then
        day_tmp=`printf "20%s-%s-%s" ${1:0:2}  ${1:2:2}  ${1:4:2}`
    fi
    next_tmp=`date -d "$day_tmp $DAYS days ago"`
    echo `date -d "$next_tmp" +%Y%m%d`
}

get_next_days()
{ # input: 120323 / 20120323
    DAYS=1
    if [[ $# > 1 ]]; then DAYS=$2; fi
    day_tmp=$1
    day_arg_len=`expr length $1`
    if [[ $day_arg_len == 6 ]]; then
        day_tmp=`printf "20%s-%s-%s" ${1:0:2}  ${1:2:2}  ${1:4:2}`
    fi
    next_tmp=`date -d "$day_tmp $DAYS days"`
    echo `date -d "$next_tmp" +%Y%m%d`
}
#get_next_days 120323 2
#get_prev_days 120323 9
#get_next_days 20120323 10
#get_prev_days 20120323

date2stamp () {
    date --utc --date "$1" +%s
}

stamp2date () {
    date --utc --date "1970-01-01 $1 sec" "+%Y-%m-%d %T"
}

dateDiff () {
    case $1 in
        -s)   sec=1;      shift;;
        -m)   sec=60;     shift;;
        -h)   sec=3600;   shift;;
        -d)   sec=86400;  shift;;
        *)    sec=86400;;
    esac
    dte1=$(date2stamp $1)
    dte2=$(date2stamp $2)
    diffSec=$((dte2-dte1))
    if ((diffSec < 0)); then abs=-1; else abs=1; fi
    echo $((diffSec/sec*abs))
}
