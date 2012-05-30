#! /bin/bash

# ENV_FILE="$HOME/wireless/env"
# if [[ -f $ENV_FILE ]]; then echo "ddadd"; source $ENV_FILE; fi

source env_file
source comm_func.sh
#################################### Variables ##########################################
VERBOSE=1
LAST_DAYS=1
SYNC_CMD='hadoop hdp_push'
UNIQ_CMD='hadoop jar GetUniqClause.jar GetUniqClause'
#MAPR_CMD='bash calc.sh '

MAPR_CMD='bash map-reduce-pig/calc.sh '
WEEKMONTH_CMD='bash map-reduce-pig/calc_week_month.sh '
# database connection option
DB_OPTION='hadoop:HA#2jsOw%x@192.168.71.45/db_wireless_dim'
# local source dir
SRC_DIR_PARENT=/opt/stat-log/data/1000000
# hadoop destination dir
DST_DIR_PARENT=/wireless/1000000
if [[ ! -d $SRC_DIR_PARENT ]]; then
    echo "Not Exist Source Dir: $SRC_DIR_PARENT";
    echo;
fi

# tmp dir used for intermediate result
HADOOP_TMP='/wireless/tmp'
doEnv=0
doMapr=0
MapRangeBeg=30
MapRangeEnd=0
doweekmonth=0
#################################### Functions ##########################################
Usage() {
    echo -e "Usage: `basename $0` [opt]\n"\
    "    opt can be as following:\n"\
    "     -t        sync latest [$LAST_DAYS] days change from Local to Hadoop\n"\
    "     -s        local source data dir [$SRC_DIR_PARENT]\n"\
    "     -d        hadoop destination data dir [$DST_DIR_PARENT]\n"\
    "     -b        database connection option [$DB_OPTION]\n"\
	"     -e        do build environment (merge files/build unique phrase), default [$doEnv]\n"\
	"     -m        do map reduce day's data related, default [$doMapr]\n"\
	"     -B        do mapreduce from [$MapRangeBeg] days ago\n"\
    "     -E        do mapreduce end at [$MapRangeEnd] days ago\n"\
	"     -w        do week and month aggregate [$doweekmonth]\n"\
    "     -v        run in verbose mode\n"\
    "     -q        run in quiet mode\n"\
    "     -h        show usage\n"
    exit 2;
}

while getopts hvqt:s:d:b:B:E:emw opt
do
    case $opt in
        v) VERBOSE=1;;
        q) VERBOSE=0;;
        t) LAST_DAYS=$OPTARG;;
        s) SRC_DIR_PARENT=$OPTARG;;
        d) DST_DIR_PARENT=$OPTARG;;
        b) DB_OPTION=$OPTARG;;
        B) MapRangeBeg=$OPTARG;;
        E) MapRangeEnd=$OPTARG;;
		e) doEnv=1;;
		m) doMapr=1;;
		w) doweekmonth=1;;
        h) Usage ;;   
        ?) Usage ;;
    esac
done
# shift $(($OPTIND -1))
# if [[ $* ]]; then fi


SRC_DIR_PARENT=`cd $SRC_DIR_PARENT && pwd`
SRC_DIR_LEN=`expr length $SRC_DIR_PARENT`
TIME_TOKEN="-mtime -$LAST_DAYS"
############################################################################################
#################################### Main Process ##########################################
############################################################################################
echo
Info 'environment test'
Info "CLASSPATH: $CLASSPATH"
Info "HADOOP_CLASSPATH: $HADOOP_CLASSPATH"
if [[ $doEnv == 1 ]]; then
    # merge all found files
    Info 'start merge process'
    comma_files=''

	batchNum=100
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
				eval $sync_cmd_str
				CheckResult $? 'ERROR: Sync file to hadoop' "running command:\n$sync_cmd_str"
				batchSrc=''
				batchDst=''
				curNum=0;
				exit
			fi
        else
            Info "ignore file: $f"
        fi
    done
	Info 'end files merge'

	exit

    # get uniq prhase from changed files and store into db
    Info 'start unique phrase'
    comma_files=${comma_files/%,/}

	if [[ $comma_files == '' ]]; then
		Info 'no change files'
		exit
	fi
    uniq_cmdstr="$UNIQ_CMD '$comma_files' '$HADOOP_TMP/phrase/`date '+%Y%m%d'`-$$-$RANDOM' -b '$DB_OPTION'"
    Debug $uniq_cmdstr
    eval $uniq_cmdstr
    # CheckResult $? 'ERROR: prepare mapr env' "running command:\n$uniq_cmdstr"; 
    Info 'end build environment'

#   if [[ $onlyEnv == 1 ]];then
#       Info 'configure to ***only*** run build environment'
#   fi
fi

rangeBeg=`date -d "$MapRangeBeg days ago" '+%Y%m%d'`
rangeEnd=`date -d "$MapRangeEnd days ago" '+%Y%m%d'`
# do map reduce process for modified days
if [[ $doMapr == 1 ]]; then
    Info 'start mapred process'
    # DAYS=`find $SRC_DIR_PARENT -maxdepth 1 $TIME_TOKEN | sort`
    DAYS=`find $SRC_DIR_PARENT $TIME_TOKEN | sed -n '/\/\(.*\)\/.*/s//\1/p' | sort | uniq`
	Debug "Need Process: $DAYS"
    for d in $DAYS; do
        day_time=`get_day_time $d`
        if [[ $day_time == "" ]]; then
            continue
		elif [[ $day_time -lt $rangeBeg || $day_time -gt $rangeEnd || $day_time == $rangeEnd ]]; then
			Info "$day_time not in range [$rangeBeg, $rangeEnd)"
			continue
		else
            # do real mapred task
            job_cmd_str="$MAPR_CMD $day_time"
            Debug $job_cmd_str
			eval $job_cmd_str
            CheckResult $? "ERROR: run mapr job" "running command:\n$job_cmd_str"
        fi
    done

#   if [[ $onlyMapr == 1 ]];then
#       Info 'configure to ***only*** run MapRed'
#   fi
fi

startDay=20120326
cutDay=`date -d '30 days ago' '+%Y%m%d'`
stopPoint=$(($startDay<$cutDay?$cutDay:$startDay))
curDay=`date '+%Y%m%d'`
modDaysStart=$curDay
modDaysEnd=$curDay
if [[ $doweekmonth == 1 ]]; then
    DAYS=`find $SRC_DIR_PARENT $TIME_TOKEN | sed -n '/\/\(.*\)\/.*/s//\1/p' | sort | uniq`
    for d in $DAYS; do
        day_time=`get_day_time $d`
        if [[ $day_time == "" ]]; then
            continue
		elif [[ $day_time -lt $stopPoint || $day_time -gt $curDay ]]; then
			Info "$day_time not in range [$stopPoint, $curDay]"
			continue
		else
			if [[ $day_time -lt $modDaysStart ]]; then modDaysStart=$day_time; fi
			if [[ $day_time -gt $modDaysEnd ]]; then modDaysEnd=$day_time; fi
		fi
    done

	# tune: run weeks & months operation
	weekmon_cmdstr="$WEEKMONTH_CMD $modDaysStart $modDaysEnd"
	Debug "Week Month processing ..."
	Debug "$weekmon_cmdstr"
	eval $weekmon_cmdstr
fi


Info "all done"
echo
