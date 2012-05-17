<?php
define('VERSION','dev');
if (VERSION == 'release')
{
    define('SEARCH_ENGINE_HOST', '10.1.1.60');
    define('SEARCH_ENGINE_PORT', '12601');

    define('REDIS_SERVER_HOST', '10.1.1.60');
    define('REDIS_SERVER_PORT', '6379');
    define('REDIS_DATABASE',    '2');
}
else
{
    define('SEARCH_ENGINE_HOST', '10.1.1.60');
    define('SEARCH_ENGINE_PORT', '12601');

    define('REDIS_SERVER_HOST', '10.1.1.60');
    define('REDIS_SERVER_PORT', '6379');
    define('REDIS_DATABASE',    '2');
}

define('DS', DIRECTORY_SEPARATOR);
define('LOG_DIR',      dirname(dirname(__FILE__)) . DS . 'logs' . DS);
define('LIB_DIR',      dirname(dirname(__FILE__)) . DS . 'lib' . DS);
define('CONFIG_DIR',   dirname(__FILE__) . DS);
define('SCRIPT_PATH',     dirname(dirname(__FILE__)) . DS . 'script' . DS);
define('DEFAULT_DBCHARSET', 'utf8');
//define('DICT_PATH',     dirname(dirname(__FILE__)) . DS . 'etc' . DS . 'dict.xdb');
//define('RULES_PATH',     dirname(dirname(__FILE__)) . DS . 'etc' . DS . 'rules.ini');


define('GET_HINTS_CMD_ID',        2001);
define('GET_RECOMMENDS_CMD_ID',   2002);
define('GET_SEARCH_CMD_ID',       2003);

define('SET_VIEW_TS_CMD_ID',      3001);
define('SET_VOTE_TS_CMD_ID',      3002);
define('GET_HOT_KEYWORDS_CMD_ID', 3003);


define('SEARCH_ENGINE_ERR',       1000);
define('SEARCH_ENGINE_NOFIND_HINTS', 1001);
define('SEARCH_ENGINE_NOFIND_RECOMMEND', 1002);
define('SEARCH_ENGINE_NOFIND_SEARCH',    1003);
define('SEARCH_ENGINE_NOFIND_HOTSEARCH', 1004);
define('PARAMETER_ERROR',         1005);

