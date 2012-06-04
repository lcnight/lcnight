#! /bin/bash

usage ()
{
    echo -e "Usage: `basename $0` -[m,s]\n"\
    "-m		do mapreduce\n"\
    "-s		do sync\n"\
    "-h		show help\n"
    exit 2;
}

do_sync=0
do_mapr=0
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
    ./sync_mapr.sh -r -m -S ./map-reduce-pig/calc_active.sh >>./logs/mapr_active.log 2>&1
    echo "[`date '+%F %T'`] run basic mapreduce success !"

    # range [x, y)
    ./sync_mapr.sh -m -D -B 30 -E 24 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth4.log 2>&1 &
    ./sync_mapr.sh -m -D -B 24 -E 16 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth3.log 2>&1 &
    ./sync_mapr.sh -m -D -B 16 -E 8 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth2.log 2>&1 &
    ./sync_mapr.sh -m -D -B 8 -E 0 ./map-reduce-pig/calc_other.sh >>./logs/mapr_oth1.log 2>&1 &

    # wait all child processes to complete
    wait
    echo "[`date '+%F %T'`] common mapreduce complete, do slow calculate"

    ./sync_mapr.sh -m -D -B 30 -E 24 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow4.log 2>&1 &
    ./sync_mapr.sh -m -D -B 24 -E 16 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow3.log 2>&1 &
    ./sync_mapr.sh -m -D -B 16 -E 8 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow2.log 2>&1 &
    ./sync_mapr.sh -m -D -B 8 -E 0 ./map-reduce-pig/calc_slow.sh >>./logs/mapr_slow1.log 2>&1 &
    wait
fi

echo "[`date '+%F %T'`] all done"
echo
