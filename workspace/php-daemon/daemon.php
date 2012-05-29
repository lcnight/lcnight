#! /usr/bin/php5 -q
<?php 
for ($iixx = 0 ; $iixx < 3 ; ++$iixx) {
    fwrite(STDOUT, "stdout $iixx\n");
    sleep(2);
}
function close_std_inout()
{
    fclose(STDIN);
    fclose(STDOUT);
    fclose(STDERR);
}

$fd = fopen("./log", "a+");

$pid = pcntl_fork();
if ($pid < 0) {
    fwrite($fd, "fork child process fail \n");
    exit(-1);
} 
if ($pid > 0) {
    fwrite($fd, "parent process, exit(0) wtih child pid $pid \n");
    exit(0);
} 

//set session head
$sid = posix_setsid() ;
if ($sid < 0) {
    fwrite($fd, "fail to set session \n");
} else {
    fwrite($fd, "new session id $sid\n");
}

close_std_inout();

//run in session leader
//fork child in daemon
$pid = pcntl_fork();
if ($pid < 0) {
    fwrite($fd, "fork child process fail \n");
    exit(-1);
} 
else if ($pid > 0) {
    $cld_arr = array();
    $cld_arr[] = $pid;
    //var_dump($cld_arr);

    while(count($cld_arr) > 0) {
        sleep(1);
        fwrite($fd, "start to waitpid exit\n");
        $e_pid = pcntl_waitpid(-1, $status, WNOHANG);
        fwrite($fd, "waitpid p:$e_pid s:$status\n");
        if ($e_pid == 0) {
            fwrite($fd, "no child process available\n");
            continue;
        }
        if (in_array($e_pid, $cld_arr)) {
            fwrite($fd, "process $e_pid has exit with status $status\n");
            unset($cld_arr[array_search($e_pid, $cld_arr)]);

            $refork_pid = pcntl_fork();
            if ($refork_pid == 0) {
                child_process($fd, "refork hello world");
            } else {
                $cld_arr[] = $refork_pid;
                fwrite($fd, "re fork process $refork_pid\n");
            }
        } else {
            fwrite($fd, "unexpect process $e_pid\n");
        }
    }
    fwrite($fd, "all children have exited\n");
} 
else {
    $time = time();
    fwrite($fd, "[b] $time child process running ... \n");
    
    child_process($fd, "hello world");

    $time = time();
    fwrite($fd, "[e] $time child end ...\n");
} 

function child_process(&$fd, $str)
{
    $fd = fopen("./child_work", "a+");
    fwrite($fd, "get $str \n");
    sleep(5);
    fwrite($fd, "after 5 seconds\n");
}


?>
