#! /bin/bash
# change current directory to absolute path
cd `cd $(dirname $0) && pwd`

export HADOOP_INSTALL=~/hadoop
export PATH=$HADOOP_INSTALL/bin:$PATH
export CLASSPATH=./*:$CLASSPATH

source comm_func.sh
Usage () {
    cat <<HELP_USAGE
sync configured prefix-xxxx files to hadoop

Usage: `basename $0` 
HELP_USAGE
}

ZKSERVER=localhost:2181/lock
SRC_DIR=/home/lc/var/test
DST_DIR=/user/lc/tmp
CONF_PREFIX=./conf/
CONF_FILE=$CONF_PREFIX/prelist.conf
SYNCSHELL="./synctool.sh"
while read prefix suffix; do
    if [[ $prefix =~ ^# || $prefix == "" ]]; then continue; fi
    preconfile=$CONF_PREFIX/$prefix-last.log

    Info; Info "processing file: $prefix***$suffix"
    if [[ -f $preconfile ]]; then
        lastfile=`tail -n 1 $preconfile`
        newertoken="-newer $lastfile"
    else
        newertoken=''
    fi
    RAWList=`find $SRC_DIR $newertoken -name $prefix*$suffix | grep -v '^\.+$' | sort `
    if [[ $RAWList == '' ]]; then
        Info "cannot find files to process for $prefix**$suffix"; continue;
    fi

    BATList=`echo $RAWList | tr ' ' ','`
    $SYNCSHELL -v -s $BATList -d $DST_DIR
    CheckResult $? 'merge/sync batch files error' "raw files: $RAWList"
    lastfile=`echo $RAWList |  tr ' ' '\n' | tail -n 1`
    echo `Info '# batch push files'` >> $preconfile
    echo $lastfile >> $preconfile
done <$CONF_FILE
