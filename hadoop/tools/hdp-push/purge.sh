#! /bin/bash

source comm_func.sh

Purge_Days=30
if [[ $# < 1 || $1 == -h ]]; then
    echo "Usage: `basename $0` <purge dir> [purge days]"
    exit
else 
    Purge_Dir=$1
    if [[ $# > 1 ]]; then Purge_Days=$2; fi
fi

NOW_DAY=`date +%Y%m%d`
PURGE_DAY=`get_prev_days $NOW_DAY $Purge_Days` 

echo "Purge Day dirs before $PURGE_DAY"
echo

daystr_list=`find $Purge_Dir -maxdepth 1`
for ds in $daystr_list; do
    d=`get_day_time $ds`
    if [[ $d != "" && $d < $PURGE_DAY ]]; then
        rm -rvf $ds;
    fi
done 

echo
