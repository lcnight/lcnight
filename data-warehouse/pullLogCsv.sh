#! /bin/bash

SSH_OPTS='-nq -p58000'
SCP_OPTS='-q -P58000'
recip="lc@taomee.com"
# =============================================================================
#                           Local Variables
# =============================================================================
debug=1
noCopy=0
compactMode=0
toDir=~/wizard/wizard-stats/data-warehouse/base-dir/pull-data
#binDir=~/wizard/wizard-stats/data-warehouse/base-dir
baseDir=~/wizard/wizard-stats/data-warehouse/base-dir
cfgFile=etlLog.cfg
#csvCfgFile=etlCsv.cfg

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
        "\n       -c            transfer file in compact format"\
		"\n       -z            move marker to last file without copy any"\
    	"\n       -T <toDir>    destination dir, [$toDir]"\
    	"\n       -B <baseDir>  base dir for config file and work area, [$baseDir]"\
    	"\n       -C <cfgFile>  specify config file, [$cfgFile]"\
    	"\n"
}

while getopts vzchB:C:T: opt 
do
	case $opt in
		v)	debug=1 ;;
		z)  noCopy=1 ;;
		c)  compactMode=1 ;;
		X)  binDir=$OPTARG ;;
		B)	baseDir=$OPTARG ;;
		T)  toDir=$OPTARG ;;
		C)	cfgFile=$OPTARG ;;
		h)  Usage; exit 0 ;;
		\?) Usage; exit 3;;
	esac
done
#shift `expr $OPTIND - 1`

# =============================================================================
#                           Function Definitions
# ============================================================================= 
Info () {
	echo -e "[`date '+%F %T'`] $*" ;
}
Msg () {
	if [ $debug -eq 1 ]; then echo -e $* ; fi
}

ERR_MSG=''
AbortProcess () {
   errSubject="$1"
   errMsg=$ERR_MSG
   if [[ $2 != '' ]]; then
       errMsg=$2
   fi
   # ... will send out email alert 
   echo "mail -s '$errSubject'"
   echo -e "$errMsg" | mail -s "$errSubject" $recip
   exit -1
}

CheckResult ()
{ # 1(code) 2(subject) 3(errMsg)
    if [[ $# < 1 ]];then return 0; fi
    if [[ $1 != 0 ]]; then
        AbortProcess "$2" "$3"
    fi
}

# =============================================================================
#                               Main Process
# ============================================================================= 
Info "start pulling process"
tmpCfgFile="$baseDir/$cfgFile"
cat "$tmpCfgFile" | while read HOST DIR PREFIX SUFFIX; do 
    if [[ $HOST =~ ^# || $HOST == "" ]]; then 
        continue;
    fi 
    TargetPattern=$HOST:$DIR/$PREFIX***$SUFFIX
    Msg "\nProcess start at `date '+%F %T'` for $TargetPattern"

    lastFile=$baseDir/${PREFIX}lastFile.log
    filterToken='-maxdepth 1 -type f'
    if [[ -f $lastFile ]]; then
        filterToken="$filterToken -newer `tail -1 $lastFile`"
    fi

    if [[ $SUFFIX == '.txt' ]]; then
        msgStr="ssh $SSH_OPTS $HOST \"cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort | head -n -1\""
        Msg $msgStr
        fileList=`ssh $SSH_OPTS $HOST "cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort | head -n -1"`
        CheckResult $? "Fetch ERROR: $TargetPattern" "$msgStr"
        if [[ $fileList == "" ]]; then
            Msg "can not find any files $filterToken";
            continue;
        fi
        Msg "pull from $HOST:$DIR, file list:\n$fileList"
        for ff in $fileList; do
            basefile=`basename $ff`
            if [[ $noCopy == 0 ]]; then
                Msg "pull $basefile"
                if [[ $compactMode == 0 ]]; then
                    scp $SCP_OPTS $HOST:$DIR/$basefile $toDir
                else
                    compactFile=${basefile}.tgz
                    ssh $SSH_OPTS $HOST "cd $DIR && tar czf $compactFile $basefile"
                    scp $SCP_OPTS $HOST:$DIR/$compactFile $toDir
                    cd $toDir && tar xvf $compactFile && rm -vf $compactFile
                    ssh $SSH_OPTS $HOST "rm -f $DIR/$compactFile"
                fi
                CheckResult $? "pull ERROR: $HOST:$DIR/$basefile"
            fi
            echo "# pull $ff at `date '+%F %T'`" >> $lastFile
            echo "$basefile" >> $lastFile
        done
    elif [[ $SUFFIX == '.csv' ]]; then
        msgStr="ssh $SSH_OPTS $HOST \"cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort\""
        Msg $msgStr
        fileList=`ssh $SSH_OPTS $HOST "cd $DIR; find . $filterToken -name '$PREFIX*$SUFFIX' | sort"`
        CheckResult $? "Fetch ERROR: $TargetPattern" "$msgStr"
        if [[ $fileList == "" ]]; then
            Msg "can not find any files $filterToken";
            continue;
        fi
        Msg "pull from $HOST:$DIR, file list:\n$fileList"
        for ff in $fileList; do
            basefile=`basename $ff`
            if [[ $noCopy == 0 ]]; then
                Msg "pull $basefile"
                if [[ $compactMode == 0 ]]; then
                    scp $SCP_OPTS $HOST:$DIR/$basefile $toDir
                else
                    compactFile=${basefile}.tgz
                    ssh $SSH_OPTS $HOST "cd $DIR && tar czf $compactFile $basefile"
                    scp $SCP_OPTS $HOST:$DIR/$compactFile $toDir
                    cd $toDir && tar xvf $compactFile && rm -vf $compactFile
                    ssh $SSH_OPTS $HOST "rm -f $DIR/$compactFile"
                fi
                CheckResult $? "pull ERROR: $HOST:$DIR/$basefile"
            fi
            echo "# pull $ff at `date '+%F %T'`" >> $lastFile
            echo "$basefile" >> $lastFile
        done
    else
        msgStr="skip unknown file extention $SUFFIX, only support .txt|.csv"
        Msg $msgStr
        AbortProcess "$msgStr" "config file: $tmpCfgFile\nlast log file: $lastFile\ntarget pattern: $TargetPattern"
    fi
    Msg "Process end at `date '+%F %T'`\n"
done
Info "end pulling"
echo;echo;
