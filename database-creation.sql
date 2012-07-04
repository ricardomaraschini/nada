CREATE DATABASE nagios_baseline;
USE nagios_baseline;

CREATE TABLE `history` (
  `command_line` varchar(500) DEFAULT NULL,
  `entry_time` datetime DEFAULT NULL,
  `value` float DEFAULT NULL,
  `metric` varchar(256) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8

ALTER TABLE command_date ADD INDEX(comand_line,entry_time);
