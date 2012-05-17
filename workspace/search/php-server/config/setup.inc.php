<?php
require 'define.inc.php';
require 'db_config.inc.php';
require LIB_DIR . 'wdb_pdo.class.php';
require LIB_DIR . 'logger.class.php';
require LIB_DIR . 'file_handler.class.php';
require LIB_DIR . 'socket_handler.class.php';
require 'function.php';
//require LIB_DIR . 'system_ctl.inc.php';
require LIB_DIR . 'Predis.php';
//require LIB_DIR . 'phpanalysis.class.php';
//require LIB_DIR . 'pscws4.class.php';

// 设置数据库错误处理模式
WDB_PDO::set_mode_debug();
// 设置数据库错误处理函数
WDB_PDO::set_error_handler_name('db_err_handler');
// 设置数据库处理字符集
WDB_PDO::set_charset(DEFAULT_DBCHARSET);
