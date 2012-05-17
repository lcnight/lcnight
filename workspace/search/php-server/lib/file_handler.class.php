<?php
class file_handler
{
    /**
     * 文件路径
     * @access private
     * @var string
     */
    private $path;

    /**
     * 初始化
     * @access public
     * @param string $path
     * @return boolean
     */
    public function init($path)
    {
        if (!file_exists($path) && !touch($path))
        {
            return false;
        }

        $this->path = $path;
        return true;
    }

    /**
     * 写文件
     * @access public
     * @param string $buffer
     * @param string $mode
     * @param int   $offset
     * @return boolean
     */
    public function write($buffer, $offset = 0)
    {
        $fp = fopen($this->path, 'r+');
        if (false == $fp)
        {
            return false;
        }

        if (0 != fseek($fp, $offset, SEEK_SET))
        {
            return false;
        }

        $rwrite = fwrite($fp, $buffer, strlen($buffer));
        if (false === $rwrite || $rwrite != strlen($buffer))
        {
            return false;
        }

        return true;
    }

    public function read($offset, $length)
    {
        $fp = fopen($this->path, 'r');
        if (false == $fp)
        {
            return false;
        }

        if (0 != fseek($fp, $offset, SEEK_SET))
        {
            return false;
        }

        $rread = fread($fp, $length);

        return $rread;
    }
}
