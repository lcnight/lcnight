#! /bin/bash

source lib/comm_func
Usage () {
    echo -e \
    "\nload Csv files, Alarm Logs and aggregate corresponding fields.\n"\
    	"\nUsage: `basename $0` [opt] "\
    	"\n      where [opt] can be any of the following:"\
    	"\n       -h            help"\
    	"\n       -v            run in verbose debug mode"\
    	"\n       -q            run in quiet mode"\
        "\n       -d            only load Character Dim csv files, always load"\
        "\n       -c            only load csv files, besides loading Dim"\
        "\n       -l            only load alarm log files, besides loading Dim"\
        "\n       -a            only do aggregate processing, besides loading Dim"\
    	"\n"
}

debug=1
onlyDim=0
onlyCsv=0
onlyAlarm=0
onlyAgg=0
while getopts hvqdcla opt 
do
	case $opt in
		v)	debug=1 ;;
		q)	debug=0 ;;
		d)  onlyDim=1 ;;
		c)  onlyCsv=1 ;;
		l)  onlyAlarm=1 ;;
		a)  onlyAgg=1 ;;
		h)  Usage; exit 0 ;;
		\?) Usage; exit 3;;
	esac
done

############################## Global Variables ####################################
# processing log dir
LOG_DIR=~/wizard/wizard-stats/data-warehouse/base-dir/log
# dir name for csv files
CSV_DIR=loadcsv
# dir name for alarm_log files
ALARM_DIR=loadlog
# dir name for aggregate script
AGG_DIR=aggupdate
############################## Main Process #####################################
# loading character dim info firstly
Msg "start loading CharacterDim"
./$CSV_DIR/load_CharacterDim.php  2>&1 >> $LOG_DIR/characterDim.log
Msg "loading Character Dim End"
if [[ $onlyDim == 1 ]]; then
    exit 0;
fi

# aggregate Character Attributes
./$AGG_DIR/agg_CharacterOnline.php 2>&1 >> $LOG_DIR/agg_characterOnline.log &
./$AGG_DIR/agg_CharacterQuests.php 2>&1 >> $LOG_DIR/agg_characterQuests.log &
wait && Msg "aggregating Character Dim end"
if [[ $onlyAgg == 1 ]]; then
    exit 0;
fi

if [[ $onlyCsv == 1 && $onlyAlarm == 1 ]]; then
    Msg "option error: OnlyCSV and OnlyAlarm cannot both be set to true";
    exit 0;
fi

if [[ ($onlyCsv == 0 && $onlyAlarm == 0) || $onlyCsv == 1 ]];then
    # load csv files into data warehouse
    ./$CSV_DIR/load_CharacterLevelDuration.php 2>&1 >> $LOG_DIR/csv_characterLevel.log  &
    ./$CSV_DIR/load_CharacterOnlineDuration.php 2>&1 >> $LOG_DIR/csv_characterOnline.log  &
    ./$CSV_DIR/load_CombatAction.php 2>&1 >> $LOG_DIR/csv_combatAction.log  &
    ./$CSV_DIR/load_CombatDuration.php 2>&1 >> $LOG_DIR/csv_combatDuration.log  &
    ./$CSV_DIR/load_MalformedMsg.php 2>&1 >> $LOG_DIR/csv_malformedMsg.log  &
    ./$CSV_DIR/load_PetEquip.php 2>&1 >> $LOG_DIR/csv_petEquip.log  &
    ./$CSV_DIR/load_PetMinigame.php 2>&1 >> $LOG_DIR/csv_petMinigame.log  &
    ./$CSV_DIR/load_PlayerGoldTransfer.php 2>&1 >> $LOG_DIR/csv_playerGlod.log  &
    ./$CSV_DIR/load_PlayerInventoryAddition.php 2>&1 >> $LOG_DIR/csv_playerInvAdd.log  &
    ./$CSV_DIR/load_PlayerInventoryRemoval.php 2>&1 >> $LOG_DIR/csv_playerInvRem.log  &
    ./$CSV_DIR/load_QuestCompletion.php 2>&1 >> $LOG_DIR/csv_questCompletion.log  &
    ./$CSV_DIR/load_SpellUsage.php 2>&1 >> $LOG_DIR/csv_spellUsage.log  &
    wait && Msg "loading CSV files End"
    if [[ $onlyCsv == 1 ]]; then
        Msg 'configure to ***only*** Loading CSV files'
        exit 0;
    fi
fi

if [[ ($onlyCsv == 0 && $onlyAlarm == 0) || $onlyAlarm == 1 ]];then
    # load Alarm Log files into data warehouse
    ./$ALARM_DIR/load_Item.php 2>&1 >> $LOG_DIR/alarm_item.log  &
    ./$ALARM_DIR/load_LoginInfo.php 2>&1 >> $LOG_DIR/alarm_login.log  &
    ./$ALARM_DIR/load_PetInfo.php 2>&1 >> $LOG_DIR/alarm_pet.log  &
    ./$ALARM_DIR/load_Service.php 2>&1 >> $LOG_DIR/alarm_service.log  &
    wait && Msg "loading Alarm Log files End"
    if [[ $onlyAlarm == 1 ]]; then
        Msg 'configure to ***only*** Loading Alarm Log files'
        exit 0;
    fi
fi
Msg "all done"
