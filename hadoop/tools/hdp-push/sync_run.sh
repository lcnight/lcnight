#! /bin/bash

ENV_FILE="~/.environment"
if [[ -f $ENV_FILE ]]; then
    source $ENV_FILE
fi
source comm_func.sh

SRC_DIR_PARENT=/home/lc/hadoop/wireless/data/
if [[ ! -d $SRC_DIR_PARENT ]]; then
    echo "Source Directory not exist: $SRC_DIR_PARENT";
    echo; exit;
fi
LAST_DAYS=7
TIME_TOKEN="-mtime -$LAST_DAYS"
SRC_DIR_PARENT=`cd $SRC_DIR_PARENT && pwd`
SRC_DIR_LEN=`expr length $SRC_DIR_PARENT`
DST_DIR_PARENT=/user/lc
SYNC_CMD='bin/hadoop hdp_push'
#BEG_CMD='bin/haddoop jar GetUniqClause.jar GetUniqClause'
BEG_CMD='false'
DO_CMD='wo qu'
INPUT_PARENT='/wireless'
OUTPUT_PARENT='/output/wireless'
DB_OPTS='root:ta0mee@10.1.1.60/db_wireless_dim'

if [[ $1 == '-h' ]]; then
    echo "usage: `basename $0` [ n days ago ]"
    echo
    exit
elif [[ $1 == '-v' ]]; then
    VERBOSE=true
elif [[ $1 ]]; then
    LAST_DAYS=$1
fi

echo "merge process: start at `date`"
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

echo "mapred process: start at `date`"
DAYS=`find $SRC_DIR_PARENT -maxdepth 1 $TIME_TOKEN`
for d in $DAYS; do
    day_time=`get_day_time $d`
    if [[ $day_time == "" ]]; then
        continue
    else
        # prepare environment
        beg_cmd_str="$BEG_CMD $INPUT_PARENT/$d $OUTPUT_PARENT/$d -b $DB_OPTS"
        if [[ $VERBOSE ]]; then
            echo $beg_cmd_str; 
        fi
        eval $beg_cmd_str
        ECODE=$?
        if [[ $ECODE != 0 ]]; then
            echo "error detected, code: $ECODE";    exit $ECODE
        fi

        # do real mapred task
        job_cmd_str="$DO_CMD $day_time"
        if [[ $VERBOSE ]]; then
            echo $job_cmd_str; echo
        fi
        eval $job_cmd_str
        ECODE=$?
        if [[ $ECODE != 0 ]]; then
            echo "error detected, code: $ECODE";    exit $ECODE
        fi
    fi
done

echo "all process: end at `date`"
echo
