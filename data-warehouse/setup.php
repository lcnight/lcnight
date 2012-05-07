<?php 
if (!defined(TZ)) define(TZ, 'Asia/Shanghai');
if (!defined(TZ_OFF)) define(TZ_OFF, '+8 hours');
///  system initilize
if (!date_default_timezone_set(TZ)) {
    exit('Error date_default_timezone_set with '.TZ);
}
///  end initilize


if (!defined(BaseDir)) define(BaseDir, dirname(__FILE__));
if (!defined(LibDir)) define(LibDir, BaseDir.'/lib');
if (!defined(HOST_DB1)) define(HOST_DB1, 'root:ta0mee@10.1.1.60/db_wizard_dw');
if (!defined(HOST_DB2)) define(HOST_DB2, 'root:ta0mee@10.1.1.44/db_wizard_dw');
if (!defined(PullDir)) define(PullDir, '/home/lc/wizard/wizard-stats/data-warehouse/base-dir/pull-data');
if (!defined(BakDir)) define(BakDir, '/home/lc/wizard/wizard-stats/data-warehouse/base-dir/bak-dir');
if (!defined(LogLineSep)) define(LogLineSep, "\r\n");

if (!defined(CSV_LINE_MAX)) define(CSV_LINE_MAX, 2*1024);
$ETL_CSV_MAP=array( // csv-token => (csv-prefix, host-db /*, table-name */)
    'CharacterDim' => array('character_dim-', HOST_DB1,  't_CharacterDim'),
    'CharacterDayOnline' => array('', HOST_DB1,  't_CharacterDayOnline'),
    'CharacterOnlineDuration' => array('characterOnline_time-', HOST_DB1, 't_CharacterOnlineDuration'), 
    'CharacterLevelDuration' => array('characterleveling-', HOST_DB1, 't_CharacterLevelDuration'), 
    'MalformedMsg' => array('MalformedMsg-', HOST_DB1, 't_MalformedMsg'),
    'QuestCompletion' => array('questsCompletion-', HOST_DB1, ''), 
    'PlayerGoldTransfer' => array('playGoldTransfer-', HOST_DB1, ''), 
    'PlayerInventoryAddition' => array('PlayInventoryAddition-', HOST_DB1, ''), 
    'PlayerInventoryRemoval' => array('PlayInventoryRemoval-', HOST_DB1, ''), 
    'PetEquip' => array('PetEquip-', HOST_DB1, ''),
    'PetMinigame' => array('PetMinigame-', HOST_DB1, ''),
    'SpellUsage' => array('SpellUsage-', HOST_DB1, ''),
    'CombatAction' => array('combatAction-', HOST_DB1, ''),
    'CombatDuration' => array('combatDuration-', HOST_DB1, '')
    );

$ETL_LOG_MAP=array(// log-token => (log-prefix, table-name)
    'ItemLog' => array('ItemLog_', HOST_DB1, 't_Item'),
    'LoginLog' => array('LoginLog_', HOST_DB1, 't_Access'),
    'PetInfo' => array('PetLog_', HOST_DB1, 't_Pet'),
    'Service' => array('ServiceLog_', HOST_DB1, 't_Service')
    );


/************************************************************************* 
 *                      configuration function
 ************************************************************************/ 
function get_etlconf($token, &$prefix, &$db_url, &$tbl = 'NULL') {
    global $ETL_CSV_MAP, $ETL_LOG_MAP;

    if (isset($ETL_CSV_MAP[$token])) {
        $arr = $ETL_CSV_MAP[$token];
        $prefix = $arr[0];
        $db_url = $arr[1];
        $tbl = $arr[2];
    } 
    else if (isset($ETL_LOG_MAP[$token])) {
        $arr = $ETL_LOG_MAP[$token];
        $prefix = $arr[0];
        $db_url = $arr[1];
        $tbl = $arr[2];
    } 
    else {
        $prefix = false;
        $db_url = false; 
        $tbl = false; 
    }
    return false;
}

require_once(LibDir.'/mysql_db.php');
require_once(LibDir.'/util.php');
?>
