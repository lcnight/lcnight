#! /usr/bin/php -f
<?php 
require_once('setup.php');

define(Partition, false);
define(ProcessToken, 'CharacterLevelDuration');
define(TblPrefix, 't_CharacterLevelDuration');
define(TblToken, '${XXXXXX}');
$TblInstance =  Partition == true ? TblPrefix.'_'.TblToken : TblPrefix;
define(TblDefinition, 'create table if not exists '.$TblInstance.' (
        time timestamp default 0, CharacterID bigint(36) unsigned, Level int(11), 
        OnlineDuration int(11), CalendarDuration int(11), primary key (CharacterID, Level)); 
');
define(InsertSql, 'insert into '.$TblInstance.' (time, CharacterID, Level, OnlineDuration, CalendarDuration) values ');

get_etlconf(ProcessToken, $prefix, $db_url, $tbl);
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
foreach($files as $ff) {
    Msg("process file: $ff");
    load_csv_file($db, $ff);

    bak_file(BakDir, ProcessToken, $ff);
}
$db->close();


/*********************************** functions **************************************/
function load_csv_file($db, $f) {
    $tbl_flag = '';
    $rows = 0;
    $values = '';
    $fd = fopen($f, 'r');
    while ($cols = fgetcsv($fd, CSV_LINE_MAX)) {
        if ($rows == 0) {
            Msg("skip title name line");
            ++$rows;
            continue;
        }
        ++$rows;
        $timestamp = strtotime($cols[1] . TZ_OFF);
        $time = date('Y-m-d H:i:s', $timestamp);
        $CharacterID = $cols[2];
        $Level = $cols[3];
        $OnlineDuration = $cols[4];
        $CalendarDuration = $cols[7];
        $values .= "('$time','$CharacterID','$Level','$OnlineDuration','$CalendarDuration') on duplicate key update OnlineDuration=OnlineDuration+'$OnlineDuration',CalendarDuration=CalendarDuration+'$CalendarDuration';";

        if (!Partition) {
            do_real_insert($db, $tbl_flag, $values);
        } else {
            $tmp_tbl_flag = date('Ym', $timestamp);
            if($tbl_flag != $tmp_tbl_flag) {
                $tbl_flag = $tmp_tbl_flag;
                do_real_insert($db, $tbl_flag, $values, true);
            } 
            else { // no create insert 
                do_real_insert($db, $tbl_flag, $values);
            } 
        }
    }

    $real_rows = $rows - 1;
    Msg("load $real_rows rows => ".TblPrefix."_$tbl_flag");

    fclose($fd);
}

function do_real_insert(&$db, &$tbl_flag, &$values, $create = false)
{
    if ($create) {
        Msg("create if not exists");
        do_create($db, $tbl_flag);
    }

    if (Partition) {
        $sql_str = str_replace(TblToken, $tbl_flag, InsertSql); 
    } else {
        $sql_str = InsertSql;
    }
    $sql_str .= $values;

    Debug($sql_str);

    if ($db->exec_sql($sql_str)) {
        # sucess and reset values clause
        $values = '';
    } else {
        $db->print_err();
        exit(-1);
    }
}

function do_create(&$db, &$tbl_flag='')
{
    if (Partition) {
        $tbl_def = str_replace(TblToken, $tbl_flag, TblDefinition);
    } else {
        $tbl_def = TblDefinition;
    }

    if (!$db->exec_sql($tbl_def)) {
        $db->print_err();
        exit(-1);
    }
}
?>
