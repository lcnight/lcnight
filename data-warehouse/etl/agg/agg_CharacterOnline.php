#! /usr/bin/php
<?php 
// according to QuestCompletion.{Starts|Cancels|Ends}
// update t_CharacterDim.{FirstLoginTime|TotalOnlineDuration|QuestsEnd}
require_once('setup.php');

define('SRC_CSVTOKEN', 'CharacterOnlineDuration');
define('CSVCols', 16);
define('DST_TblCharDim', 'CharacterDim');
define('DST_TblCharOnline', 'CharacterDayOnline');

// get source csv files
get_etlconf(SRC_CSVTOKEN, $prefix, $csv_db_url);
$files = array();
get_dirfilepat(PullDir, $prefix, $files);
//get_dirfilepat('/home/lc/wizard/wizard-stats/data-warehouse/base-dir/bak-dir/CharacterOnlineDuration', $prefix, $files);
if (count($files) == 0) {
    Alert('not find files: '. PullDir . "/$prefix***");
    return;
}

// get destination db/table
get_etlconf(DST_TblCharDim, $dim_prefix, $dim_db_url, $dim_tbl);
get_etlconf(DST_TblCharOnline, $online_prefix, $online_db_url, $online_tbl);

/*********************************** Main **************************************/

foreach($files as $f) {
    Msg('process file: '.$f);
    process_file($db, $f);
}

/*********************************** Functions **************************************/
function process_file(&$db, $f)
{
    $fd = fopen($f, 'r');
    $process_rows = 0;
    $rows = 0;
    $batch_num = 1000;
    // characterID => (firstlogin, allSessions, allOnlineDurations)
    $agg_fsd = array();
    // characterID:day => onlineDurations
    $agg_dd = array();
    while ($cols = fgetcsv($fd, CSV_LINE_MAX)) {
        if ($rows == 0) {
            $col_num = count($cols);
            if ($col_num != CSVCols) {
                $dump_cols = print_r($cols, true);
                SendMail("Error aggregate ".SRC_CSVTOKEN, "$col_num columns not as expected ".CSVCols." columns\n" . $dump_cols);
                exit(-1);
            }
            ++$rows;
            Msg("skip collumn name line");
            continue;
        }
        ++$rows;
        $ZoneId = intval($cols[3]);
        if ($ZoneID != 0) {
            continue;
        }
        ++$process_rows;
        $timestamp = strtotime($cols[1] . TZ_OFF);
        $CharacterID = $cols[2];
        $loginTime = $timestamp;
        $sessions = intval($cols[4]);
        $gold_rece = intval($cols[7]);
        $gold_paid = intval($cols[10]);
        $durations = intval($cols[13]);

        agg_dim($agg_fsd, $agg_dd, $CharacterID, $timestamp, $sessions, $durations, $gold_rece, $gold_paid);

        if ($process_rows % $batch_num == 0) {
            agg_to_db($db, $agg_fsd, $agg_dd);
        }
    }

    if ((count($agg_fsd) > 0 || count($agg_dd) > 0) && $process_rows > 0) {
        agg_to_db($db, $agg_fsd, $agg_dd);
    }

    Msg("aggregate $process_rows rows => ".DST_TblCharDim);
    fclose($fd);
}

// characterID => (firstlogin, allSessions, allOnlineDurations, goldReceived, goldPaidout)
// characterID:day => onlineDurations
function agg_dim(&$arr_fsd, &$arr_dd, $charID, $timestamp, $sessions, $durations, $grece, $gpaid)
{
    if (!isset($arr_fsd[$charID])) {
        $arr_fsd[$charID] = array();
        $arr_fsd[$charID][0] = $timestamp;
        $arr_fsd[$charID][1] = 0;
        $arr_fsd[$charID][2] = 0;
        $arr_fsd[$charID][3] = 0;
        $arr_fsd[$charID][4] = 0;
    }
    if ($arr_fsd[$charID][0] > $timestamp) {
        $arr_fsd[$charID][0] = $timestamp;
    }
    $arr_fsd[$charID][1] += $sessions;
    $arr_fsd[$charID][2] += $durations;
    $arr_fsd[$charID][3] += $grece;
    $arr_fsd[$charID][4] += $gpaid;

    $day = date('Ymd', $timestamp);
    $key = "$charID:$day";
    if (!isset($arr_dd[$key])) {
        $arr_dd[$key] = array();
        $arr_dd[$key][0] = 0;
    }
    $arr_dd[$key][0] += $durations;
}

function agg_to_db(&$db, &$arr_fsd, &$arr_dd)
{
    global $dim_tbl, $dim_db_url;
    global $online_tbl, $online_db_url;

    $db = new mysql_db($dim_db_url);
    foreach($arr_fsd as $k => $v) {
        $fst = $v[0];
        $ses = $v[1];
        $dur = $v[2];
        $grec = $v[3];
        $gpaid = $v[4];
        $up_sql = 'update '.$dim_tbl." set FirstLoginTime=if((FirstLoginTime=0)||($fst<FirstLoginTime),$fst,FirstLoginTime)
            ,TotalSessions=TotalSessions+$ses, TotalOnlineDuration=TotalOnlineDuration+$dur,
            GoldRecieved=GoldRecieved+$grec, GoldPaidOut=GoldPaidOut+$gpaid
            where CharacterID = $k";
        if (!$db->exec_sql($up_sql)) {
            var_dump($up_sql);
            $db->print_err();
            SendMail('Error to aggregate : '.SRC_CSVTOKEN,  DST_TblCharDim." SQL string: $up_sql\nERR: ".$sql_str.$db->get_err());
            exit(-1);
        }
        unset($arr[$k]);
    }
    $db->close();

    $db = new mysql_db($online_db_url);
    foreach($arr_dd as $k => $v) {
        list($id, $day) = explode(':', $k);
        $dur = $v[0];

        $in_sql = "insert $online_tbl(CharacterID,time,OnlineDuration) values ($id, $day, $dur) on duplicate key update OnlineDuration=OnlineDuration+$dur";
        if (!$db->exec_sql($in_sql)) {
            var_dump($in_sql);
            $db->print_err();
            SendMail('Error to aggregate : '.SRC_CSVTOKEN,  DST_TblCharOnline." SQL string: $in_sql\nERR: ".$sql_str.$db->get_err());
            exit(-1);
        }
        unset($arr[$k]);
    }
    $db->close();
}

?>
