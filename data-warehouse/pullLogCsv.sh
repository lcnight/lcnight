#! /bin/bash

SSH_OPTS=-nq
# =============================================================================
#                           Local Variables
# =============================================================================
debug=0
noCopy=0
binDir=~/wizard/etl-base/
baseDir=~/wizard/etl-base/
cfgFile=etlLog.cfg
#csvCfgFile=etlCsv.cfg
toDir=~/wizard/etl-base/pull-data
recip="lc@taomee.com"

# =============================================================================
#                               Parse Options
# =============================================================================
Usage () {
    echo -e \
    	"\nExtract Alarm Logs or Csv send them to concerned parties.\n"\
    	"\nUsage: `basename $0` [opt] "\
    	"\n      where [opt] can be any of the following:"\
    	"\n       -h            help"\
    	"\n       -v            run in verbose debug mode"\
		"\n       -z            move marker to last file without copy any"\
    	"\n       -T <toDir>    destination dir, [$toDir]"\
		"\n       -X <binDir>   bin dir for running exec scripts, [$binDir]"\
    	"\n       -B <baseDir>  base dir for config file and work area, [$baseDir]"\
    	"\n       -C <cfgFile>  specify config file, [$cfgFile]"\
    	"\n"
}

while getopts vzhB:X:C:L:T: opt 
do
	case $opt in
		v)	debug=1 ;;
		z)  noCopy=1 ;;
		X)  binDir=$OPTARG ;;
		B)	baseDir=$OPTARG ;;
		T)  toDir=$OPTARG ;;
		C)	logCfgFile=$OPTARG ;;
		h)  Usage; exit 0 ;;
		\?) Usage; exit 3;;
	esac
done
#shift `expr $OPTIND - 1`

# =============================================================================
#                           Function Definitions
# ============================================================================= 
Msg () {
	if [ $debug -eq 1 ]; then echo -e $* ; fi
}

ERR_MSG=''
AbortProcess () {
   errMsg="ETL ERROR: $*"
   echo $errMsg
   # ... will send out email alert 
   echo "mail -s '$errMsg'"
   echo $ERR_MSG | mail -s "$errMsg" $recip
   exit -1
}

# =============================================================================
#                               Main Process
# ============================================================================= 

tmpCfgFile="$baseDir/$cfgFile"
cat "$tmpCfgFile" | while read HOST DIR PREFIX SUFFIX; do 
    if [[ $HOST =~ ^# || $HOST == "" ]]; then 
        continue;
    fi 
    Msg "\nProcess start at `date '+%F %T'` for $HOST:$DIR/$PREFIX***$SUFFIX"

    lastFile=$baseDir/${PREFIX}lastFile.log
    filterToken='-maxdepth 1'
    if [[ -f $lastFile ]]; then
        filterToken="-maxdepth 1 -newer `tail -1 $lastFile`"
    fi

    if [[ $SUFFIX == '.txt' ]]; then
        Msg "ssh $SSH_OPTS $HOST \"cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort | head -n -1\""
        fileList=`ssh $SSH_OPTS $HOST "cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort | head -n -1"`
        if [[ $fileList == "" ]]; then
            Msg "can not find any files $filterToken";
            continue;
        fi
        Msg "pull from $HOST:$DIR, file list:\n$fileList"
        for ff in $fileList; do
            basefile=`basename $ff`
            if [[ $noCopy == 0 ]]; then
                Msg "pull $basefile"
                scp -q $HOST:$DIR/$basefile $toDir
            fi
            echo "# pull $ff at `date '+%F %T'`" >> $lastFile
            echo "$basefile" >> $lastFile
        done
    elif [[ $SUFFIX == '.csv' ]]; then
        Msg "ssh $SSH_OPTS $HOST \"cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort\""
        fileList=`ssh $SSH_OPTS $HOST "cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort"`
        if [[ $fileList == "" ]]; then
            Msg "can not find any files $filterToken";
            continue;
        fi
        Msg "pull from $HOST:$DIR, file list:\n$fileList"
        for ff in $fileList; do
            basefile=`basename $ff`
            if [[ $noCopy == 0 ]]; then
                Msg "pull $basefile"
                scp -q $HOST:$DIR/$basefile $toDir
            fi
            echo "# pull $ff at `date '+%F %T'`" >> $lastFile
            echo "$basefile" >> $lastFile
        done
    else
        Msg "skip unknown file extention $SUFFIX, only support .txt|.csv"
    fi
    Msg "Process end at `date '+%F %T'`\n"
done
