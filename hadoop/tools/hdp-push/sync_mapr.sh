#! /bin/bash

# ENV_FILE="$HOME/wireless/env"
# if [[ -f $ENV_FILE ]]; then echo "ddadd"; source $ENV_FILE; fi

source env_file
source comm_func.sh
#################################### Variables ##########################################
VERBOSE=1
LAST_DAYS=1
# MAPR_CMD='bash map-reduce-pig/calc.sh '
# WEEKMONTH_CMD='bash map-reduce-pig/calc_week_month.sh '

# database connection option
DB_OPTION='hadoop:HA#2jsOw%x@192.168.71.45/db_wireless_dim'
# local source dir
SRC_DIR_PARENT=/opt/taomee/hadoop/stat-log/data/1000000
# hadoop destination dir
DST_DIR_PARENT=/wireless/1000000
# tmp dir used for intermediate result
HADOOP_TMP='/wireless/tmp'
if [[ ! -d $SRC_DIR_PARENT ]]; then
    echo "Not Exist Source Dir: $SRC_DIR_PARENT";
    echo;
fi

dryrun=0
doEnv=0
doMapr=0
MapRangeBeg=30
MapRangeEnd=0
modeSeq=0
modeDiv=1
reverseRunMode=0

#################################### Functions ##########################################
Usage() {
    echo -e "Usage: `basename $0` [opt] <running script>\n"\
    "opt can be as following:\n"\
    "     -t        sync latest [$LAST_DAYS] days change from Local to Hadoop\n"\
    "     -s        local source data dir [$SRC_DIR_PARENT]\n"\
    "     -d        hadoop destination data dir [$DST_DIR_PARENT]\n"\
    "     -b        database connection option [$DB_OPTION]\n"\
    "     -e        do build environment (merge files/build unique phrase), default [$doEnv]\n"\
    "     -m        do map reduce day's data related, default [$doMapr]\n"\
    "     -S        do map in Sequnce Mode [$modeSeq], not compatiable with Divided Mode\n"\
    "     -D        do map in Divided Mode [$modeDiv], not compatiable with Sequnce Mode\n"\
    "     -B        do Divided Mode mapreduce from [$MapRangeBeg] days ago\n"\
    "     -E        do Divided Mode mapreduce end at [$MapRangeEnd] days ago\n"\
    "     -r        run in time reverse mode\n"\
    "     -v        run in verbose mode\n"\
    "     -q        run in quiet mode\n"\
    "     -z        dryrun mode [$dryrun]\n"\
    "     -h        show usage\n"
    exit 2;
}

while getopts hvemzqt:s:d:b:SDB:E:r opt
do
    case $opt in
        v) VERBOSE=1;;
        q) VERBOSE=0;;
	t) LAST_DAYS=$OPTARG;;
	s) SRC_DIR_PARENT=$OPTARG;;
	d) DST_DIR_PARENT=$OPTARG;;
	b) DB_OPTION=$OPTARG;;
	e) doEnv=1;;
	m) doMapr=1;;
	S) modeSeq=1; modeDiv=0;;
	D) modeDiv=1; modeSeq=0;;
	B) MapRangeBeg=$OPTARG;;
	E) MapRangeEnd=$OPTARG;;
	r) reverseRunMode=1;;
	z) dryrun=1;;
	h) Usage ;;   
	?) Usage ;;
    esac
done
if [[ $doMapr == 1 && ! ${!OPTIND} ]]; then
    Info 'Must supply one running script'"\n"
    Usage;
fi
runningShell="bash ${!OPTIND} "

SYNC_CMD='hadoop hdp_push'
UNIQ_CMD='hadoop jar GetUniqClause.jar GetUniqClause'

SRC_DIR_PARENT=`cd $SRC_DIR_PARENT && pwd`
SRC_DIR_LEN=`expr length $SRC_DIR_PARENT`
TIME_TOKEN="-mtime -$LAST_DAYS"
############################################################################################
#################################### Main Process ##########################################
############################################################################################
echo
Info '#################################### environment ####################################'
Info "CLASSPATH: $CLASSPATH"
Info "HADOOP_CLASSPATH: $HADOOP_CLASSPATH"
Info '#################################### environment ####################################'
if [[ $doEnv == 1 ]]; then
    # merge all found files
    Info 'start merge process'
    comma_files=''

    batchNum=300
    batchSrc=''
    batchDst=''
    curNum=0
    FILES=`find $SRC_DIR_PARENT $TIME_TOKEN`
    for f in $FILES; do
	if [[ -d $f ]]; then
	    Info "ignore dir: $f"
	elif [[ -f $f && -r $f && -s $f ]]; then
	    Info "find: $f"
	    basefile=${f:$SRC_DIR_LEN}
	    hdp_file=$DST_DIR_PARENT$basefile

	    comma_files+="$hdp_file,"
	    # sync_cmd_str="$SYNC_CMD -m $f -f $hdp_file"

	    batchSrc+="$f,"
	    batchDst+="$hdp_file,"
	    curNum=$(($curNum+1))
	    if [[ $curNum -lt $batchNum ]]; then
		continue;
	    else
		batchSrc=${batchSrc/%,/}
		batchDst=${batchDst/%,/}
		sync_cmd_str="$SYNC_CMD -m $batchSrc -f $batchDst"
		Debug $sync_cmd_str; 
		if [[ $dryrun == 0 ]]; then
		    eval $sync_cmd_str
		    CheckResult $? 'ERROR: Sync file to hadoop' "running command:\n$sync_cmd_str"
		fi
		batchSrc=''
		batchDst=''
		curNum=0;
	    fi
	else
	    Info "ignore file: $f"
	fi
    done
    if [[ $curNum -gt 0 ]]; then
	batchSrc=${batchSrc/%,/}
	batchDst=${batchDst/%,/}
	sync_cmd_str="$SYNC_CMD -m $batchSrc -f $batchDst"
	Debug $sync_cmd_str; 
	if [[ $dryrun == 0 ]]; then
	    eval $sync_cmd_str
	fi
    fi
    Info 'end merge files'

    # get uniq prhase from changed files and store into db
    Info 'start unique phrase'
    comma_files=${comma_files/%,/}
    if [[ $comma_files == '' ]]; then
	Info 'no change files'
	exit
    fi
    uniq_cmdstr="$UNIQ_CMD '$comma_files' '$HADOOP_TMP/phrase/`date '+%Y%m%d'`-$$-$RANDOM' -b '$DB_OPTION'"
    Debug $uniq_cmdstr
    if [[ $dryrun == 0 ]]; then
	eval $uniq_cmdstr
	CheckResult $? 'ERROR: prepare mapr env' "running command:\n$uniq_cmdstr"; 
    fi
    Info 'End build environment'
fi

# do map reduce process for modified days
if [[ $doMapr == 1 ]]; then
    if [[ $modeSeq == 1 && $modeDiv == 1 ]]; then
	Usage;
    fi

    Info 'start mapred process'
    DAYS=`find $SRC_DIR_PARENT $TIME_TOKEN | sed -n '/\/\(.*\)\/.*/s//\1/p' | sort | uniq`
    Debug "Need Process: `echo $DAYS | tr '\n' ' '`"

    cutAgo=15
    cutDay=`date -d "$cutAgo days ago" '+%Y%m%d'`
    if [[ $modeDiv == 1 ]] ; then
	if [[ $MapRangeBeg -lt $MapRangeEnd ]]; then
	    Info "Divided Mode: RangeBeg must larger than RangeEnd"
	    Usage;
	fi
	runPoint=$(($MapRangeBeg<$cutAgo?$MapRangeBeg:$cutAgo))
	rangeBeg=`date -d "$runPoint days ago" '+%Y%m%d'`
	rangeEnd=`date -d "$MapRangeEnd days ago" '+%Y%m%d'`
    elif [[ $modeSeq == 1 ]]; then
	startDay=20120326
	stopPoint=$(($startDay<$cutDay?$cutDay:$startDay))
	curDay=`date '+%Y%m%d'`

	rangeBeg=$stopPoint
	rangeEnd=$curDay
    else
	Info "Must run map reduce in [Seq|Div] mode";
	Usage;
    fi

    runningDayslist=''
    for d in $DAYS; do
	day_time=`get_day_time $d`
	if [[ $day_time == "" ]]; then
	    continue
	elif [[ $day_time -lt $rangeBeg || $day_time -gt $rangeEnd || $day_time == $rangeEnd ]]; then
	    Info "$day_time not in range [$rangeBeg, $rangeEnd)"
	    continue
	else
	    if [[ $reverseRunMode == 0 ]]; then
		runningDayslist="$runningDayslist $day_time"
	    else
		runningDayslist="$day_time $runningDayslist"
	    fi
	fi
    done

    for day in $runningDayslist; do 
	job_cmd_str="$runningShell $day"
	Debug $job_cmd_str
	if [[ $dryrun == 0 ]]; then
	    # do real mapred task
	    eval $job_cmd_str
	    CheckResult $? "ERROR: run mapr job" "running command:\n$job_cmd_str"
	fi
    done
fi
Info "all done"

####startDay=20120326
####cutDay=`date -d '30 days ago' '+%Y%m%d'`
####stopPoint=$(($startDay<$cutDay?$cutDay:$startDay))
####curDay=`date '+%Y%m%d'`
####modDaysStart=$curDay
####modDaysEnd=$curDay
####if [[ $doweekmonth == 1 ]]; then
####    DAYS=`find $SRC_DIR_PARENT $TIME_TOKEN | sed -n '/\/\(.*\)\/.*/s//\1/p' | sort | uniq`
####    for d in $DAYS; do
####        day_time=`get_day_time $d`
####        if [[ $day_time == "" ]]; then
####            continue
####		elif [[ $day_time -lt $stopPoint || $day_time -gt $curDay ]]; then
####			Info "$day_time not in range [$stopPoint, $curDay]"
####			continue
####		else
####			if [[ $day_time -lt $modDaysStart ]]; then modDaysStart=$day_time; fi
####			if [[ $day_time -gt $modDaysEnd ]]; then modDaysEnd=$day_time; fi
####		fi
####    done
####
####	# tune: run weeks & months operation
####	weekmon_cmdstr="$WEEKMONTH_CMD $modDaysStart $modDaysEnd"
####	Debug "Week Month processing ..."
####	Debug "$weekmon_cmdstr"
####	eval $weekmon_cmdstr
####fi
