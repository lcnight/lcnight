<?php
/** * 数据库出错处理函数
 *
 * @param Exception $e
 * @return NULL
 */
function db_error_handler($e)
{
    write_log('DB err:' . $e->getMessage());
}

/**
 * 根据key返回数据库连接
 *
 * @param string $key
 * @return false|object
 */
function get_db_conn($key)
{
    // 加载数据库配置
    static $db_cfg = NULL; if (NULL == $db_cfg)
    {
        $db_cfg = include('db_config.inc.php');
    }

    // 数据库连接数组变量
    static $conn = array();
    if (!isset($conn[$key]))
    {
        $dsn = WDB_PDO::build_dsn($db_cfg[$key]['name'], $db_cfg[$key]['host'], $db_cfg[$key]['port']);
        try
        {
            $conn[$key] = new WDB_PDO($dsn, $db_cfg[$key]['user'], $db_cfg[$key]['passwd']);
        }
        catch (Exception $e)
        {
            logger::write((string)$e);
            return false;
        }
    }

    if (!$conn[$key]->is_right_conn())
    {
        logger::write('fail to get db conn(' . $key . ')');
        return false;
    }

    return $conn[$key];
}

/**
 * db错误默认处理函数
 *
 * @param Exception $e
 * @return NULL
 */
function db_err_handler($e)
{
    logger::write($e);
}

function get_search_hints($keyword)
{
    $pkg = array('cmd_id' => GET_HINTS_CMD_ID,
                 'keyword' => $keyword);

    $send_json_pkg = json_encode($pkg) . "\r\n";

    $socket = new socket_handler();

    $result = $socket->send2(SEARCH_ENGINE_HOST, SEARCH_ENGINE_PORT, $send_json_pkg);
    if ($result == false) {
        return false;
    }

    return json_decode($result, true);
}

function get_search_recommends($keywords_vec)
{
    $pkg = array('cmd_id' => GET_RECOMMENDS_CMD_ID,
                 'keywords' => $keywords_vec);

    $send_json_pkg = json_encode($pkg) . "\r\n";

    $socket = new socket_handler();

    $result = $socket->send2(SEARCH_ENGINE_HOST, SEARCH_ENGINE_PORT, $send_json_pkg);
    if ($result == false) {
        return false;
    }

    return json_decode($result, true);
}

function get_search_results($keywords_vec, $page_num, $result_count)
{
    $pkg = array('cmd_id' => GET_SEARCH_CMD_ID,
                 'page_num' => $page_num,
                 'result_per_page' => $result_count,
                 'keywords' => $keywords_vec);

    $send_json_pkg = json_encode($pkg) . "\r\n";

    $socket = new socket_handler();
    $result = $socket->send2(SEARCH_ENGINE_HOST, SEARCH_ENGINE_PORT, $send_json_pkg);
    if ($result == false) {
        return false;
    }

    return json_decode($result, true);
}

function set_view_ts($did)
{
    $pdo_handler = get_db_conn('process_doc_db');
    if ($pdo_handler == false) {
        return false;
    }

    $sql = 'UPDATE t_doc SET view_ts = view_ts + 1 WHERE did = ' . $did;

    $data = $pdo_handler->execute($sql, WDB_PDO::SQL_RETURN_TYPE_EXEC);
    if ($data === false) {
        return false;
    }

    return true;
}

function set_vote_ts($did, $vote_rank)
{
    $pdo_handler = get_db_conn('process_doc_db');

    $sql = 'UPDATE t_doc SET vote_ts = vote_ts + ' . $vote_rank . ' WHERE did = '
        . $did;

    $data = $pdo_handler->execute($sql, WDB_PDO::SQL_RETURN_TYPE_EXEC);
    if ($data === false) {
        return false;
    }

    return true;
}

function get_hot_search_words()
{
    $redis_serv = array(
            'host' => REDIS_SERVER_HOST,
            'port' => REDIS_SERVER_PORT,
            'database' => REDIS_DATABASE
            );

    $redis = new Predis_Client($redis_serv);

    $result = $redis->zrevrange("search_words", 0, 9);

    return $result;
}

function json_err_return($status)
{
    $ret_pkg = array('status' => $status);
    echo json_encode($ret_pkg);
}

function json_empty_return()
{
    $ret_pkg = array('status' => 0);
    echo json_encode($ret_pkg);
}
