# color control
r_b="\e[1;31;40m"   # red
b_b="\e[1;34;40m"   # blue
end="\e[0m";        # escape end
#echo -e "$r_b aaa $end"

# global env variable
export LANG=en_US.UTF-8
export LC_ALL=en_US.UTF-8

# logging control
DEBUG=1
Debug () {
    if [ $DEBUG -eq 1 ]; then
        if [[ $# == 0 ]]; then echo; return; fi
        echo -e "[`date '+%F %T'`] $*";
    fi
}

Info () {
    if [[ $# == 0 ]]; then echo; return; fi

    echo -e "[`date '+%F %T'`] $*" ;
}

# datetime process contol
function get_day_time()
{
    if [[ $# < 1 ]]; then return; fi

    if [[ $1 == '.' || ! $1 =~ [[:digit:]]{8}$ ]]; then
        echo ""
    else
        local len=`expr length $1`
        local beg=$((len - 8));
        local str=$1
        echo ${str:beg}
    fi
}
#get_day_time '.'
#get_day_time '20120329'
#get_day_time /home/data
#get_day_time /home/data/20120329

get_prev_days()
{ # YYMMDD | YYYYMMDD
    local DAYS=1
    if [[ $# > 1 ]]; then DAYS=$2; fi
    local day_tmp=$1
    local day_arg_len=`expr length $1`
    if [[ $day_arg_len == 6 ]]; then
        day_tmp=`printf "20%s-%s-%s" ${1:0:2}  ${1:2:2}  ${1:4:2}`
    fi
    local next_tmp=`date -d "$day_tmp $DAYS days ago"`
    echo `date -d "$next_tmp" +%Y%m%d`
}
get_prev_months()
{ # YYYYMM n
    if [[ $# < 1 || ${#1} != 6 || $1 =~ [^0-9] || $2 =~ [^0-9] ]]; then return 0; fi

    if [[ $2 == "" ]] ; then
        local prev=1
    else
        local prev=$2
    fi

    echo `date -d "${1}01 $prev months ago" '+%Y%m'`
}
#get_prev_months 201206
#get_prev_months 201206 3

get_next_days()
{ # input: 120323 / 20120323
    local DAYS=1
    if [[ $# > 1 ]]; then DAYS=$2; fi
    local day_tmp=$1
    local day_arg_len=`expr length $1`
    if [[ $day_arg_len == 6 ]]; then
        day_tmp=`printf "20%s-%s-%s" ${1:0:2}  ${1:2:2}  ${1:4:2}`
    fi
    local next_tmp=`date -d "$day_tmp $DAYS days"`
    echo `date -d "$next_tmp" +%Y%m%d`
}
#get_prev_days 20120323
#get_prev_days 120323 9
#get_next_days 20120323 10
get_prev_months ()
{ # YYYYMM n
    if [[ $# < 1 || ${#1} != 6 || $1 =~ [^0-9] ]]; then return 0; fi

    if [[ $2 == "" ]] ; then
        next=1
    else
        next=$2
    fi

    echo `date -d "${1}01 $next months ago" '+%Y%m'`
}
#get_next_months 201206
#xxx=`get_next_months 201206 3`
#echo $xxx

get_next_months ()
{ # YYYYMM n
    if [[ $# < 1 || ${#1} != 6 || $1 =~ [^0-9] ]]; then return 0; fi

    if [[ $2 == "" ]] ; then
        next=1
    else
        next=$2
    fi

    echo `date -d "${1}01 $next months" '+%Y%m'`
}

date2stamp ()
{
    date --utc --date "$1" +%s
}

stamp2date ()
{
    date --utc --date "1970-01-01 $1 sec" "+%Y-%m-%d %T"
}

dateDiff ()
{
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

# space separated mail list
recips='lc@taomee.com'
SendMail ()
{
    local errSubject="$1"
    local errMsg="$2"
    Info "mail '$errSubject' to $recips"
    echo -e "$errMsg" | mail -s "$errSubject" $recips
}
# logic control
CheckResult ()
{ # 1(code) 2(subject) 3(errMsg)
    if [[ $# < 1 ]];then return 0; fi
    if [[ $1 != 0 ]]; then  ## 0 => OK
        Debug "[errcode:$1] $2\n$3"
        SendMail "[errcode:$1] $2" "$3"
		exit $1
    fi
}
AbortProcess () {
   local errSubject="$1"
   local errMsg="$2"

   # sendout alert email
   SendMail $errSubject $errMsg
   exit -1
}

###  example commandline parameter parser
#while getopts hvc: opt; do
    #case $opt in
        #c) echo condition: $OPTARG;;
        #v) verbose=1;;
        #h) Usage;;
        #?) Usage;;
        #*) Usage;;
    #esac
#done
#shift $(($OPTIND - 1))

# change current directory to absolute path
#cd `cd $(dirname $0) && pwd`
