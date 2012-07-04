#! /bin/bash

# all game users' activers/newers/keepers/backers
# compute distribution for {region/paid/channel}

Current_Dir=`dirname $0`
Current_Dir=`cd $Current_Dir && pwd`

#PATH="/home/lc/hadoop/hadoop-1.0.1/bin:$PATH"
#CLASSPATH=":./*:/home/lc/lib/java/*:$CLASSPATH"
source comm_func.sh

Usage() {
    echo "Usage: `basename $0` [-zvh] YYYYMM"
    exit 1;
}

dryRun=0
while getopts zvh opt; do
    case $opt in
        z) dryRun=1;;
        v) DEBUG=1; VERBOSE=1;;
        h) Usage;;
        *) Usage;;
    esac
done
shift $(($OPTIND - 1))

if [[ $VERBOSE == 1 || $dryRun == 1 ]]; then
    set -x
fi

PaidDir=/home/lc/hadoop/testdata/boss/mb
LoginDir=/home/lc/hadoop/testdata/account/login
RegisterDir=/home/lc/hadoop/testdata/account/register
OutputDir=/home/lc/hadoop/testdata/output
ConfParams='-conf hadoop-lc.xml'

#PaidDir=/user/lc/boss/mb
#LoginDir=/user/lc/account/login
#RegisterDir=/user/lc/account/register
#OutputDir=/user/lc/userstats
#ConfParams='-conf hadoop-cluster.xml'

MYSQL_RESULT='mysql://root:ta0mee@10.1.1.60/db_userstats'
if [[ $dryRun == 0 ]]; then
    HADOOP_FS='hadoop fs '
    HADOOP_SHELL='hadoop jar UserStat.jar '
    LOAD_SHELL="hadoop LoadDistrDriver $ConfParams $MYSQL_RESULT"
else
    HADOOP_FS='echo hadoop fs '
    HADOOP_SHELL='echo hadoop jar UserStat.jar '
    LOAD_SHELL="echo hadoop LoadDistrDriver $ConfParams $MYSQL_RESULT"
fi

############ hadoop operational functions ############
dfs_purgedir() { # dirpath confparams
    dirpath=$1
    if [[ $# -lt 2 ]]; then
        confParams=''
    else
        confParams=$2
    fi
    $HADOOP_FS $confParams -rmr $dirpath
}
dfs_exists() { # path confparams
    path=$1
    if [[ $# -lt 2 ]]; then
        confParams=''
    else
        confParams=$2
    fi
    if [[ $dryRun == 0 ]]; then
        $HADOOP_FS $confParams -test -e $path; echo $?;
    else
        echo 0;
    fi
}
############ hadoop operational functions ############
pMonth=$1
if [[ ${#pMonth} != 6 || $1 =~ [^0-9] ]]; then
    Usage;
fi
ALLOutputBase=$OutputDir/all
MonthOutputBase=$OutputDir/month

############### month input ###############
parentMonth=`get_next_months $pMonth`
grandMonth=`get_next_months $parentMonth`
monthInputPaidDir=$PaidDir/$pMonth
InputPaidDirExists=`dfs_exists $monthInputPaidDir "$ConfParams"`
monthInputLoginDir=$LoginDir/$pMonth
InputLoginDirExists=`dfs_exists $monthInputLoginDir "$ConfParams"`
monthInputRegisterDir=$RegisterDir/$pMonth
InputRegisterDirExists=`dfs_exists $monthInputRegisterDir "$ConfParams"`

if [[ $dryRun == 0 &&
    ($InputPaidDirExists != '0' || $InputLoginDirExists != '0' || $InputRegisterDirExists != '0') ]] ; then
    Info "$monthInputPaidDir not exists $InputPaidDirExists"
    Info "$monthInputLoginDir not exists $InputLoginDirExists"
    Info "$monthInputRegisterDir not exists $InputRegisterDirExists"
    exit 0
fi


allOutputDir=$ALLOutputBase/$pMonth
monthOutputDir=$MonthOutputBase/$pMonth

# ---------- build basic data output ----------

monthPaidOutputDir=$monthOutputDir/paid
dfs_purgedir $monthPaidOutputDir "$ConfParams"
$HADOOP_SHELL UserPaidOrNot $ConfParams $monthInputPaidDir $monthPaidOutputDir

monthRegisterOutputDir=$monthOutputDir/registers
dfs_purgedir $monthRegisterOutputDir "$ConfParams"
$HADOOP_SHELL UserRegister $ConfParams $monthInputRegisterDir $monthRegisterOutputDir

allRegisterOutputDir=$allOutputDir/registers
allParentOutputRegisterDir=$ALLOutputBase/$parentMonth/registers
dfs_purgedir $allRegisterOutputDir "$ConfParams"
$HADOOP_SHELL UniqKeyValue $ConfParams $monthRegisterOutputDir $allParentOutputRegisterDir $allRegisterOutputDir

monthLoginOutputDir=$monthOutputDir/login
dfs_purgedir $monthLoginOutputDir "$ConfParams"
$HADOOP_SHELL UserLoginSuite $ConfParams $monthInputLoginDir $monthLoginOutputDir

monthActiveJoinTblOutputDir=$monthOutputDir/activersJoinTblGRPC
dfs_purgedir $monthActiveJoinTblOutputDir "$ConfParams"
$HADOOP_SHELL MapMultiJoinDriver $ConfParams \
    left $monthLoginOutputDir/basic-* $monthPaidOutputDir $allRegisterOutputDir \
    $monthActiveJoinTblOutputDir

##################  core activers game  ###############
monthActiveOutputDir=$monthOutputDir/activersGame
dfs_purgedir $monthActiveOutputDir "$ConfParams"
$HADOOP_SHELL UserGameList $ConfParams $monthActiveJoinTblOutputDir $monthActiveOutputDir

allActiveOutputDir=$allOutputDir/activersGame
allParentOutputActiveDir=$ALLOutputBase/$parentMonth/activersGame
dfs_purgedir $allActiveOutputDir "$ConfParams"
$HADOOP_SHELL UniqKeyValue $ConfParams $monthActiveOutputDir $allParentOutputActiveDir $allActiveOutputDir

monthActiveGameJoinTblOutputDir=$monthOutputDir/activersGameJoinTbl
dfs_purgedir $monthActiveGameJoinTblOutputDir "$ConfParams"
$HADOOP_SHELL ActiversGameJoinTbl $ConfParams \
    $monthActiveJoinTblOutputDir $monthActiveGameJoinTblOutputDir
##################  core activers game  ###############

# ---------- end of basic data output ----------


# ---------- month activers
############### activers distribution
monthActiversDistrOutputDir=$monthOutputDir/activersDistr
dfs_purgedir $monthActiversDistrOutputDir "$ConfParams"
$HADOOP_SHELL ActiversGameDistr $ConfParams \
    $monthActiveGameJoinTblOutputDir $monthActiversDistrOutputDir
$LOAD_SHELL 'insert t_game_info(time,gameid,regionid,paid,channelid,<activers>)' $pMonth $monthActiversDistrOutputDir

monthActiversOnlineDistrOutputDir=$monthOutputDir/activersOnlineDistr
dfs_purgedir $monthActiversOnlineDistrOutputDir "$ConfParams"
$HADOOP_SHELL ActiversGameOnlineDistr $ConfParams $monthLoginOutputDir/gameDayAvgDuration* $monthActiversOnlineDistrOutputDir
$LOAD_SHELL 'replace t_game_online(time,type,gameid,property,startpoint,value)' "$pMonth,'activer'" $monthActiversOnlineDistrOutputDir
# ---------- end month activers


# ---------- month newers
############### check privous all activers exists ? ###################
allParentExists=`dfs_exists $allParentOutputActiveDir "$ConfParams"`
monthNewersOutputDir=$monthOutputDir/newers
monthNewersJoinOutputDir=$monthOutputDir/newers_JoinTbl
monthNewersDistrOutputDir=$monthOutputDir/newersDistr
dfs_purgedir $monthNewersOutputDir "$ConfParams"
if [[ $allParentExists != '0' ]]; then  # exists
    $HADOOP_SHELL MapJoinDriver $ConfParams inner $monthActiveOutputDir $monthActiveOutputDir $monthNewersOutputDir
else
    $HADOOP_SHELL MapJoinDriver $ConfParams leftonly $monthActiveOutputDir $allParentOutputActiveDir $monthNewersOutputDir
fi
############### newers unique number distribution
dfs_purgedir $monthNewersJoinOutputDir "$ConfParams"
$HADOOP_SHELL MapJoinDriver $ConfParams left $monthNewersOutputDir $monthActiveGameJoinTblOutputDir $monthNewersJoinOutputDir
dfs_purgedir $monthNewersDistrOutputDir "$ConfParams"
$HADOOP_SHELL ActiversGameDistr $ConfParams \
    -D distr.skip.index=2 $monthNewersJoinOutputDir $monthNewersDistrOutputDir
$LOAD_SHELL 'insert t_game_info(time,gameid,regionid,paid,channelid,<newers>)' $pMonth $monthNewersDistrOutputDir

################ newers online distribution
monthNewersOnlineJoinOutputDir=$monthOutputDir/newersOnline_JoinTbl
monthNewersOnlineDistrOutputDir=$monthOutputDir/newersOnlineDistr
dfs_purgedir $monthNewersOnlineJoinOutputDir "$ConfParams"
$HADOOP_SHELL ReduceJoinDriver $ConfParams \
    inner $monthNewersOutputDir $monthLoginOutputDir/gameDayAvgDuration* $monthNewersOnlineJoinOutputDir
dfs_purgedir $monthNewersOnlineDistrOutputDir "$ConfParams"
$HADOOP_SHELL ActiversGameOnlineDistr $ConfParams \
    -D distr.online.skip.index=2 $monthNewersOnlineJoinOutputDir $monthNewersOnlineDistrOutputDir
$LOAD_SHELL 'replace t_game_online(time,type,gameid,property,startpoint,value)' "$pMonth,'newer'" $monthNewersOnlineDistrOutputDir
# ---------- end month newers


# ---------- month keepers
################### check privous month activers exists ? ###################
monthPrevActiveOutputDir=$MonthOutputBase/$parentMonth/activersGame
monthPrevActiveExists=`dfs_exists $monthPrevActiveOutputDir "$ConfParams"`
monthKeepersOuputDir=$monthOutputDir/keepers
monthKeepersJoinOutputDir=$monthOutputDir/keepers_JoinTbl
monthKeepersDistrOutputDir=$monthOutputDir/keepersDistr
# compute only when prev month activers exists
if [[ $monthPrevActiveExists == '0' ]]; then
    dfs_purgedir $monthKeepersOuputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams inner $monthActiveOutputDir $monthPrevActiveOutputDir $monthKeepersOuputDir

    ############### keepers unique number distr
    dfs_purgedir $monthKeepersJoinOutputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams \
        left $monthKeepersOuputDir $monthActiveGameJoinTblOutputDir $monthKeepersJoinOutputDir
    dfs_purgedir $monthKeepersDistrOutputDir "$ConfParams"
    $HADOOP_SHELL ActiversGameDistr $ConfParams \
        -D distr.skip.index=2 $monthKeepersJoinOutputDir $monthKeepersDistrOutputDir
    $LOAD_SHELL 'insert t_game_info(time,gameid,regionid,paid,channelid,<keepers>)' $pMonth $monthKeepersDistrOutputDir

    ################ keepers online distr
    monthKeepersOnlineJoinOutputDir=$monthOutputDir/keepersOnline_JoinTbl
    monthKeepersOnlineDistrOutputDir=$monthOutputDir/keepersOnlineDistr
    dfs_purgedir $monthKeepersOnlineJoinOutputDir "$ConfParams"
    $HADOOP_SHELL ReduceJoinDriver $ConfParams \
        inner $monthKeepersOuputDir $monthLoginOutputDir/gameDayAvgDuration* $monthKeepersOnlineJoinOutputDir
    dfs_purgedir $monthKeepersOnlineDistrOutputDir "$ConfParams"
    $HADOOP_SHELL ActiversGameOnlineDistr $ConfParams \
        -D distr.online.skip.index=2 $monthKeepersOnlineJoinOutputDir $monthKeepersOnlineDistrOutputDir
    $LOAD_SHELL 'replace t_game_online(time,type,gameid,property,startpoint,value)' \
        "$pMonth,'keeper'" $monthKeepersOnlineDistrOutputDir
fi
# ---------- end month keepers


# ---------- month losters
monthLostersOuputDir=$monthOutputDir/losters
# compute only when prev month activers exists
if [[ $monthPrevActiveExists == '0' ]]; then
    dfs_purgedir $monthLostersOuputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams rightonly $monthActiveOutputDir $monthPrevActiveOutputDir $monthLostersOuputDir
fi
# ---------- end month losters


# ---------- month backers 曾经登录过,上月未活跃,本月活跃
monthBackersBaseOutputDir=$monthOutputDir/backersBase
monthBackersOutputDir=$monthOutputDir/backers
monthBackersJoinOutputDir=$monthOutputDir/backers_JoinTbl
monthBackersDistrOutputDir=$monthOutputDir/backersDistr
# compute only when prev month activers exists
if [[ $monthPrevActiveExists == '0' ]]; then
    dfs_purgedir $monthBackersBaseOutputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams leftonly $monthActiveOutputDir $monthPrevActiveOutputDir $monthBackersBaseOutputDir
    dfs_purgedir $monthBackersOutputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams inner $monthBackersBaseOutputDir $allParentOutputActiveDir $monthBackersOutputDir

    ############### backers unique number distribution
    dfs_purgedir $monthBackersJoinOutputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams \
        left $monthBackersOutputDir $monthActiveGameJoinTblOutputDir $monthBackersJoinOutputDir
    dfs_purgedir $monthBackersDistrOutputDir "$ConfParams"
    $HADOOP_SHELL ActiversGameDistr $ConfParams \
        -D distr.skip.index=3 $monthBackersJoinOutputDir $monthBackersDistrOutputDir
    $LOAD_SHELL 'insert t_game_info(time,gameid,regionid,paid,channelid,<backers>)' $pMonth $monthBackersDistrOutputDir

    ################ backers online distr
    monthBackersOnlineJoinOutputDir=$monthOutputDir/backersOnline_JoinTbl
    monthBackersOnlineDistrOutputDir=$monthOutputDir/backersOnlineDistr
    dfs_purgedir $monthBackersOnlineJoinOutputDir "$ConfParams"
    $HADOOP_SHELL ReduceJoinDriver $ConfParams \
        inner $monthBackersOutputDir $monthLoginOutputDir/gameDayAvgDuration* $monthBackersOnlineJoinOutputDir
    dfs_purgedir $monthBackersOnlineDistrOutputDir "$ConfParams"
    $HADOOP_SHELL ActiversGameOnlineDistr $ConfParams \
        -D distr.online.skip.index=3 $monthBackersOnlineJoinOutputDir $monthBackersOnlineDistrOutputDir
    $LOAD_SHELL 'replace t_game_online(time,type,gameid,property,startpoint,value)' \
        "$pMonth,'backer'" $monthBackersOnlineDistrOutputDir
fi
# ---------- end month backers


# ---------- 2 months lost backers 上上月活跃，上月未活跃，本月活跃
################### check privous month losters exists ? ###################
monthPrevLostersOutputDir=$MonthOutputBase/$parentMonth/losters
monthPrevLostersExists=`dfs_exists $monthPrevLostersOutputDir "$ConfParams"`
monthLostBackersOutputDir=$monthOutputDir/lostbackers
monthLostBackersJoinOutputDir=$monthOutputDir/lostbackers_JoinTbl
monthLostBackersDistrOutputDir=$monthOutputDir/lostbackersDistr
# compute only when prev month activers exists
if [[ $monthPrevLostersExists == '0' ]]; then
    dfs_purgedir $monthLostBackersOutputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams inner $monthActiveOutputDir $monthPrevLostersOutputDir $monthLostBackersOutputDir

    ############### lostBackers unique number distribution
    dfs_purgedir $monthLostBackersJoinOutputDir "$ConfParams"
    $HADOOP_SHELL MapJoinDriver $ConfParams \
        left $monthLostBackersOutputDir $monthActiveGameJoinTblOutputDir $monthLostBackersJoinOutputDir
    dfs_purgedir $monthLostBackersDistrOutputDir "$ConfParams"
    $HADOOP_SHELL ActiversGameDistr $ConfParams \
        -D distr.skip.index=3 $monthLostBackersJoinOutputDir $monthLostBackersDistrOutputDir
    $LOAD_SHELL 'insert t_game_info(time,gameid,regionid,paid,channelid,<lostbackers>)' \
        $pMonth $monthLostBackersDistrOutputDir

    ################ lostBackers online distr
    monthLostBackersOnlineJoinOutputDir=$monthOutputDir/lostbackersOnline_JoinTbl
    monthLostBackersOnlineDistrOutputDir=$monthOutputDir/lostbackersOnlineDistr
    dfs_purgedir $monthLostBackersOnlineJoinOutputDir "$ConfParams"
    $HADOOP_SHELL ReduceJoinDriver $ConfParams \
        inner $monthLostBackersOutputDir $monthLoginOutputDir/gameDayAvgDuration* $monthLostBackersOnlineJoinOutputDir
    dfs_purgedir $monthLostBackersOnlineDistrOutputDir "$ConfParams"
    $HADOOP_SHELL ActiversGameOnlineDistr $ConfParams \
        -D distr.online.skip.index=3 $monthLostBackersOnlineJoinOutputDir $monthLostBackersOnlineDistrOutputDir
    $LOAD_SHELL 'replace t_game_online(time,type,gameid,property,startpoint,value)' \
        "$pMonth,'lostbacker'" $monthLostBackersOnlineDistrOutputDir
fi
