<?php
class log
{
    /**
     * 写日志
     *
     * @param string $s_message 日志信息
     * @param string $s_type    日志类型
     */
    public static function write($s_message, $s_type = 'log')
    {
        $s_now_time = date('[y-m-d H:i:s]');
        $s_now_day  = date('Y_m_d');
        // 根据类型设置日志目标位置
        $s_target   = LOG_PATH;
        if(!is_dir($s_target)) @mkdir($s_target, 755);
        switch($s_type)
        {
            case 'debug':
                $s_target .= 'Debug_' . $s_now_day . '.log';
                break;
            case 'warn':
                $s_target .= 'Warn_' . $s_now_day . '.log';
                break;
            case 'error':
                $s_target .= 'Err_' . $s_now_day . '.log';
                break;
            case 'data':
                $s_target .= 'Data_' . $s_now_day . '.log';
                break;
            case 'log':
                $s_target .= 'Log_' . $s_now_day . '.log';
                break;
            default:
            	$s_target .= 'Log_' . $s_now_day . '.log';
                break;
        }
        // 检查日志目录是否可写
        if (! is_writable(LOG_PATH)) {
            exit('日志目录不可写!');
        }
        //检测日志文件大小, 超过配置大小则重命名
        if (file_exists($s_target) && LOG_FILE_SIZE <= filesize($s_target)) {
            $s_file_name = substr(basename($s_target), 0, strrpos(basename($s_target), '.log')) . '_' . time() . '.log';
        	rename($s_target, dirname($s_target) . DS . $s_file_name);
        }
        clearstatcache();
        // 写日志, 返回成功与否
        return error_log("$s_now_time $s_message\n", 3, $s_target);
    }

    /**
     * 写日志
     *
     * @param string $s_message 日志信息
     * @param string $s_type    日志类型
     */
    public static function write_data($s_message, $s_type = "")
    {
        $s_now_day  = date('Y_m_d');
        // 根据类型设置日志目标位置
        $s_target   = LOG_DATA_PATH;
        if(!is_dir($s_target)) @mkdir($s_target, 755);

        $s_target .= 'Data_' . $s_type . '_' . $s_now_day . '.log';

        // 检查日志目录是否可写
        if (! is_writable(LOG_DATA_PATH)) {
            exit('日志目录不可写!');
        }
        //检测日志文件大小, 超过配置大小则重命名
        if (file_exists($s_target) && LOG_DATA_FILE_SIZE <= filesize($s_target)) {
            $s_file_name = substr(basename($s_target), 0, strrpos(basename($s_target), '.log')) . '_' . time() . '.log';
        	rename($s_target, dirname($s_target) . DS . $s_file_name);
        }
        clearstatcache();
        // 写日志, 返回成功与否
        return error_log("$s_message\n", 3, $s_target);
    }
    
}
