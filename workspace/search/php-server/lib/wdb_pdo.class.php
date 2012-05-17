<?php
/**
 * WE DO BETTER - PDO
 *
 * This class is for you to deal with database more easily
 * @Baron <the_guy_1987@hotmail.com>
 * @version 1.0
 */
class WDB_PDO
{
    /**
     * 数据库PDO实例
     * @access private
     * @var object
     */
    private $wdb_pdo;

    /**
     * 是否为调试状态
     * @access private
     * @var boolean
     */
    private static $is_debug = true;

    /**
     * 数据库长连接属性
     * @access public
     * @var boolean
     */
    public static $attr_persistent = false;

    /**
     * 数据库长连接属性
     * @access private
     * @var string
     */
    private static $charset = 'UTF8';

    /**
     * 数据连接字符串
     * @access private
     * @var string
     */
    private $dsn = '';

    /**
     * 数据库用户
     * @access private
     * @var string
     */
    private $user = '';

    /**
     * 数据库用户密码
     * @access private
     * @var string
     */
    private $passwd = '';

    /**
     * 错误描述
     * @access private
     * @var string
     */
    private $err_desc = '';

    public function get_err_desc()
    {
        return $err_desc;
    }

    /**
     * 设定数据库连接字符集
     * @access public
     * @param $_charset
     */
    public static function set_charset($_charset)
    {
        self::$charset = $_charset;
    }

    /**
     * 设置工作模式
     * @access public
     * @param boolean $_flag
     */
    public static function set_mode_debug($_flag = true)
    {
        self::$is_debug = $_flag;
    }

    /**
     * 当前数据库资源是否可用
     */
    public function is_right_conn()
    {
        try
        {
            return $this->wdb_pdo->getAttribute(PDO::ATTR_CONNECTION_STATUS);
        }
        catch (Exception $e)
        {
            $this->error_handler($e);
            return false;
        }
    }

    /**
     * 构造函数
     * @access public
     * @param string $_dsn          数据源
     * @param string $_user         用户名
     * @param string $_pass         密码
     */
    function __construct($_dsn, $_user, $_pass)
    {
        if (empty($_dsn) || empty($_user) || empty($_pass))
            throw new Exception('parameters are not enough');
        $result = $this->switch_db($_dsn, $_user, $_pass);
        if (!$result)
        {
            throw new Exception('failed to get the instance of pdo');
        }
        else
        {
            $this->dsn = $_dsn;
            $this->user = $_user;
            $this->pass = $_pass;
        }
    }

    /**
     * 数据库重连
     * @access public
     * @return boolean
     */
    public function reconnect()
    {
        if (empty($this->dsn) || empty($this->user) || empty($this->pass))
        {
            return false;
        }
        return $this->switch_db($this->dsn, $this->user, $this->pass);
    }

    /**
     * 拼装DSN
     * @access public
     * @param string $_db_name    数据库名称
     * @param string $_host       主机地址
     * @param string $_port       主机端口
     * @param string $_db_source  数据源类型
     * @return string|false
     */
    public static function build_dsn($_db_name,
                                     $_host = 'localhost',
                                     $_port = 3306,
                                     $_db_source = 'mysql')
    {
        try
        {
            $dsn = sprintf("%s:host=%s;port=%s;dbname=%s",
                           $_db_source, $_host, $_port, $_db_name);
            return $dsn;
        }
        catch (Exception $e)
        {
            $this->error_handler($e);
            return false;
        }
    }

    /**
     * 拼装DSN - socket
     * @access public
     * @param string $_db_name    数据库名称
     * @param string $_db_socket  数据socket
     * @param string $_db_source  数据源类型
     * @return string|false
     */
    public static function build_dsn_by_socket($_db_name, $_db_socket, $_db_source = 'mysql')
    {
        try
        {
            $dsn = sprintf("%s:unix_socket=%s;dbname=%s",
                           $_db_source, $_socket, $_db_name);
            return $dsn;
        }
        catch (Exception $e)
        {
            $this->err_handler($e);
            return false;
        }
    }

    /**
     * 切换DB方法
     * @access public
     * @param string $_dsn    数据源
     * @param string $_user   用户名
     * @param string $_pass   密码
     * @return true|false
     */
    public function switch_db($_dsn, $_user, $_pass)
    {
        if (empty($_dsn) || empty($_user) || empty($_pass))
            return false;
        try
        {
            $this->wdb_pdo = new PDO($_dsn, $_user, $_pass);
            $this->wdb_pdo->setAttribute(PDO::ATTR_ERRMODE,
                                         PDO::ERRMODE_EXCEPTION);
            //$this->wdb_pdo->setAttribute(PDO::ATTR_DEFAULT_FETCH_MODE,
              //                           PDO::FETCH_ASSOC);
            if (!empty(self::$charset))
            {
                $set_charset = $this->execute('SET NAMES ' . self::$charset);
                if ($set_charset === false)
                    throw new Exception('exception: failed to set charset');
            }

            if (!$this->is_right_conn())
                return false;

            return true;
        }
        catch (Exception $e)
        {
            $this->error_handler($e);
            return false;
        }
    }

    /**
     * 预编译SQL语句
     * @access public
     * @param string $_statement     SQL语句
     * @return false|PDOStatement    PDO申明对象
     */
    public function prepare($_sql)
    {
        //if (self::$is_debug) {
        //    error_log('SQL: '.$_sql);
        //}

        try
        {
            return $this->wdb_pdo->prepare($_sql);
        }
        catch (Exception $e)
        {
            $this->error_handler($e);
            return false;
        }
    }

    /**
     * 开始事务
     * @access public
     * @return {boolean}
     */
    public function beginTransaction()
    {
        return $this->wdb_pdo->beginTransaction();
    }

    /**
     * 提交事务
     * @access public
     * @return {boolean}
     */
    public function commit()
    {
        return $this->wdb_pdo->commit();
    }

    /**
     * 事务回滚
     * @access public
     * @return {boolean}
     */
    public function rollBack()
    {
        return $this->wdb_pdo->rollBack();
    }

    /**
     * 执行SQL语句批量处理 - 事务
     * @access public
     * @param array $_sql
     * @return boolean
     */
    public function commits($_sql)
    {
        $this->wdb_pdo->beginTransaction();
        try
        {
            foreach ($_sql as $key => &$var)
                $this->wdb_pdo->exec($var);
            $this->wdb_pdo->commit();
            return true;
        }
        catch (Exception $e)
        {
            $this->error_handler($e);
            $this->wdb_pdo->rollBack();
            return false;
        }
    }

    // 常量:SQL执行返回结果：自动，执行，取第一单元格，取整行，取
    const SQL_RETURN_TYPE_AUTO = 'auto';
    const SQL_RETURN_TYPE_EXEC = 'exec';
    const SQL_RETURN_TYPE_CELL = 'cell';
    const SQL_RETURN_TYPE_ROW  = 'row';
    const SQL_RETURN_TYPE_ALL  = 'all';

    /**
     * 执行SQL
     * @access public
     * @param string $_sql      SQL语句
     * @param string $_type     返回类型
     * @param array  $_param    相关参数
     * @return object
     */
    public function execute($_sql,
                            $_type = self::SQL_RETURN_TYPE_EXEC,
                            $_param = NULL)
    {
        try
        {
            $sth = $this->wdb_pdo->prepare($_sql);
            $sth->execute($_param);
            switch ($_type)
            {
                case self::SQL_RETURN_TYPE_EXEC:
                    $result = $sth->rowCount();
                    break;
                case self::SQL_RETURN_TYPE_CELL:
                    $result = $sth->fetchColumn();
                    if ($result === false)
                        $result = null;
                    break;
                case self::SQL_RETURN_TYPE_ROW:
                    $result = $sth->fetch();
                    // 若取数据失败仍然返回数据数组,内容为空
                    if ($result === false)
                        $result = array();
                    break;
                case self::SQL_RETURN_TYPE_ALL:
                    $result = $sth->fetchAll();
                    break;
                default:
                   $result = $sth->rowCount();
                   break;
            }
            return $result;
        }
        catch (Exception $e)
        {
            $this->error_handler($e);
            return false;
        }
    }

    /**
     * 查看是否存在某域某值
     * @param {string} $_table  表名
     * @param {string} $_field  域名
     * @param {object} $_value  值
     */
    public function exists($_table, $_field, $_value)
    {
        $sql = 'select exists(select ' . $_field . ' from ' . $_table
               . ' where ' . $_field . '=?) as is_exist';
        $sth = $this->wdb_pdo->prepare($sql);
        if (is_array($_value))
        {
            foreach ($_value as $key => &$var)
            {
                try
                {
                    $sth->execute(array($var));
                    $data = $sth->fetch();
                }
                catch (Exception $e)
                {
                    $this->error_handler($e);
                    return false;
                }
                if (1 != $data['is_exist'])
                    return false;
            }
            return true;
        }
        else
        {
            try
            {
                $sth->execute(array($_value));
                $data = $sth->fetch();
            }
            catch (Exception $e)
            {
                $this->error_handler($e);
                return false;
            }
            if (1 != $data['is_exist'])
                return false;
            else
                return true;
        }
    }

    const SQL_QUERY_TYPE_SELECT     = 'select';
    const SQL_QUERY_TYPE_INSERT     = 'insert';
    const SQL_QUERY_TYPE_REPLACE    = 'replace';
    const SQL_QUERY_TYPE_UPDATE     = 'update';
    /**
     * 拼装SQL语句
     * @access public
     * @param string $_type   sql语句类型
     * @param array  $_param  sql相关参数
     * @return string sql     拼装后的SQL语句
     */
    public function build_sql($_type, $_param)
    {
        switch ($_type)
        {
            case self::SQL_QUERY_TYPE_SELECT:
                $sql = self::build_sql_select($_param['field'],
                                              $_param['table'],
                                              $_param['where'],
                                              $_param['group_by'],
                                              $_param['order_by'],
                                              $_param['limit']);
                break;
            case self::SQL_QUERY_TYPE_INSERT:
                $sql = self::build_sql_insert($_param['field'],
                                              $_param['table'],
                                              $_param['value'],
                                              $_param['type']);
                break;
            case self::SQL_QUERY_TYPE_UPDATE:
                $sql = self::build_sql_update($_param['field'],
                                              $_param['table'],
                                              $_param['value'],
                                              $_param['where']);
                break;
            default:
                $sql = '';
        }
        return $sql;
    }

    /**
     * 拼装SQL语句 - select
     * @access public
     * @param array  $_field      sql语句类型
     * @param string $_table      查询表
     * @param array  $_where      where部分
     * @param array  $_group_by   group by部分
     * @param array  $_order_by   order by部分
     * @param string $_limit      limit部分
     * @return string $sql        拼装后的SQL语句
     */
    public static function build_sql_select($_field,
                                            $_table,
                                            $_where,
                                            $_group_by,
                                            $_order_by,
                                            $_limit)
    {
        $sql = '';
        $field = implode(',', $_field);
        $where    = !empty($_where) ?
                        'where' . implode(',', $_where) : '';
        $group_by = !empty($_group_by) ?
                        'group by ' . implode(',', $_group_by) : '';
        $order_by = !empty($_order_by) ?
                        'order by ' . implode(',', $_order_by) : '';
        $limit    = !empty($_limit) ?
                        'limit ' . implode(',', $_limit) : '';
        $sql = 'select ' . $field . ' from ' . $_table
               . ' ' . $where
               . ' ' . $group_by
               . ' ' . $order_by
               . ' ' . $limit;
        return $sql;
    }

    /**
     * 拼装SQL语句 - insert/replace
     * @access public
     * @param array  $_field   sql语句类型
     * @param string $_table   查询表
     * @param array  $_values  插入/更新数据
     * @param array  $_type    insert/replace
     * @return string          拼装后的SQL语句
     */
    public static function build_sql_insert($_field,
                                        $_table,
                                        $_value,
                                        $_type = 'insert')
    {
        $sql = '';
        $field = implode(',', $_field);
        $value = implode(',', $_value);
        $sql = 'insert into ' . $_table . ' (' . $field . ')'
               . ' values (' . $value . ')';
        return $sql;
    }

    /**
     * 拼装SQL语句 - update
     * @access public
     * @param array     $_field      sql语句类型
     * @param string    $_table      查询表
     * @param array     $_value      update值
     * @param array     $_where      where部分
     * @return string             拼装后的SQL语句
     */
    public static function build_sql_update($_field,
                                            $_table,
                                            $_value,
                                            $_where)
    {
        $sql = '';
        $field = implode(',', $_field);
        $where = !empty($_where) ?
                    'where' . implode(',', $_where) : '';
        $arr_set = array();
        foreach ($_field as $key => &$var)
        {
            $arr_set[] = $var . '=' . self::escape_string($_value[$key]);
        }
        $str_set = implode(',', $arr_set);
        $sql = 'update ' . $_table
               . ' set ' . $str_set
               . ' ' . $where;
        return $sql;
    }

    /**
     * 转义特殊字符
     * @access public
     * @param string $_str    待过滤字符串
     * @return string         过滤后字符串
     */
    public static function escape_string($_str)
    {
        return mysql_real_escape_string($_str);
    }

    /**
     * 错误处理函数名
     * @access private
     * @var string
     */
    private static $error_handler_name = '';

    /**
     * 设置错误处理函数名
     * @access private
     * @param string $_error_handler_name
     */
    public static function set_error_handler_name($_error_handler_name)
    {
        self::$error_handler_name = $_error_handler_name;
    }

    /**
     * 错误处理函数
     * @param {Exception} $e
     */
    private function error_handler($e)
    {
        $this->err_desc = $e->getMessage();
        if (self::$is_debug)
        {
            $func = self::$error_handler_name;
            if (function_exists($func))
                $func($e);
            else
                echo $e->getMessage();
        }
    }
}
?>
