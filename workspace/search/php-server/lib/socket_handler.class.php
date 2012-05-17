<?php
/**
 * socket类
 */
class socket_handler
{
    /**
     * socket实例
     * @access private
     * @var object
     */
    private $socket = NULL;

    /**
     * 连接
     * @access public
     * @param string $host
     * @param string $port
     * @return boolean
     */
    public function connect($host, $port)
    {
        /// 创建socket
        $this->socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
        if (false == $this->socket)
        {
            return false;
        }
        /// 链接主机
        if (!socket_connect($this->socket, $host, $port))
        {
            socket_close($this->socket);
            return false;
        }
        return true;
    }

    function __destruct()
    {
        $this->close();
    }

    /**
     * 关闭socket链接
     * @access public
     * @return NULL
     */
    public function close()
    {
        if (!empty($this->socket))
        {
            @socket_close($this->socket);
            $this->socket = NULL;
        }
    }

    /**
     * 发送数据
     * @access public
     * @param string $data
     * @return boolean
     */
    public function send($data)
    {
        $length = strlen($data);

        while (true) {
            $result = socket_write($this->socket, $data, $length);
            if ($result === false) {
                return false;
            }
            if ($result < $length) {
                $data = substr($data, $result);
                $length -= $result;
            } else {
                break;
            }
        }
        return true;
    }

    /**
     * 接收数据
     * @access public
     * @return false|string
     */
    public function receive()
    {
        $sock_data = ' ';

        while (true) {
            $data = socket_read($this->socket, 1024 * 16, PHP_BINARY_READ);
            if (false == $data || strlen($data) <= 0)
            {
                return false;
            }

            $sock_data .= $data;
            if (strstr($sock_data, "\r\n") !== false) {
                break;
            }
        }

        return $sock_data;
    }

    /**
     * 一步直接发送数据
     * @access public
     * @string $host
     * @string $port
     * @string $data
     * @return false|string
     */
    public function send2($host, $port, $data)
    {
        $result = $this->connect($host, $port);
        if (false === $result)
        {
            return false;
        }
        $result = $this->send($data);
        if (false === $result)
        {
            return false;
        }
        $result = $this->receive();
        if (false === $result)
        {
            return false;
        }
        $this->close();
        return $result;
    }
}
