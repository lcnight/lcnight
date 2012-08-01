<?php
function send_data($socket, $pack)
{
    while(strlen($pack) > 0)
    {
        $len = socket_write($socket, $pack, strlen($pack));
        if ($len === false)
        {
            if (socket_last_error($socket) == SOCKET_EINTR)
            {
                continue;
            }
            else
            {
                log::write("socket_write fail: reason: ".socket_strerror(socket_last_error($socket)). "\n", "error");
                return -1;
            }
        }
        $pack = substr($pack, $len); 
    }
    return 0;
}

function send_data_and_nonblock($socket, $pack, $timeout)
{
    $s_time = time();
    while(strlen($pack) > 0)
    {
        $len = socket_write($socket, $pack, strlen($pack));
        if ($len === false)
        {
            if (socket_last_error($socket) == SOCKET_EINTR || socket_last_error($socket) == SOCKET_EAGAIN)
            {
                usleep(1);
                continue;
            }
            else
            {
                log::write("socket_write fail: reason: ".socket_strerror(socket_last_error($socket)), "warn");
                socket_close($socket);
                return -1;
            }
        }
        else
        {
            $pack = substr($pack, $len); 
            $s_time = time();
        }

        if (time() - $s_time > $timeout)
        {
            log::write("timeout send_data_nonblock","warn");            
            socket_close($socket);
            return -1;
        }
        usleep(100);
    }
    return 0;
}

function recv_data_and_nonblock($socket, $pack_size, &$pack, $timeout)
{
    $s_time = time();
    while(strlen($pack) < $pack_size)
    {
        $recv_data = socket_read($socket, 4096);
        if ($recv_data === false)
        {
            if (socket_last_error($socket) == SOCKET_EINTR || socket_last_error($socket) == SOCKET_EAGAIN)
            {
                usleep(1);
                continue;
            }
            else
            {
                log::write("socket_read fail:reason: ".socket_strerror(socket_last_error($socket)),"warn");
                socket_close($socket);
                return -1;
            }
        } 
        else if ($recv_data == "")
        {
            log::write("socket_read zero bytes:reason:".socket_strerror(socket_last_error($socket)), "warn");
            socket_close($socket);
            return -1;
        }
        else
        {
            $pack .= $recv_data;
            $s_time = time();
        }

        if (time() - $s_time > $timeout)
        {
            log::write("timeout send_data_nonblock","warn");            
            socket_close($socket);
            return -1;
        }
        usleep(100);
    } 
    return 0;
}

function recv_data($socket, $pack_size, &$pack)
{
    while(strlen($pack) < $pack_size)
    {
        $recv_data = socket_read($socket, 4096);
        if ($recv_data === false)
        {
            if (socket_last_error($socket) == SOCKET_EINTR)
            {
                continue;
            }
            else
            {
                log::write("socket_read fail:reason: ".socket_strerror(socket_last_error($socket)),"error");
                return -1;
            }
        } 
        if ($recv_data == "")
        {
            log::write("socket_read zero bytes:reason:".socket_strerror(socket_last_error($socket)), "warn");
            return -2;
        }
        $pack .= $recv_data;
    } 
    return 0;
}

function init_connect_and_nonblock($ip, $port, &$socket)
{
    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($socket === false)
    {
        log::write("socket_create fail: reason: ".socket_strerror(socket_last_error()),"warn"); 
        socket_close($socket);
        return -1;
    }
    $result = socket_connect($socket, $ip, $port);
    if ($result === false)
    {
        log::write("socket_connect() fail:reason: ".socket_strerror(socket_last_error($socket)),"warn");
        socket_close($socket);
        return -1;
    } 
    socket_set_nonblock($socket);
    return 0;
}

function init_connect($ip, $port, &$socket)
{
    $socket = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if ($socket === false)
    {
        log::write("socket_create fail: reason: ".socket_strerror(socket_last_error()),"error"); 
        return -1;
    }
    $result = socket_connect($socket, $ip, $port);
    if ($result === false)
    {
        log::write("socket_connect() fail:reason: ".socket_strerror(socket_last_error($socket)),"error");
        return -1;
    } 
    return 0;
}

