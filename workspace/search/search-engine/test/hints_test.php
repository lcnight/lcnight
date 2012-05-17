<?php
$argv = $_SERVER['argv'];

$array = array('cmd_id' => 2001,
               'keyword' => $argv[1]);

$json_array = json_encode($array) . "\r\n";

echo $json_array, "\n";

$socket = socket_create(AF_INET,SOCK_STREAM,SOL_TCP);
if(!$socket)
{
    echo("create socket failed!!!\n");
    exit();
}

if(!socket_connect($socket, "10.1.8.136", 12601))
{
    echo("connect to server failed!!!\n");
    exit();
}

$result = socket_write($socket,$json_array,strlen($json_array));
if($result === false)
{
    echo("send data to server failed!!!\n");
    exit();
}

if($result != strlen($json_array))
{
    echo("unable to send all data!!!\n");
    exit();
}

//recv from server
$recved_string = socket_read($socket,16 * 1024,PHP_BINARY_READ);
if($recved_string == false || strlen($recved_string) <= 0)
{
    echo("recved data from server failed!!!!\n");
    exit();
}

$rcv_array = json_decode($recved_string, true);

echo "status \033[1;40;32m" . $rcv_array['status'] . "\033[0m\n";

if ($rcv_array['status'] != 0) {
    exit();
}

echo "keyword hints: \n";
foreach($rcv_array['keyword_hints'] as $k) {
    echo "\033[1;40;31m" . $k . "\033[0m\n";
}

?>
