#! /usr/bin/php -f

<?php 
require_once('mysql_db.php');
require_once('util.php');
require_once('../setup.php');

//$db = new mysql_db("root:ta0mee@10.1.1.60/test");
//$arr = $db->get('total', 'a=1');
$db = new mysql_db("root:ta0mee@10.1.1.60/db_wizard_dw");
//$arr = $db->_fetch('select * from etl_pet where id = 225462;', 'num_all');
$arr = $db->_fetch('show table status;', 'assoc_all');
var_dump($arr);
$db->close();

//get_etlconf('QuestCompletion', $pre, $db, $tbl);
//echo "$pre, $db, $tbl\n";

//get_etlconf('QuestCompletion', $pre, $db);
//echo "$pre, $db, $tbl\n";

//send_mail("hello", "test from php");

//var_dump(get_num("aaa"));
//var_dump(get_num("aaa 23"));
//var_dump(get_num("aaa (44)"));
//var_dump(get_num("55)"));
//

//$file = 'PetLog_1204130600.txt';
//echo file_used($file, $pp);
?>
