#! /bin/bash
###############################################################################
#                    run supplied command for ever                            #
###############################################################################

Usage () {
fname=`basename $0`
    cat <<HEREDOC
$fname [options] command-string
    -i seconds      interval(seconds) between running
    -t endtime      run command until specified time
example: 
$fname -t '2013-01-06 11:0:0' 'ps -ef | grep java'
HEREDOC
}

verbose=0;  endtime=''; interval=60
while getopts vht:i: opt; do
    case $opt in 
        i) interval=$OPTARG;;
        t) endtime=$OPTARG;;
        v) verbose=1;;
        h) Usage;;
    esac
done
shift $(($OPTIND - 1))

cmdstring="$1"
if [[ "$endtime" != "" ]]; then endtimestamp=`date -d "$endtime" +%s 2>/dev/null`; fi

while true ; do
    if [[ "$endtimestamp" != '' && `date +%s` -gt $endtimestamp ]]; then break; fi
    if [[ $verbose == 1 ]]; then echo "TS[`date +%s`]" ; fi
    eval $cmdstring
    sleep $interval
done

