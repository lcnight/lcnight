#! /bin/bash

source comm_func.sh

LAST_DAYS=1
TIME_TOKEN="-mtime -$LAST_DAYS"
SRC_DIR_PARENT=~/hadoop/data/
SRC_DIR_PARENT=`cd $SRC_DIR_PARENT && pwd`
SRC_DIR_LEN=`expr length $SRC_DIR_PARENT`
DST_DIR_PARENT=/user/lc
SYNC_CMD='bin/hadoop hdp_push'
DO_CMD='wo lie ge qu'

if [[ $1 == '-h' ]]; then
    echo "usage: `basename $0` [ n days ago ]"
    echo
    exit
elif [[ $1 == '-v' ]]; then
    VERBOSE=true
elif [[ $1 ]]; then
    LAST_DAYS=$1
fi

FILES=`find $SRC_DIR_PARENT $TIME_TOKEN`
# merge all found files
for f in $FILES; do
    if [[ -d $f ]]; then
        echo "ignore dir: $f"
    elif [[ -f $f && -r $f && -s $f ]]; then
        echo "merge file: $f"
        basefile=${f:$SRC_DIR_LEN}
        cmd_str="$SYNC_CMD -m $f -f $DST_DIR_PARENT$basefile"
        if [[ $VERBOSE ]]; then
            echo $cmd_str; echo
        fi
        eval $cmd_str
    else
        echo "no process file: $f"
    fi
done

DAYS=`find $SRC_DIR_PARENT -maxdepth 1 $TIME_TOKEN`
for d in $DAYS; do
    day_time=`get_day_time $d`
    if [[ $day_time == "" ]]; then
        continue
    else
        job_cmd_str="$DO_CMD $day_time"
        if [[ $VERBOSE ]]; then
            echo $job_cmd_str; echo
        fi
        eval $job_cmd_str
    fi
done

echo
