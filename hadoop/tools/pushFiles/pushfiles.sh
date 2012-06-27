#! /bin/bash
Usage ()
{
    echo "`basename $0` [-zvh]";
    exit 0;
}

Current_Dir=`dirname $0`
Current_Dir=`cd $Current_Dir && pwd`
echo $Current_Dir
cd $Current_Dir

#PATH="/home/lc/hadoop/hadoop-1.0.1/bin:$PATH"
#CLASSPATH=":./*:/home/lc/lib/java/*:$CLASSPATH"
Conf_Params="-conf conf/hadoop-lc.xml"
HADOOP_SHELL="hadoop fs $Conf_Params -put "
source ./lib/comm_func.sh

dryRun=0
while getopts zvh opt; do
    case $opt in
        z) dryRun=1;;
        v) DEBUG=1; VERBOSE=1;;
        h) Usage;;
        *) Usage;;
    esac
done

cat ./conf/sync.cfg | while read token srcdir dstdir; do

if [[ $token =~ ^#.* ]]; then
    continue;
fi
pointFile=./conf/$token.log
cutPoint=`tail -1 $pointFile`
if [[ $cutPoint == '' ]]; then
    cutPoint=19700101
fi

fileslist=`cd $srcdir; find . -maxdepth 1 | sort`
for ff in $fileslist; do
    fullPath=$srcdir/$ff
    if [[ ! -f $fullPath || ! -s $fullPath ]]; then
        Info "ignore : $ff"
        continue;
    else
        dayStr=`get_day_time $ff`
        if [[ ${#dayStr} == 0  ]]; then
            Info "ignore : $ff"
            continue;
        fi
        if [[ $dayStr < $cutPoint ||  $dayStr == $cutPoint ]]; then
            continue;
        fi

        Info "process : $ff"
        monthStr=`get_month_str $dayStr`
        fullDstPath=$dstdir/$monthStr/$dayStr
        if [[ $dryRun == 1 ]]; then
            echo "$HADOOP_SHELL $fullPath $fullDstPath"
        else
            $HADOOP_SHELL $fullPath $fullDstPath
        fi
        if [[ $? != 0 ]]; then
            Info "Error when: $fullPath => $fullDstPath";
            break;
        fi
		if [[ $dryRun == 0 ]]; then
			echo "# $fullPath => $dstdir/$monthStr/$dayStr" >> $pointFile
			echo $dayStr >> $pointFile
		fi
    fi
done

done
