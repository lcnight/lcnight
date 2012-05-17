#! /bin/bash

function prc_usage
{
    echo "Usage: `basename $0` <start|stop|restart|status>"
    echo;
    exit
}

if [[ $# < 1 ]]; then
    prc_usage
fi

red_clr="\033[31m"
grn_clr="\033[32m"
end_clr="\033[0m"

bin_token="search_engine"
prc_flds="user,pid,ppid,sid,stat,pcpu,pmem,vsz,wchan:20,cmd"

function prc_start
{
    ./bin/$bin_token  ./conf/search_engine.conf

    for ((cnt=0 ; cnt<2 ; cnt++)) ; do
        sleep 1
        pid=`cat ./daemon.pid`
        result=`ps -p $pid | wc -l`
        if [ $result -gt 1 ]; then
            printf "$grn_clr%50s$end_clr\n" $bin_token" is running"
            break;
        else
            printf "$red_clr%50s$end_clr\n" $bin_token" is not running"
        fi
    done
}

function prc_stop
{
    pid=`cat ./daemon.pid`
    result=`ps -p $pid | wc -l`

    if [ $result -gt 1 ]; then
        kill `cat ./daemon.pid`
        sleep 1
        pid=`cat ./daemon.pid`
        result=`ps -p $pid | wc -l`
        if [ $result -gt 1 ]; then
            printf "$red_clr%50s$end_clr\n" "$bin_token is still running"
        else
            printf "$grn_clr%50s$end_clr\n" "$bin_token  has been stopped"
        fi
    else
        printf "$red_clr%50s$end_clr\n" "$bin_token is not running"
    fi
}

function prc_status()
{
    ps -e -o $prc_flds | grep -v 'grep' | grep -E "$bin_token|WCHAN"
}

case $1 in
    start)
    prc_start;
    ;;
    stop)
    prc_stop;
    ;;
    restart)
    prc_stop; prc_start;
    ;;
    status)
    prc_status;
    ;;
    *)
    usage
    ;;
esac
