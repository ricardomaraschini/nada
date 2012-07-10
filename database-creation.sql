CREATE DATABASE nada;
USE nada;

CREATE TABLE `commands` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `command_line` varchar(1024) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE `history` (
  `command_line_id` int(11) unsigned NOT NULL,
  `entry_time` datetime DEFAULT NULL,
  `value` float DEFAULT NULL,
  `metric` varchar(256) DEFAULT NULL,
  KEY `idx_command_time` (`command_line_id`,`entry_time`),
  CONSTRAINT `FK_commands` FOREIGN KEY (`command_line_id`) REFERENCES `commands` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
