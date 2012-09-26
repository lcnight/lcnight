#! /bin/bash
cd `cd $(dirname $0) && pwd`
source comm_func.sh

Usage() {
    cat<<HERE_DOC
Usage: `basename $0` [options] <process dir>
        -b          bak days
        -p          purge days
        -m          find files in modified time mode
        -v          run in verbose mode
        -h          show help and exit

HERE_DOC
    exit
}

mtimemode=0
bak_days=7
purge_days=30
while getopts hvmp:b: opt; do
    case $opt in
        b) bak_days=$OPTARG;;
        p) purge_days=$OPTARG;;
        m) mtimemode=1;;
        v) verbose=1;;
        h) Usage;;
        ?) Usage;;
        *) Usage;;
    esac
done
shift $(($OPTIND - 1))
if [[ $# == 0 ]]; then Usage; fi
if [[ $# -gt 0 ]]; then processDir=$1; fi

Info "Bakup $bak_days and Purge $purge_days Days files"

# change working dir
if [[ $verbose == 1 ]]; then Info "change working dir: $processDir" ; fi
cd $processDir;
if [[ $mtimemode == 1 ]]; then
    if [[ $# -gt 1 ]]; then filePattern="-name $2"; fi
    filterstr="-maxdepth 1 -type f $filePattern"
    if [[ $verbose == 1 ]]; then Info "filter pattern: $filterstr" ; fi
    find . $filterstr -mtime +$purge_days | xargs rm -fv {} 2>/dev/null
    find . $filterstr -mtime +$bak_days | xargs gzip -v {} 2>/dev/null
else
    NOW_DAY=`date +%Y%m%d`
    BAK_DAY=`get_prev_days $NOW_DAY $bak_days`
    PURGE_DAY=`get_prev_days $NOW_DAY $purge_days` 
    if [[ $verbose == 1 ]]; then 
        Info "purge files before day: $PURGE_DAY, bak before day: $BAK_DAY"
    fi

    for ds in *; do
        d=`get_day_time $ds`
        if [[ "$d" == "" ]]; then 
            Info "sikp file: $ds"
            continue; 
        fi

        if [[ $d -lt $PURGE_DAY ]]; then
            rm -rvf $ds;
        elif [[ $d -lt $BAK_DAY ]]; then
            gzip $ds
        fi
    done 
fi

Info 'all done'
