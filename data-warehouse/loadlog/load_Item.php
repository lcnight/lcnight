#! /usr/bin/php -f
<?php
require_once('setup.php');

define('Partition', false);
define('ProcessToken', 'ItemLog');
define('TblToken', '${XXXXXX}');
define('SYSTOKEN', 'ITMTRN');
/* 5.6.1.1  Item Purchase, Sold, Delete, Use and Expiration Transaction Logs
 * <TransactionType>:<VendorID>:<CharacterID>:<ItemDebugName>:<TemplateID>:<CurrencyType>:<currencyValue>:<Cause>
 * 5.6.1.2  Item Loot Transaction Logs
 * Loot:<ZoneID>:<CharacterID>:<ItemDebugName>:<TemplateID>:<CurrencyType>:<CurrencyValue>:<Cause>
 * 5.6.1.4  Item Craft Transaction Logs
 * Craft:<StationID>:<CharacterID>:<ItemDebugName>:<TemplateID>:<CurrencyType>:<CurrencyValue>:<Cause>
 *
 * 5.6.1.3  Item Gift and Item Redeem transaction Logs
 * <TransactionType>:<BuyerCharacterID>:<RecipientCharacterID>:<ItemDebugName>:<TemplateID>
 */
get_etlconf(ProcessToken, $prefix, $db_url, $TblPrefix);
$TblInstance =  Partition == true ? $TblPrefix.'_'.TblToken : $TblPrefix;
define(TblNormalDefinition, 'create table if not exists '.$TblInstance.'_Normal (
        time timestamp default 0, type varchar(255), VendorID bigint(20) unsigned,
        ZoneID bigint(20) unsigned, StationID bigint(20) unsigned, CharacterID bigint(20) unsigned,
        ItemName varchar(255), ItemID bigint(20) unsigned, ItemCount int(11) default 1, 
        Currency enum(\'Crowns\',\'Gold\',\'AT\',\'Loot\',\'None\'), Cost int(11), Cause varchar(255),
        primary key (time,type,CharacterID,ItemID,Currency));');
define(TblGiftDefinition, 'create  table if not exists '.$TblInstance.'_Gift (
        time timestamp default 0,type enum(\'Gift\',\'Redeem\'),BuyerID bigint(20) unsigned,RecipID bigint(20) unsigned,
        ItemName varchar(255),ItemID bigint(20) unsigned,ItemCount int(11) default 1,
        primary key(time, type, BuyerID, RecipID, ItemID));');
// insert unique items: Purchase, Sold, Delete, Use, etc...
define(InsertUniqSql, 'insert into '.$TblInstance.'_Normal (time,type,VendorID,ZoneID,StationID,CharacterID,ItemName,ItemID,Currency,Cost,Cause) values ');
// insert gift items: Gift, Redeem
define(InsertAggSql, 'insert into '.$TblInstance.'_Gift (time,type,BuyerID,RecipID,ItemName,ItemID) values ');
// [not used] insert aggregated items: PetSnack, Reagent
//define(InsertAggSql, 'insert into '.$TblInstance.' (time,type,Vendor,CharacterID,Info,Currency,Cost,Cause) values ');

$files = array();
get_dirfilepat(PullDir, $prefix, $files);
if (count($files) == 0) {
    Alert('not find files: '. PullDir . "/$prefix***");
    return;
}

/*********************************** Main **************************************/
$db = new mysql_db($db_url);
if (!Partition) {
    Msg("create if not exists");
    do_create($db);
}

$agg_list = $db->_fetch('select id, type from etl_inventory_item
    where type = \'Reagent\' or type = \'PetSnack\';', 'assoc_all');

foreach($files as $ff) {
    Msg("process file: $ff");
    load_alarm_logfile($db, $ff);

    bak_file(BakDir, ProcessToken, $ff);
}
$db->close();


//[ITMTRN]04/20/12 19:48:53 [STAT] ITMTRN     Loot:1103331:33214047252069417:Steal Health TC MM:607475001:Loot:0:BoosterPackGiftRedeem
//[ITMTRN]04/20/12 19:48:53 [STAT] ITMTRN     Loot:1103331:33214047252069417:Sprite TC MM:1722687341:Loot:0:BoosterPackGiftRedeem
/*********************************** Functions **************************************/
function load_alarm_logfile(&$db, $f) {
    global $TblInstance, $agg_list;
    $lineSepLen = strlen(LogLineSep);
    $detailSubSys = 'ITMTRN';
    $sep = ':';
    $batch_num = 100;
    $tbl_flag = '';
    $rows = 0;
    $gift_values = '';
    $normal_values = '';
    $fd = fopen($f, 'r');
    while ($line = fgets($fd, CSV_LINE_MAX)) {
        if ($line == LogLineSep) {
            Msg('skip blank line');
            continue;
        }
        $beg = 1;
        $end = strpos($line, ']', $beg);
        $SysToken = substr($line, $beg, $end - $beg);
        $beg = $end + 1;
        $end = strpos($line, '[', $beg);
        $datetime = substr($line, $beg, $end - $beg);
        $beg = strpos($line, $detailSubSys, $end + 1);
        if ($SysToken != SYSTOKEN || $beg === false) {
            SendMail('ERROR: not expected Subsystem'.SYSTOKEN."/$detailSubSys", $line."\n\nwith SubSystem: [$SysToken]");
            exit(0);
        }
        ++$rows;
        // skip space
        $beg += strlen($detailSubSys) + 1;
        while($line[$beg] == ' ') ++$beg;

        $timestamp = strtotime($datetime);
        $datetime = date('Y-m-d H:i:s',  $timestamp);
        //$timeid = date('YmdHis',  $timestamp);
        $record = substr($line, $beg);
        $arr = explode($sep, $record);
        $type = $arr[0];
        if ($type == 'Gift' || $type == 'Redeem') {
            $buyer = $arr[1];
            $recip = $arr[2];
            $itemname = $arr[3];
            $itemid = $arr[4];
            $gift_values .= "('$datetime','$type','$buyer','$recip','$itemname','$itemid'),";
        } else {
            $zone = $station = '';
            $vendor = $arr[1];
            $charID = $arr[2];
            $itemname = $arr[3];
            $itemid = $arr[4];
            $currency = $arr[5];
            $cost = $arr[6];
            $cause = substr($arr[7], 0, strlen($arr[7]) - $lineSepLen);
            if ($type == 'Loot') {
                $vendor = '';
                $zone = $arr[1];
            }
            if ($type == 'Craft') {
                $vendor = '';
                $station = $arr[1];
            }
            $normal_values .= "('$datetime','$type','$vendor','$zone','$station','$charID','$itemname','$itemid','$currency','$cost','$cause'),";
        }

        if (!Partition) {
            if ($rows % $batch_num != 0) { continue; }
            do_real_insert($db, $tbl_flag, InsertUniqSql, $normal_values);
            do_real_insert($db, $tbl_flag, InsertAggSql, $gift_values);
        } else {
            $tmp_tbl_flag = date('Ym', $timestamp);
            if ($tbl_flag != $tmp_tbl_flag) {
                $tbl_flag = $tmp_tbl_flag;
                do_real_insert($db, $tbl_flag, InsertUniqSql, $normal_values, true);
                do_real_insert($db, $tbl_flag, InsertAggSql, $gift_values, true);
            }
            else if ($rows % $batch_num != 0) {
                continue;
            } else { // no create insert
                do_real_insert($db, $tbl_flag, InsertUniqSql, $normal_values);
                do_real_insert($db, $tbl_flag, InsertAggSql, $gift_values);
            }
        }
    }
    if ($normal_values != '' ) {
        do_real_insert($db, $tbl_flag, InsertUniqSql, $normal_values, true);
    }
    if ($gift_values != '' ) {
        do_real_insert($db, $tbl_flag, InsertAggSql, $gift_values, true);
    }

    Msg("load $rows rows => $TblInstance");
    fclose($fd);
}

function do_real_insert(&$db, &$tbl_flag, $pre_sql, &$values, $create = false)
{
    if ($create) {
        Msg("create if not exists");
        do_create($db, $tbl_flag);
    }

    if ($values == '') { return; }

    if (Partition) {
        $sql_str = str_replace(TblToken, $tbl_flag, $pre_sql);
    } else {
        $sql_str = $pre_sql;
    }
    $sql_str .= $values;
    $sql_str[strrpos($sql_str, ',')] = ' ';
    // a little trick, batch update :) only for this
    $sql_str .= 'on duplicate key update ItemCount = ItemCount + 1;';

    //Debug($sql_str);

    if ($db->exec_sql($sql_str)) {
        # sucess and reset values clause
        $values = '';
    } else {
        $db->print_err();
        SendMail("ERROR: insert table", $sql_str."\nSQL ERROR:".$db->get_err());
        exit(-1);
    }
}

function do_create(&$db, &$tbl_flag='')
{
    global $TblInstance;
    if (Partition) {
        $tbl_def = str_replace(TblToken, $tbl_flag, TblNormalDefinition);
    } else {
        $tbl_def = TblNormalDefinition;
    }
    if (!$db->exec_sql($tbl_def)) {
        $db->print_err();
        SendMail("ERROR: Create table $TblInstance", $tbl_def."\nSQL ERROR:".$db->get_err());
        exit(-1);
    }

    if (Partition) {
        $tbl_def = str_replace(TblToken, $tbl_flag, TblGiftDefinition);
    } else {
        $tbl_def = TblGiftDefinition;
    }
    if (!$db->exec_sql($tbl_def)) {
        $db->print_err();
        SendMail("ERROR: Create table $TblInstance", $tbl_def."\nSQL ERROR:".$db->get_err());
        exit(-1);
    }
}
?>
