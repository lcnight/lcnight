#! /bin/bash

usage ()
{
	echo -e "Usage: `basename $0` -[w,m,s]\n"\
	"-w		do week month mapreduce\n"\
	"-m		do mapreduce\n"\
	"-s		do sync\n"\
	"-h		show help\n"
	exit 2;
}

do_sync=0
do_mapr=0
# do_week=0
while getopts smwh opt; do
	case $opt in 
		s) do_sync=1;;
		m) do_mapr=1;;
#		w) do_week=1;;
		h) usage;;
		*) usage;;
	esac
done

echo "[`date '+%F %T'`] processing start "

if [[ $do_sync == 1 ]]; then
	./sync_mapr.sh -e >> ./logs/sync.log 2>&1
	echo "[`date '+%F %T'`] file sync complete "
fi

####./sync_mapr.sh -m -B 30 -E 21 >>./logs/mapr30.log 2>&1 &
####./sync_mapr.sh -m -B 21 -E 14 >>./logs/mapr21.log 2>&1 &
####./sync_mapr.sh -m -B 14 -E 7 >>./logs/mapr14.log 2>&1 &
if [[ $do_mapr == 1 ]]; then
    ./sync_mapr.sh -m -S ./map-reduce-pig/calc_active.sh >>./logs/mapr_active.log 2>&1
	echo "[`date '+%F %T'`] run basic mapreduce success !"

	# range [x, y)
    ./sync_mapr.sh -m -D -B 30 -E 20 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth30.log 2>&1 &
    ./sync_mapr.sh -m -D -B 20 -E 10 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth20.log 2>&1 &
    ./sync_mapr.sh -m -D -B 10 -E 0 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth10.log 2>&1 &

	# wait all child processes to complete
	wait
	echo "[`date '+%F %T'`] common mapreduce complete, do slow calculate"

    ./sync_mapr.sh -m -D -B 30 -E 20 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow30.log 2>&1 &
    ./sync_mapr.sh -m -D -B 20 -E 10 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow20.log 2>&1 &
    ./sync_mapr.sh -m -D -B 10 -E 0 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow10.log 2>&1 &
	wait
fi

##if [[ $do_week == 1 ]]; then
##	./sync_mapr.sh -w >>./logs/mapr_weekmonth.log 2>&1 
##	echo "[`date '+%F %T'`] week month mapreduce complete "
##fi

echo "[`date '+%F %T'`] all done"
echo
