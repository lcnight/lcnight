#! /bin/bash

SRC_DIR_PARENT=.
DST_DIR_PARENT=/user/lc
LAST_DAYS=1

SYNC_CMD='bin/hadoop hdp_push'

if [[ $1 == '-h' ]]; then
    echo "usage: `basename $0` [ n days ago ]"
    echo
    exit
elif [[ $1 == '-v' ]]; then
    VERBOSE=true
elif [[ $1 ]]; then
    LAST_DAYS=$1
fi

SRC_DIR_PARENT=`cd $SRC_DIR_PARENT && pwd`
SRC_DIR_LEN=`expr length $SRC_DIR_PARENT`

FILES=`find $SRC_DIR_PARENT -mtime -$LAST_DAYS`
# merge all found files
for f in $FILES; do
    if [[ -d $f ]]; then
        echo "ignore dir: $f"
    elif [[ -f $f && -r $f && -s $f ]]; then
        echo "merge file: $f"
        basefile=${f:$SRC_DIR_LEN}
        cmd_str="$SYNC_CMD -m $f -f $DST_DIR_PARENT$basefile"
        if [[ $VERBOSE ]]; then
            echo $cmd_str
            echo
        fi
        eval $cmd_str
    else
        echo "no process file: $f"
    fi
done

echo
