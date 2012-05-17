<?
$is_system_running = false;

declare(ticks = 1);

function sysctl_sig_handler($sig_num)
{
    GLOBAL $is_system_running;
    if(($sig_num == SIGTERM) || ($sig_num == SIGINT) || ($sig_num == SIGQUIT)) {
        error_log('Caught exit signal.');
        $is_system_running = false;
    }
}

pcntl_signal(SIGTERM, 'sysctl_sig_handler', false);
pcntl_signal(SIGINT, 'sysctl_sig_handler', false);
pcntl_signal(SIGQUIT, 'sysctl_sig_handler', false);
pcntl_signal(SIGHUP, SIG_IGN);
pcntl_signal(SIGPIPE, SIG_IGN);

function sysctl_start_run()
{
    GLOBAL $is_system_running;

    $is_system_running = true;
    return true;
}

function sysctl_is_running()
{
    GLOBAL $is_system_running;
    return $is_system_running;
}

function sysctl_stop_run()
{
    GLOBAL $is_system_running;

    $is_system_running = false;
    return true;
}

?>
