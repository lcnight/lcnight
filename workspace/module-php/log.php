<?php 
if(!defined('nl')) define('nl', "\n");

class LOG {
    private static $m_init = false;
    private static $m_logdir = './logs/';
    private static $m_logpre = 'mylog-';
    private static $m_logdst = array('console'); // can be 'file' | 'console'
    private static $m_logprocs = array(); // array('file' => 'log_file', 'console'=>'log_console');
    private static $m_logfiles = array('Debug' => null, 'Info' => null, 'Warn' => null, 'Error' => null);
    private static $m_fd = null;

    public static function initialize($logdst = array('console'), $logpre = 'mylog-', $logdir = './logs/') {
        self::$m_logdst = $logdst;

        if(!file_exists($logdir)) mkdir($logdir);
        self::$m_logdir = $logdir;
        self::$m_logpre = $logpre;

        foreach($logdst as $dst) {
            switch ($dst) {
            case 'file': 
                self::$m_logprocs['file'] = 'log_file';
                break;
            case 'console':
                self::$m_logprocs['console'] = 'log_console';
                break;
            default:
                fprintf(STDERR, 'supply not supported log destination'.nl);
                exit(-1);
            }
        }
        return self::$m_init = true;
    }

    public static function OK() {
        if (self::$m_init == false) {
            return false;
        }
    } 

    public static function open_file($token = 'Debug') 
    {
        if (self::$m_logfiles[$token] == null) {
            self::$m_logfiles[$token] = 
                fopen(self::$m_logdir . '/' . self::$m_logpre  . "$token.txt" , 'a+');
        }

        return self::$m_logfiles[$token];
    }

    public static function do_log(&$fd, $token = 'Debug', $msg = '')
    {
        if (!self::OK() && !self::initialize()) {
            fprintf(STDERR, 'fail to initialize'.nl);
            return ;
        }

        if ($msg == '') {
            $logmsg = nl;
        } else {
            $logmsg = '['.date('Y-m-d H:i:s').'] '. readlink('/proc/self') .' ['.$token.'] ' . $msg . nl;;
        }

        //var_dump(self::$m_logprocs);
        foreach(self::$m_logprocs as $dst => $proc) {
            $proc($fd, $logmsg);
        }
    } 

    public static function Error($msg = '') 
    {
        $fd = self::open_file('Error');
        self::do_log($fd, 'Error', $msg);
    }

    public static function Warn($msg = '') 
    {
        $fd = self::open_file('Warn');
        self::do_log($fd, 'Warn', $msg);
    }

    public static function Info($msg = '') 
    {
        $fd = self::open_file('Info');
        self::do_log($fd, 'Info', $msg);
    }

    public static function Debug($msg = '') 
    {
        $fd = self::open_file('Debug');
        self::do_log($fd, 'Debug', $msg);
    }

}

function log_file(&$fd, &$msg) {
    fwrite($fd, $msg);
}

function log_console(&$fd, &$msg) {
    fprintf(STDOUT, $msg);
}

?>
