#! /bin/bash
#change current directory to absolute path
cd `cd $(dirname $0) && pwd`

Usage() {
cat << HELP_DOC
running Ads process for specific day/month/quarter

Usage: `basename $0` [options] YYYYMMDD
    -a          process all
    -p          process pages to get PageViews and UniqViews
    -u          process users actions to get registed/new/active players number
    -c          process users cost data to get paid players and cost sum number
    -k          process month keepers to get month keepers, paid players and cost sum
    -d          do day processing
    -m          do month processing
    -q          do quarter processing
    -z          run in dryrun mode
    -v          run in verbose mode
    -h          show usage help and exit

HELP_DOC
exit 0
}

# include common configuration
source comm_conf.sh

verbose=0; dryrun=0;
process_all=0;
proces_pvuv=0; process_user=0; process_cost=0; process_keep=0;
process_day=0; process_month=0; process_quarter=0;
while getopts hvzapuckdmq opt; do
    case $opt in
        a) process_all=1;;
        p) process_pvuv=1;;
        u) process_user=1;;
        c) process_cost=1;;
        k) process_keep=1;;
        d) process_day=1;;
        m) process_month=1;;
        q) process_quarter=1;;
        z) dryrun=1;; v) verbose=1;;
        h) Usage;; ?) Usage;; *) Usage;;
    esac
done
shift $(($OPTIND - 1))
if [[ $1 =~ [^0-9] || ${#1} != 8 ]]; then Usage; fi
pday=$1

if [[ $dryrun == 1 ]];then
    HADOOP_FS="echo hadoop fs $CONF_PARAM "
    HADOOP_JAR="echo hadoop jar $JAR_NAME "
    LOADMYSQL="echo ./load_shell.sh"
fi

# include bash lib functions
source comm_func.sh
if [[ $verbose == 1 || $dryrun == 1 ]]; then
    set -x;
fi

endMonth=`is_month_end $pday`
endQuarter=`is_quarter_end $pday`

OUTDAY_BASE=$OUTPUT_PATH/day
OUTDAY_PATH=$OUTDAY_BASE/$pday
OUTMONTH_BASE=$OUTPUT_PATH/month
OUTMONTH_PATH=$OUTMONTH_BASE/$endMonth
OUTQUARTER_BASE=$OUTPUT_PATH/quarter
OUTQUARTER_PATH=$OUTQUARTER_BASE/$endQuarter

#============================================================================
#                       main process begin
#============================================================================
### page views && unique views
if [[ $process_all == 1 || $process_pvuv == 1 ]]; then
    accesslog_pattern="$ACCESSLOG_PATH/ad*$pday*.log"
    if [[ `dfs_exists "$accesslog_pattern"` != 0 && # check access log exists
        ($process_all == 1 || $process_day == 1) ]]; then
        # ad,url,uuid \t num
        day_pvuv_raw_path=$OUTDAY_PATH/pvuv_raw
        day_pvuv_raw_exists=`dfs_exists $day_pvuv_raw_path`
        if [[ $day_pvuv_raw_exists != 0 ]]; then dfs_purge $day_pvuv_raw_path; fi
        $HADOOP_JAR pages.PvUv $CONF_PARAM $CONF_TADMAP_FILES $accesslog_pattern $day_pvuv_raw_path

        if [[ `dfs_exists $day_pvuv_sum_path` != 0 ]]; then
            # ad,url,[uniq|times] \t num
            day_pvuv_sum_path=$OUTDAY_PATH/pvuv_sum
            if [[ `dfs_exists $day_pvuv_sum_path` != 0 ]]; then dfs_purge $day_pvuv_sum_path; fi
            $HADOOP_JAR core.UniqTimes $CONF_PARAM $day_pvuv_raw_path $day_pvuv_sum_path
            ## load day page views and uniq views
            $LOADMYSQL -t daypv $pday $day_pvuv_sum_path
            $LOADMYSQL -t dayuv $pday $day_pvuv_sum_path
        fi
    fi

    # check if month or quarter end ?
    if [[ $endMonth != 0 && ($process_all == 1 || $process_month == 1)  ]]; then
        month_pvuv_raw_path=$OUTMONTH_PATH/pvuv_raw
        if [[ `dfs_exists $month_pvuv_raw_path` != 0 ]]; then dfs_purge $month_pvuv_raw_path;fi
        $HADOOP_JAR core.SumKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            "$OUTDAY_BASE/$endMonth*/pvuv_raw" $month_pvuv_raw_path

        if [[ `dfs_exists $month_pvuv_raw_path` != 0 ]]; then
            month_pvuv_sum_path=$OUTMONTH_PATH/pvuv_sum
            if [[ `dfs_exists $month_pvuv_sum_path` != 0 ]]; then dfs_purge $month_pvuv_sum_path; fi
            $HADOOP_JAR core.UniqTimes $CONF_PARAM $month_pvuv_raw_path $month_pvuv_sum_path
            ## load month page views and uniq views
            $LOADMYSQL -t monthpv $endMonth $month_pvuv_sum_path
            $LOADMYSQL -t monthuv $endMonth $month_pvuv_sum_path
        fi
    fi

    if [[ $endQuarter != 0 && ($process_all == 1 || $process_quarter == 1) ]]; then
        quarter_pvuv_raw_path=$OUTQUARTER_PATH/pvuv_raw
        if [[ `dfs_exists $quarter_pvuv_raw_path` != 0 ]]; then dfs_purge $quarter_pvuv_raw_path; fi
        $HADOOP_JAR core.SumKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            `get_quarter_paths $endQuarter $OUTMONTH_BASE pvuv_raw` $quarter_pvuv_raw_path

        if [[ `dfs_exists $quarter_pvuv_raw_path` != 0 ]]; then
            quarter_pvuv_sum_path=$OUTQUARTER_PATH/pvuv_sum
            if [[ `dfs_exists $quarter_pvuv_sum_path` != 0 ]]; then dfs_purge $quarter_pvuv_sum_path;fi
            $HADOOP_JAR core.UniqTimes $CONF_PARAM $quarter_pvuv_raw_path $quarter_pvuv_sum_path
            ## load quarter page views and uniq views
            $LOADMYSQL -t quarterpv $endQuarter $quarter_pvuv_sum_path
            $LOADMYSQL -t quarteruv $endQuarter $quarter_pvuv_sum_path
        fi
    fi
fi

### dir for day activers
day_online_path=$OUTDAY_PATH/game_online_mimitad
### dir for month activers
month_online_path=$OUTMONTH_PATH/game_online_mimitad
### dir for quarter activers
quarter_online_path=$OUTQUARTER_PATH/game_online_mimitad

### registers && activers && newers uniq number
if [[ $process_all == 1 || $process_user == 1 ]]; then
    day_register_pattern="$REGISTER_PATH/$pday"
    if [[ `dfs_exists "$day_register_pattern"` != 0 && # check exists
        ($process_all == 1 || $process_day == 1) ]]; then
        Info "day game online register processing ..."
        # ad,gameid \t num
        register_day_path=$OUTDAY_PATH/game_register
        if [[ `dfs_exists $register_day_path` != 0 ]]; then dfs_purge $register_day_path; fi
        $HADOOP_JAR users.UserGameRegister $CONF_PARAM regsummary \
            "$day_register_pattern" $register_day_path

        ## load day's ad game registers
        $LOADMYSQL -t dayreg $pday $register_day_path
    fi

    day_online_pattern="$ONLINE_PATH/$pday"
    if [[ `dfs_exists "$day_online_pattern"` != 0 && # check exists
        ($process_all == 1 || $process_day == 1) ]]; then
        Info "day game online /activer/newer/ processing ..."
        if [[ `dfs_exists $day_online_path` != 0 ]]; then dfs_purge $day_online_path; fi
        $HADOOP_JAR users.UserGameLogin $CONF_PARAM \
            "$day_online_pattern" $day_online_path

        day_online_active_path=$OUTDAY_PATH/game_online_active
        if [[ `dfs_exists $day_online_active_path` != 0 ]]; then dfs_purge $day_online_active_path; fi
        $HADOOP_JAR users.UserGameStat $CONF_PARAM active "$day_online_path" $day_online_active_path
        ## load day's ad game active
        $LOADMYSQL -t dayactive $pday $day_online_active_path

        ## merge all login <game,mimi => tad>
        prev_day=`get_prev_days $pday`
        prevday_allonline_path=$OUTDAY_BASE/$prev_day/all_online_mimitad
        # gameid,mimi \t tad
        day_allonline_path=$OUTDAY_PATH/all_online_mimitad
        if [[ `dfs_exists $day_allonline_path` != 0 ]]; then dfs_purge $day_allonline_path; fi
        $HADOOP_JAR core.UniqKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            $prevday_allonline_path $day_online_path $day_allonline_path

        ## compute day's newers
        # gameid,mimi \t tad(cur) \t ""
        online_day_newjoin_path=$OUTDAY_PATH/game_online_newjoin
        if [[ `dfs_exists $online_day_newjoin_path` != 0 ]]; then dfs_purge $online_day_newjoin_path; fi
        $HADOOP_JAR core.MapJoinDriver $CONF_PARAM  $OUTPUT_REDUCE_COMPRESS leftonly \
            $day_online_path $prevday_allonline_path $online_day_newjoin_path

        if [[ `dfs_exists $online_day_newjoin_path` != 0 ]]; then
            online_day_new_path=$OUTDAY_PATH/game_online_new
            if [[ `dfs_exists $online_day_new_path` != 0 ]]; then dfs_purge $online_day_new_path; fi
            $HADOOP_JAR users.UserGameStat $CONF_PARAM new "$online_day_newjoin_path" $online_day_new_path
            ## load day's ad game new
            $LOADMYSQL -t daynew $pday $online_day_new_path
        fi
    fi

    if [[ $endMonth != 0 && ($process_all == 1 || $process_month == 1)  ]]; then
        ##================================================================================##
        Info 'month register processing ...'
        register_month_path=$OUTMONTH_PATH/game_register
        if [[ `dfs_exists $register_month_path` != 0 ]]; then dfs_purge $register_month_path; fi
        $HADOOP_JAR core.SumKeyValue $CONF_PARAM \
            "$OUTDAY_BASE/$endMonth*/game_register" $register_month_path
        ## load month's ad game registers
        $LOADMYSQL -t monthreg $endMonth $register_month_path

        ##================================================================================##
        Info 'month activer & newer processing ...'
        if [[ `dfs_exists $month_online_path` != 0 ]]; then dfs_purge $month_online_path; fi
        $HADOOP_JAR core.UniqKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            "$OUTDAY_BASE/$endMonth*/game_online_mimitad" $month_online_path

        online_month_active_path=$OUTMONTH_PATH/game_online_active
        if [[ `dfs_exists $online_month_active_path` != 0 ]]; then dfs_purge $online_month_active_path; fi
        $HADOOP_JAR users.UserGameStat $CONF_PARAM active "$month_online_path" $online_month_active_path
        ## load month's ad game active
        $LOADMYSQL -t monthactive $endMonth $online_month_active_path

        prevmonth_lastday=`get_prev_days ${endMonth}01`
        prevmonth_all_path=$OUTDAY_BASE/$prevmonth_lastday/all_online_mimitad
        online_month_newjoin_path=$OUTMONTH_PATH/game_online_newjoin
        if [[ `dfs_exists $online_month_newjoin_path` != 0 ]];then dfs_purge $online_month_newjoin_path; fi
        $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS leftonly \
            $month_online_path $prevmonth_all_path $online_month_newjoin_path

        if [[ `dfs_exists $online_month_newjoin_path` != 0 ]]; then
            online_month_new_path=$OUTMONTH_PATH/game_online_new
            if [[ `dfs_exists $online_month_new_path` != 0 ]]; then dfs_purge $online_month_new_path; fi

            $HADOOP_JAR users.UserGameStat $CONF_PARAM new "$online_month_newjoin_path" $online_month_new_path
            ## load month's ad game new
            $LOADMYSQL -t monthnew $endMonth $online_month_new_path
        fi
    fi

    if [[ $endQuarter != 0 && ($process_all == 1 || $process_quarter == 1) ]]; then
        ##================================================================================##
        Info 'quarter register processing ...'
        register_quarter_path=$OUTQUARTER_PATH/game_register
        if [[ `dfs_exists $register_quarter_path` != 0 ]]; then dfs_purge $register_quarter_path; fi
        $HADOOP_JAR core.SumKeyValue $CONF_PARAM \
            `get_quarter_paths $endQuarter $OUTMONTH_BASE game_register` $register_quarter_path
        ## load quarter's ad game registers
        $LOADMYSQL -t quarterreg $endQuarter $register_quarter_path

        ##================================================================================##
        Info 'quarter activer & newer processing ...'
        if [[ `dfs_exists $quarter_online_path` != 0 ]]; then dfs_purge $quarter_online_path; fi
        $HADOOP_JAR core.UniqKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            `get_quarter_paths $endQuarter $OUTMONTH_BASE game_online_mimitad` $quarter_online_path

        online_quarter_active_path=$OUTQUARTER_PATH/game_online_active
        if [[ `dfs_exists $online_quarter_active_path` != 0 ]]; then dfs_purge $online_quarter_active_path; fi
        $HADOOP_JAR users.UserGameStat $CONF_PARAM active "$quarter_online_path" $online_quarter_active_path
        ## load quarter's ad game active
        $LOADMYSQL -t quarteractive $endQuarter $online_quarter_active_path

        prevquarter_lastday=`get_prev_days ${endQuarter}01`
        prevquarter_all_path=$OUTDAY_BASE/$prevquarter_lastday/all_online_mimitad
        online_quarter_newjoin_path=$OUTQUARTER_PATH/game_online_newjoin
        if [[ `dfs_exists $online_quarter_newjoin_path` != 0 ]]; then dfs_purge $online_quarter_newjoin_path; fi
        $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS leftonly \
            $quarter_online_path $prevquarter_all_path $online_quarter_newjoin_path

        if [[ `dfs_exists $online_quarter_newjoin_path` != 0 ]]; then
            online_quarter_new_path=$OUTQUARTER_PATH/game_online_new
            if [[ `dfs_exists $online_quarter_new_path` != 0 ]]; then dfs_purge $online_quarter_new_path; fi
            $HADOOP_JAR users.UserGameStat $CONF_PARAM new "$online_quarter_newjoin_path" $online_quarter_new_path
            ## load quarter's ad game new
            $LOADMYSQL -t quarternew $endQuarter $online_quarter_new_path
        fi
    fi
fi

### paid users && cost sum
# dir for day's cost
cost_day_path=$OUTDAY_PATH/game_cost
# dir for month's cost
cost_month_path=$OUTMONTH_PATH/game_cost
# dir for quarter's cost
cost_quarter_path=$OUTQUARTER_PATH/game_cost
if [[ $process_all == 1 || $process_cost == 1 ]]; then
    day_mb_pattern="$MB_PATH/$pday";
    day_mb_e=`dfs_exists "$day_mb_pattern"`
    day_vip_pattern="$VIP_PATH/$pday"
    day_vip_e=`dfs_exists "$day_vip_pattern"`
    if [[ ( $day_mb_e != 0 || $day_vip_e != 0 ) # check exists
        && ($process_all == 1 || $process_day == 1) ]]; then
        Info 'day cost processing ...'
        # gameid,mimi \t cost
        if [[ `dfs_exists $cost_day_path` != 0 ]]; then dfs_purge $cost_day_path; fi
        $HADOOP_JAR users.UserGamePaid $CONF_PARAM $CONF_PRDMAP_FILES gamepaid \
            "$day_mb_pattern" "$day_vip_pattern" $cost_day_path

        # gameid,mimi \t cost \t tad
        cost_join_day_path=$OUTDAY_PATH/game_costjoin
        if [[ `dfs_exists $cost_join_day_path` != 0 ]]; then dfs_purge $cost_join_day_path; fi
        $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS left \
            $cost_day_path $day_online_path $cost_join_day_path

        # ad,gameid \t num
        cost_uniqsum_day_path=$OUTDAY_PATH/game_costuniqsum
        if [[ `dfs_exists $cost_uniqsum_day_path` != 0 ]]; then dfs_purge $cost_uniqsum_day_path; fi
        $HADOOP_JAR users.UserGamePaid $CONF_PARAM uniqcost \
            $cost_join_day_path $cost_uniqsum_day_path

        $LOADMYSQL -t daypaidnum $pday $cost_uniqsum_day_path
        $LOADMYSQL -t daycostsum $pday $cost_uniqsum_day_path
    fi

    if [[ $endMonth != 0 && ($process_all == 1 || $process_month == 1) ]]; then
        Info 'month cost processing ...'
        if [[ `dfs_exists $cost_month_path` != 0 ]]; then dfs_purge $cost_month_path; fi
        $HADOOP_JAR core.SumKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            "$OUTDAY_BASE/$endMonth*/game_cost" $cost_month_path

        cost_join_month_path=$OUTMONTH_PATH/game_costjoin
        if [[ `dfs_exists $cost_join_month_path` != 0 ]]; then dfs_purge $cost_join_month_path; fi
        $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS left \
            $cost_month_path $month_online_path $cost_join_month_path

        cost_uniqsum_month_path=$OUTMONTH_PATH/game_costuniqsum
        if [[ `dfs_exists $cost_uniqsum_month_path` != 0 ]]; then dfs_purge $cost_uniqsum_month_path; fi
        $HADOOP_JAR users.UserGamePaid $CONF_PARAM uniqcost \
            $cost_join_month_path $cost_uniqsum_month_path

        $LOADMYSQL -t monthpaidnum $endMonth $cost_uniqsum_month_path
        $LOADMYSQL -t monthcostsum $endMonth $cost_uniqsum_month_path
    fi

    if [[ $endQuarter != 0 && ($process_all == 1 || $process_quarter == 1) ]]; then
        Info 'quarter cost processing ...'
        if [[ `dfs_exists $cost_quarter_path` != 0 ]]; then dfs_purge $cost_quarter_path; fi
        $HADOOP_JAR core.SumKeyValue $CONF_PARAM $OUTPUT_REDUCE_COMPRESS \
            `get_quarter_paths $endQuarter $OUTMONTH_BASE game_cost` $cost_quarter_path

        cost_join_quarter_path=$OUTQUARTER_PATH/game_costjoin
        if [[ `dfs_exists $cost_join_quarter_path` != 0 ]]; then dfs_purge $cost_join_quarter_path; fi
        $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS left \
            $cost_quarter_path $quarter_online_path $cost_join_quarter_path

        cost_uniqsum_quarter_path=$OUTQUARTER_PATH/game_costuniqsum
        if [[ `dfs_exists $cost_uniqsum_quarter_path` != 0 ]]; then dfs_purge $cost_uniqsum_quarter_path; fi
        $HADOOP_JAR users.UserGamePaid $CONF_PARAM uniqcost \
            $cost_join_quarter_path $cost_uniqsum_quarter_path
        $LOADMYSQL -t quarterpaidnum $endQuarter $cost_uniqsum_quarter_path
        $LOADMYSQL -t quartercostsum $endQuarter $cost_uniqsum_quarter_path
    fi
fi

### compute month newers keeping in the following month
### month keeping use account login log, not game online login data
if [[ $process_all == 1 || $process_keep == 1 ]]; then
    Info 'month keep processing ...'
    if [[ $endMonth != 0 && ($process_all == 1 || $process_month == 1)  ]]; then
        # based on month new registers
        
        #if [[ $process_all == 1 || $process_user == 1 ]]; then
            # mimi \t gameid
            month_accactive_game=$OUTMONTH_PATH/account_mimigame
            dfs_purge $month_accactive_game
            $HADOOP_JAR users.UserLoginStat $CONF_PARAM $OUTPUT_REDUCE_COMPRESS logingame \
                "$LOGIN_PATH/$endMonth*" $month_accactive_game

            # mimi \t tad(curmonth)
            month_newregister_path=$OUTMONTH_PATH/account_newregister
            dfs_purge $month_newregister_path
            $HADOOP_JAR users.UserGameRegister $CONF_PARAM $OUTPUT_REDUCE_COMPRESS regmimitad \
                "$REGISTER_PATH/$endMonth*" $month_newregister_path

            # mimi registertad gameid
            month_registerlogin_path=$OUTMONTH_PATH/account_registerlogin
            dfs_purge $month_registerlogin_path
            $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS inner \
                $month_newregister_path $month_accactive_game $month_registerlogin_path

            # gameid,mimi tad
            month_gamemimi_tad=$OUTMONTH_PATH/account_gamemimi_tad
            dfs_purge $month_gamemimi_tad
            $HADOOP_JAR users.UserLoginStat $CONF_PARAM $OUTPUT_REDUCE_COMPRESS logingameregtad \
                $month_registerlogin_path $month_gamemimi_tad

            # ad,gameid num
            month_reglogin_keep_path=$OUTMONTH_PATH/account_registerlogin_keep
            dfs_purge $month_reglogin_keep_path
            $HADOOP_JAR users.UserGameStat $CONF_PARAM active \
                "$month_gamemimi_tad" $month_reglogin_keep_path
            $LOADMYSQL -t monthkeep "$endMonth,$endMonth" $month_reglogin_keep_path
        #fi

        # gameid,mimi \t cost \t tad
        month_reglog_cost_path=$OUTMONTH_PATH/account_registerlogin_costjoin
        #if [[ $process_all == 1 || $process_cost == 1 ]]; then
            dfs_purge $month_reglog_cost_path
            $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS inner \
                $cost_month_path $month_gamemimi_tad $month_reglog_cost_path
            # ad,gameid \t num
            month_reglog_costuniqsum_path=$OUTMONTH_PATH/account_registerlogin_costuniqsum
            dfs_purge $month_reglog_costuniqsum_path
            $HADOOP_JAR users.UserGamePaid $CONF_PARAM uniqcost \
                $month_reglog_cost_path $month_reglog_costuniqsum_path
            # load month activers' cost sum (using account login tad)
            $LOADMYSQL -t monthkeeppaidnum "$endMonth,$endMonth" $month_reglog_costuniqsum_path
            $LOADMYSQL -t monthkeepcostsum "$endMonth,$endMonth" $month_reglog_costuniqsum_path
        #fi

        ## compute 12 months keepers
        curMonth=$endMonth
        for ((i=1; i<13 ; ++i)); do
            prevMonth=`get_prev_months $curMonth $i`
            prevmonth_register_path=$OUTMONTH_BASE/$prevMonth/account_newregister
            Info "processing $prevMonth keeped in $curMonth ..."
            if [[ `dfs_exists $prevmonth_register_path` != 0 ]]; then
                baseTargetTok=$prevMonth-$curMonth
                # mimi registertad gameid
                month_prevreg_keep_path=$OUTMONTH_PATH/account_prevreg_keep_$baseTargetTok
                dfs_purge $month_prevreg_keep_path
                $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS inner \
                    $prevmonth_register_path $month_accactive_game $month_prevreg_keep_path

                # gameid,mimi tad
                month_prevreg_keep_gamemimi_tad=$OUTMONTH_PATH/account_prevreg_keep_gamemimitad_$baseTargetTok
                dfs_purge $month_prevreg_keep_gamemimi_tad
                $HADOOP_JAR users.UserLoginStat $CONF_PARAM $OUTPUT_REDUCE_COMPRESS logingameregtad \
                    $month_prevreg_keep_path $month_prevreg_keep_gamemimi_tad

                #if [[ $process_all == 1 || $process_user == 1 ]]; then
                    # ad,gameid \t num
                    month_prereg_keep_path=$OUTMONTH_PATH/account_prevreg_keep_$baseTargetTok
                    dfs_purge $month_prereg_keep_path
                    $HADOOP_JAR users.UserGameStat $CONF_PARAM active \
                        $month_prevreg_keep_gamemimi_tad $month_prereg_keep_path
                    $LOADMYSQL -t monthkeep "$prevMonth,$curMonth" $month_prereg_keep_path
                #fi

                # gameid,mimi \t cost \t tad(prev)
                month_prevreg_keep_cost_joinpath=$OUTMONTH_PATH/account_prevreg_keep_costjoin_$baseTargetTok
                #if [[ $process_all == 1 || $process_cost == 1 ]]; then
                    dfs_purge $month_prevreg_keep_cost_joinpath
                    $HADOOP_JAR core.MapJoinDriver $CONF_PARAM $OUTPUT_REDUCE_COMPRESS inner \
                        $cost_month_path $month_prevreg_keep_gamemimi_tad $month_prevreg_keep_cost_joinpath

                    # ad,game,[uniq|cost] \t num
                    month_prevreg_keep_cost_path=$OUTMONTH_PATH/account_prevreg_keep_cost_$baseTargetTok
                    dfs_purge $month_prevreg_keep_cost_path
                    $HADOOP_JAR users.UserGamePaid $CONF_PARAM uniqcost \
                        $month_prevreg_keep_cost_joinpath $month_prevreg_keep_cost_path
                    $LOADMYSQL -t monthkeeppaidnum "$prevMonth,$curMonth" $month_prevreg_keep_cost_path
                    $LOADMYSQL -t monthkeepcostsum "$prevMonth,$curMonth" $month_prevreg_keep_cost_path
                #fi
            else break; fi
        done
    fi
fi

