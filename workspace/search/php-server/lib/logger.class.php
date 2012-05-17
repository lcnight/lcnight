<?php
/**
 * 录入日志类
 */
class logger
{
    private static $dir     = '';
    private static $prefix  = '';
    private static $path    = '';
    private static $max_size = 31457280; /// 30m
    private static $serial_no = 0;
    private static $log_date = '';

    /**
     * 设置单个日志最大容量
     * @access public static
     * @param interger $max_size * @return NULL
     */
    public static function set_max_size($max_size)
    {
        self::$max_size = $max_size;
    }

    /**
     * 初始化
     * @access public static
     * @param string $str
     * @param string $prefix
     * @return boolean
     */
    public static function init($dir, $prefix)
    {
        if (empty($dir) || empty($prefix))
        {
            return false;
        }

        /// 开启日志
        if ((!file_exists($dir)) && (!mkdir($dir)))
        {
            return false;
        }

        self::$dir      = $dir;
        self::$prefix   = $prefix;
        $write_res = self::write('start logging!');
        if (false === $write_res)
        {
            return false;
        }
        return true;
    }

    /**
     * 创建新日志
     * @access private static
     * @return boolean
     */
    private static function new_log()
    {
        return ini_set('error_log', self::$path);
    }

    /**
     * 写日志
     * @access public static
     * @param string $str
     * @return boolean
     */
    public static function write($str)
    {
        /// 日志名
        if (date('Y-m-d') != self::$log_date)
        {
            self::$serial_no = 0;
            self::$log_date = date('Y-m-d');
            self::$path = self::$dir . self::$prefix . '_' . self::$log_date . '_00.log';
            self::new_log();
        }

        /// 冲洗文件状态缓存
        clearstatcache();

        /// 最大限制
        if (file_exists(self::$path) && (filesize(self::$path) >= (self::$max_size)))
        {
            ++ self::$serial_no;
            self::$path = self::$dir . self::$prefix . '_' . self::$log_date . '_' . str_pad(self::$serial_no, 2, '0', STR_PAD_LEFT) . '.log';
            $result = self::new_log();
            if (false === $result)
            {
                return false;
            }
        }
        error_log($str);
        return true;
    }
}
