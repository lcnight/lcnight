<?php
/**
 * 数据库配置文件
 */
if (VERSION == 'release')
{
	return array(
			    'process_doc_db'       => array(
				'host'      =>  '10.1.1.60',
				'port'      =>  3306,
				'user'      =>  'root',
				'passwd'    =>  'ta0mee',
				'name'      =>  'db_filter_doc'
				));
}
else
{
	return array(
			'process_doc_db'       => array(
				'host'      =>  '10.1.1.60',
				'port'      =>  3306,
				'user'      =>  'root',
				'passwd'    =>  'ta0mee',
				'name'      =>  'db_filter_doc'
				));
}
