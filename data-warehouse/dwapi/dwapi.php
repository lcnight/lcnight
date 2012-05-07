<?php 
/*************************************************************************
/// ?op=<value>&param
/// op:     operation type
/// param:  operation parameter, comma separated
//  error code: 1 => parameter missing
//              2 => not support operation
//
*************************************************************************/ 
require_once('mysql_db.php');

define('DB_URL', 'root:ta0mee@10.1.1.60/db_wizard_dw');
function err_ret(&$arr)
{
    echo json_encode($arr);
    exit(0);
}
$param_missing = array('c' => 1);
$param_error = array('c' => 2);
$op_not_support = array('c' => 3);

if (!isset($_REQUEST['op'])) {
    err_ret($param_missing);
}

$op = $_REQUEST['op'];
$result = array('c' => 0);
switch ($op)
{
//  op      get_on_dur  # get online duration
//          day         # example: 2012-04-18
//          mimi        # example: 98900033
//  result  {"c": err_code, "time": total_seconds}
case 'get_on_dur' : 
    if (!isset($_REQUEST['day']) || !isset($_REQUEST['mimi'])) {
        err_ret($param_missing);
    }
    $day = $_REQUEST['day'];
    $mimi = $_REQUEST['mimi'];
    if (!is_numeric($mimi)) {
        err_ret($param_error);
    }
    $db = new mysql_db(DB_URL);
    $sql_str = "select sum(OnlineDuration) from t_CharacterOnlineDuration force index (primary) where time between '$day 0:0:0' and '$day 23:59:59' and CharacterID in (select CharacterID from t_CharacterDim where AccountID = '$mimi') and ZoneID = 0";
    $total_secs = $db->_fetch($sql_str, 'cell');
    $result['time'] = intval($total_secs);
    break;

//  op      get_all_dur # get all days online duration
//          mimi        # example: 98900033
//  result  {"c": err_code, "alldays": {[day: times, ... ]}}
case 'get_all_dur':
    if (!isset($_REQUEST['mimi'])) {
        err_ret($param_missing);
    }
    $mimi = $_REQUEST['mimi'];
    if (!is_numeric($mimi)) {
        err_ret($param_error);
    }
    $db = new mysql_db(DB_URL);
    $sql_str = 'select a.CharacterID, b.time, b.OnlineDuration from (select CharacterID from t_CharacterDim where AccountID = '.$mimi.') a inner join t_CharacterDayOnline b on a.CharacterID = b.CharacterID order by CharacterID, time';
    $alldays = $db->_fetch($sql_str, 'assoc_all');
    $result['alldays'] = $alldays;

default :
    err_ret($op_not_support);
    break;
}  /* end of switch */
echo json_encode($result);

?>
