#! /usr/bin/php -f
<?php
require_once('mysql_db.php');

$logdir = './monitorlog';
if (!file_exists($logdir)) {
    mkdir($logdir);
}
$detaillog = $logdir.'/mdb_detail.csv';
if (file_exists($detaillog)) {
    $fd = fopen($detaillog, 'a+');
} else {
    $fd = fopen($detaillog, 'a+');
    fwrite($fd,"Mtime,DB_Token,Name,Engine,Row_format,Rows,Avg_row_length,Data_length,Index_length,Data_free,Collation,Auto_increment,Create_time,Update_time,Check_time\n");
}
$sumlog = $logdir.'/mdb_sum.csv';
if (file_exists($sumlog)) {
    $sumfd = fopen($sumlog, 'a+');
} else {
    $sumfd = fopen($sumlog, 'a+');
    fwrite($sumfd,"Mtime,db_uri,all_Data_length,all_Index_length\n");
}

// configure monitoring databases
$dblist = array('db_wizard_dw_dx' => 'dw:dwdwdwpwd@192.168.71.35/db_wizard_dw_dx',
	'db_wizard_dw_wt' => 'dw:dwdwdwpwd@192.168.71.35/db_wizard_dw_wt');
foreach($dblist as $key => $uri) {
    $db = new mysql_db($uri);
    $arr = $db->_fetch('show table status;', 'assoc_all');
    $db_all = array();
    $db_all['db_uri'] = $key;
    $db_all['alldata'] = 0;
    $db_all['allidx'] = 0;
    foreach($arr as $row) {
        $db_all['alldata'] += $row['Data_length'];
        $db_all['allidx'] += $row['Index_length'];
        write_status($fd, $key, $row);
    }
    write_sum($sumfd, $db_all);
    $db->close();
}
fclose($fd);
fclose($sumfd);

function write_sum(&$fd, &$row)
{
    fprintf($fd, "%s,%s,%s,%s\n",date('YmdHi'), $row['db_uri'], $row['alldata'], $row['allidx']);
}

function write_status(&$fd, &$key, &$row)
{
    //fprintf($fd, "%32s%32s%12s%12s%12s%12s%32s%12s%12s%12s%12s%12s%12s%16s\n", $row['Name'], $row['Engine'], $row['Version'], $row['Row_format'], $row['Rows'], $row['Avg_row_length'], $row['Data_length'], $row['Index_length'], $row['Data_free'], $row['Auto_increment'], $row['Create_time'], $row['Update_time'], $row['Check_time'], $row['Collation']);
 
    fprintf($fd, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,\n", date('YmdHi'), $key, $row['Name'], $row['Engine'], 
        $row['Row_format'], $row['Rows'], $row['Avg_row_length'], $row['Data_length'], $row['Index_length'], 
        $row['Data_free'], $row['Collation'], $row['Auto_increment'], $row['Create_time'], $row['Update_time']);
}

?>
