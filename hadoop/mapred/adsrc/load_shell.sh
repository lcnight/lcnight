#/usr/bin/bash
#change current directory to absolute path
cd `cd $(dirname $0) && pwd`

Usage () {
cat <<USAGE_INFO
utility tool used to load hadoop file line info into mysql
should be only usedby internal purpuse, unless you know what you to do

Usage: `basename $0` [options] -t type <addkey> filePattern
    -t          load hadoop result type, such as: daypv, dayuv ...
    -z          run in dryrun mode
    -v          run in verbose mode
    -h          show usage help and exit

example:
    # load day page views info into mysql
    `basename $0` -t daypv 20120808 /path/to/hadoop/file

USAGE_INFO
exit
}

ptype=''; verbose=0; dryrun=0;
while getopts hvzt: opt; do
    case $opt in
        t) ptype=$OPTARG;;
        z) dryrun=1;;
        v) verbose=1;;
        h) Usage;;
        ?) Usage;;
        *) Usage;;
    esac
done
shift $(($OPTIND - 1))
if [[ $ptype == '' ]]; then Usage; fi
if [[ $dryrun == 1 ]]; then DryOPT='-z'; fi
if [[ $verbose == 1 ]]; then set -x ; fi

source comm_conf.sh
source comm_func.sh

case $ptype in
    ### page views
    daypv)
    if [[ $# < 2 ]] ; then Usage; fi
    $HADOOP_JAR pages.LoadPv $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_pvuv_day(time,adid,urlid,<pv>)' $1 $2 $DryOPT
    ;;
    monthpv)
    if [[ $# < 2 ]] ; then Usage; fi
    $HADOOP_JAR pages.LoadPv $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_pvuv_month(time,adid,urlid,<pv>)' $1 $2 $DryOPT
    ;;
    quarterpv)
    if [[ $# < 2 ]] ; then Usage; fi
    $HADOOP_JAR pages.LoadPv $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_pvuv_quarter(time,adid,urlid,<pv>)' $1 $2 $DryOPT
    ;;
    ### unique views
    dayuv)
    if [[ $# < 2 ]] ; then Usage; fi
    $HADOOP_JAR pages.LoadUv $CONF_PARAM $RESULT_MYSQLURI \
    'insert into t_info_pvuv_day(time,adid,urlid,<uv>)' $1 $2 $DryOPT
    ;;
    monthuv)
    if [[ $# < 2 ]] ; then Usage; fi
    $HADOOP_JAR pages.LoadUv $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_pvuv_month(time,adid,urlid,<uv>)' $1 $2 $DryOPT
    ;;
    quarteruv)
    if [[ $# < 2 ]] ; then Usage; fi
    $HADOOP_JAR pages.LoadUv $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_pvuv_quarter(time,adid,urlid,<uv>)' $1 $2 $DryOPT
    ;;
    ### registers
    dayreg)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_day(time,adid,gameid,<new_register>)' $1 $2 $DryOPT
    ;;
    monthreg)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_month(time,adid,gameid,<new_register>)' $1 $2 $DryOPT
    ;;
    quarterreg)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_quarter(time,adid,gameid,<new_register>)' $1 $2 $DryOPT
    ;;
    ### activers
    dayactive)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_day(time,adid,gameid,<active_users>)' $1 $2 $DryOPT
    ;;
    monthactive)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_month(time,adid,gameid,<active_users>)' $1 $2 $DryOPT
    ;;
    quarteractive)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_quarter(time,adid,gameid,<active_users>)' $1 $2 $DryOPT
    ;;
    ### newers
    daynew)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_day(time,adid,gameid,<new_users>)' $1 $2 $DryOPT
    ;;
    monthnew)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_month(time,adid,gameid,<new_users>)' $1 $2 $DryOPT
    ;;
    quarternew)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_quarter(time,adid,gameid,<new_users>)' $1 $2 $DryOPT
    ;;
    ### paid uniq number
    daypaidnum)
    $HADOOP_JAR users.LoadPaidNum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_day(time,adid,gameid,<paid_users>)' $1 $2 $DryOPT
    ;;
    monthpaidnum)
    $HADOOP_JAR users.LoadPaidNum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_month(time,adid,gameid,<paid_users>)' $1 $2 $DryOPT
    ;;
    quarterpaidnum)
    $HADOOP_JAR users.LoadPaidNum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_quarter(time,adid,gameid,<paid_users>)' $1 $2 $DryOPT
    ;;
    ### cost sum
    daycostsum)
    $HADOOP_JAR users.LoadCostSum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_day(time,adid,gameid,<paid_costsum>)' $1 $2 $DryOPT
    ;;
    monthcostsum)
    $HADOOP_JAR users.LoadCostSum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_month(time,adid,gameid,<paid_costsum>)' $1 $2 $DryOPT
    ;;
    quartercostsum)
    $HADOOP_JAR users.LoadCostSum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_user_quarter(time,adid,gameid,<paid_costsum>)' $1 $2 $DryOPT
    ;;
    ### keeping keepers/paid unique num/cost sum
    monthkeep)
    $HADOOP_JAR users.LoadUsers $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_keep_month(basemonth,targetmonth,adid,gameid,<keepers>)' $1 $2 $DryOPT
    ;;
    monthkeeppaidnum)
    $HADOOP_JAR users.LoadPaidNum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_keep_month(basemonth,targetmonth,adid,gameid,<paid_users>)' $1 $2 $DryOPT
    ;;
    monthkeepcostsum)
    $HADOOP_JAR users.LoadCostSum $CONF_PARAM $RESULT_MYSQLURI \
        'insert into t_info_keep_month(basemonth,targetmonth,adid,gameid,<paid_costsum>)' $1 $2 $DryOPT
    ;;
    *)
    Info "unsupported process type $ptype"
    ;;
esac

