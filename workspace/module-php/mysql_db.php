<?php
class mysql_db {
    private $host = '';
    private $user = '';
    private $pwd = '';
    private $db = '';

    private $mysqli = '';
    private $inited = false;

    public function __construct($db_url) {
        // user:password@host[:port]/db
        $beg = 0;
        $end = strpos($db_url, ':');
        $this->user = substr($db_url, $beg, $end - $beg);
        $beg = $end + 1;
        $end = strpos($db_url, '@', $beg);
        $this->pwd = substr($db_url, $beg, $end - $beg);
        $beg = $end + 1;
        $end = strpos($db_url, '/', $beg);
        $this->host = substr($db_url, $beg, $end - $beg);

        $this->db = substr($db_url, $end + 1);
    }

    //public function __construct($h = 'localhost', $u = 'root', $p = 'ta0mee')
    //{
        //$this->host = $h;
        //$this->user = $u;
        //$this->pwd = $p;
        //$this->inited = false;
    //}

    public function inited() {
        return $this->inited;
    }

    public function initlize() {
        if ($this->inited) {
            return true;
        }

        $this->mysqli = @new mysqli($this->host, $this->user, $this->pwd, $this->db);

        /* check connection */
        //if ($this->mysqli->connect_errorno) {
        if (mysqli_connect_errno()) {
            printf("Connect failed (%d): %s\n",mysqli_connect_errno(), mysqli_connect_error() );
            return false;
            //exit(0);
        }

        $this->mysqli->query('set names utf8;');

        /* check connection */
        if ($this->mysqli->connect_errno) {
            printf("Connect failed: %s\n", $mysqli->connect_error);
            return false;
        }

        $this->inited = true;
        return true;
    }

    public function _fetch($s_sql, $s_type)
    {
        if (!$this->inited && !$this->initlize()) {
            return false;
        }
        $result = $this->mysqli->query($s_sql);
        if (!$result) {
            fprintf(STDERR,  "fail to do query $s_sql\n");
        }
        $arr = array();
        switch ($s_type)
        {
        case 'cell':
            $arr = $result->fetch_array(MYSQLI_NUM);
            return $arr[0];
        case 'num':
            $arr = $result->fetch_array(MYSQLI_NUM);
            break;
        case 'assoc' :
            $arr = $result->fetch_array(MYSQLI_ASSOC);
            break;
        case 'assoc_all' :
            $tbl = array();
            while ($tmp = $result->fetch_array(MYSQLI_ASSOC)) {
                $tbl[] = $tmp;
            }
            $result->free();
            return $tbl;
        case 'all' :
        default:
            $tbl = array();
            while ($tmp = $result->fetch_array(MYSQLI_BOTH)) {
                $tbl[] = $tmp;
            }
            $result->free();
            return $tbl;
        }
        $result->free();
        return $arr;
    }

    /**
     * 查询记录
     *
     * @param $s_table
     * @param $condition
     * @param $s_field
     * @param $s_type
     * @param $s_orderby
     * @param $s_sort
     * @param $s_limit
     *
     * @return array|string
     */
    public function get($s_table, $condition = null, $s_field = '*', 
        $s_type = 'all', $s_orderby = false, $s_sort = false, 
        $s_limit = false, $s_groupby = false, $debug = false)   {
            if (!$this->inited && !$this->initlize()) {
                return false;
            }
            $s_sql  = "SELECT {$s_field} FROM {$s_table}";
            $s_sql .= ($condition) ? " Where $condition " : ' ';
            $s_sql .= ($s_groupby) ? " GROUP BY $s_groupby" : '';
            $s_sql .= ($s_orderby) ? " ORDER BY $s_orderby" : '';
            $s_sql .= ($s_sort) ? " $s_sort" : ' ';
            $s_sql .= ($s_limit) ? " LIMIT $s_limit;" : ';';
            $debug && print($s_sql. "\n");
            //type is all | assoc | num
            return $this->_fetch($s_sql, $s_type);
    }

    public function select_db($db) {
        if (!$this->inited && !$this->initlize()) {
            return false;
        }
        return $this->mysqli->select_db($db);
    }

    public function affected_rows() {
        return $this->mysqli->affected_rows;
    }

    public function exec_sql($sql) {
        if (!$this->inited && !$this->initlize()) {
            return false;
        }
        $ret = $this->mysqli->real_query($sql);
        return $ret;
    }

    public function exec_sql_rows($sql, &$rows) {
        if (!$this->inited && !$this->initlize()) {
            return false;
        }
        $ret = $this->mysqli->real_query($sql);
        if (!$ret) {
            fprintf(STDERR, "exec_sql_rows:: return false\n");
        }
        $rows = $this->mysqli->affected_rows;
        return $ret;
    }

    public function print_err() {
        fprintf(STDERR, "\nSQL Error %d: %s\n", $this->mysqli->errno, $this->mysqli->error);
    }

    public function get_err() {
        return "\nSQL Error ".$this->mysqli->errno.": ".$this->mysqli->error."\n";
    }

    public function close() {
        if ($this->inited) {
            $this->mysqli->close();
        }
    }
}

?>
