-- MySQL Server version	5.0.51a-24+lenny5
--
-- Table structure for table `t_doc`
--

DROP TABLE IF EXISTS `t_doc`;
SET @saved_cs_client     = @@character_set_client;
SET character_set_client = utf8;
CREATE TABLE `t_doc` (
  `did` int(11) unsigned NOT NULL,
  `title` text,
  `content` text,
  `view_ts` int(11) unsigned default NULL COMMENT 'view times',
  `vote_ts` int(11) unsigned default NULL COMMENT 'user vote times',
  `c_time` int(11) unsigned default NULL COMMENT 'doc created time',
  `e_time` int(11) unsigned default NULL COMMENT 'doc last edit time',
  `i_time` int(11) unsigned default NULL COMMENT 'doc last index time',
  `d_flag` int(11) unsigned default '0' COMMENT 'doc deleted flag, default 0, not deleted',
  PRIMARY KEY  (`did`),
  KEY `id_time` (`did`,`e_time`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
SET character_set_client = @saved_cs_client;
