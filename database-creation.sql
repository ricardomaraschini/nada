CREATE DATABASE nada;
USE nada;

CREATE TABLE `history` (
  `command_line` varchar(500) DEFAULT NULL,
  `entry_time` datetime DEFAULT NULL,
  `value` float DEFAULT NULL,
  `metric` varchar(256) DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

ALTER TABLE history ADD INDEX idx_command_time(command_line,entry_time);
