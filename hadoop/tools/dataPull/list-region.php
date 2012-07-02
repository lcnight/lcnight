#! /usr/bin/php

<?php
if(count($argv) < 4) {
    echo "exe gameid type <YYYYMM[ YYYYMM]>\n";
    exit(0);
}
require_once('mysql_db.php');
require_once('comm_func.php');

$gameid = $argv[1];
$type = $argv[2];
$timestart = $argv[3];

$db = new mysql_db('hadoop:HA#2jsOw%x@192.168.71.45/db_userstats');
$result = $db->_fetch("select distinct regionid from t_game_info where gameid = $gameid and paid = -1 and channelid = -1  and time >= $timestart order by regionid", 'all');

$regionids = array();
foreach($result as $row) {
    $regionids[] = $row[0];
}

$tmps = array();
$idxMax = count($argv);
foreach($regionids as $id) {
    for($i = 3; $i < $idxMax; ++$i) {
        $time = $argv[$i];
        if(!isset($tmps[$time])) {
            $tmps[$time] = array();
        }
        $value = $db->_fetch("select $type from t_game_info where gameid = $gameid and paid = -1 and channelid = -1  and time = $time and regionid = $id", 'cell');
        if($value == null) {
            $value = 0;
        } else {
            $value = intval($value);
        }
        $tmps[$time][$id] = $value;
    }
}

timeid_arr_sort_($tmps);
timeid_arr_topn_($tmps);
timeid_arr_print_($tmps);

//$sortResults = array();
//arsort($tmps[$argv[$idxMax - 1]]);
//foreach($tmps[$argv[$idxMax - 1]] as $id => $value) {
    //for($i = 3; $i < $idxMax; ++$i) {
        //$timeTok = $argv[$i];
        //if (!isset($sortResults[$timeTok])) {
            //$sortResults[$timeTok] = array();
        //}
        //$sortResults[$timeTok][$id] = $tmps[$timeTok][$id];
    //}
//}

//$sortIds = $tmps[$argv[$idxMax - 1]];
//#print_r($sortResults);
//$idPrinted = false;
//foreach($sortIds as $id => $Value) {
    //echo "$id\t";
    //for($i = 3; $i < $idxMax; ++$i) {
        //echo $sortResults[$argv[$i]][$id] . "\t";
    //}
    //echo "\n";
//}

?>
