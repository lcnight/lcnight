<?php
class db_retrier extends wdb_pdo
{
    private $dsn = '';
    private $user = '';
    private $pass = '';

    /**
     * 初始化
     * @access public
     * @param string $dsn
     * @param string $user
     * @param string $pass
     * @return boolean
     */
    public function init($dsn, $user, $pass)
    {
        try
        {
            $this->wdb_pdo = new WDB_PDO($dsn, $user, $pass);
            return true;
        }
        catch (Exception $e)
        {
            return false;
        }
    }

    public function execute()
    {

    }
}
