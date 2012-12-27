#!/bin/sh

dbname=test.db

[ -n "$1" ] && dbname="$1"

sqlite3 $dbname <<"_EOF_"
DROP TABLE IF EXISTS main_history;
CREATE TABLE main_history (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  date date NOT NULL,
  time time NOT NULL,
  time_usec INTEGER NOT NULL,
  sensor_id INTEGER NOT NULL,
  value DOUBLE NOT NULL,
  node INTEGER NOT NULL,
  confirm INTEGER DEFAULT NULL
);

INSERT INTO main_history VALUES(NULL,0,0,0,100,20.3,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,101,20.65,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,102,20.7,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,103,20.1,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,105,20.3,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,106,20.65,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,107,20233.7,1,0);
INSERT INTO main_history VALUES(NULL,0,0,0,108,245560.67671,1,0);

DROP TABLE IF EXISTS `main_emergencylog`;
CREATE TABLE `main_emergencylog` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` INTEGER(5) NOT NULL,
  `type_id` INTEGER(5) NOT NULL,
  CONSTRAINT `type_id_refs_id_a3133ca` FOREIGN KEY (`type_id`) REFERENCES `main_emergencytype` (`id`)
);




DROP TABLE IF EXISTS `main_emergencyrecords`;
CREATE TABLE `main_emergencyrecords` (
  `id` INTEGER PRIMARY KEY AUTOINCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` INTEGER(10) NOT NULL,
  `log_id` INTEGER(11) NOT NULL,
  `sensor_id` INTEGER(10) NOT NULL,
  `value` double NOT NULL,
  CONSTRAINT `log_id_refs_id_77a37ea9` FOREIGN KEY (`log_id`) REFERENCES `main_emergencylog` (`id`),
  CONSTRAINT `sensor_id_refs_id_436bab5e` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
);


_EOF_

# 
# KEY `main_emergencyrecords_log_id` (`log_id`),
# KEY `main_emergencyrecords_sensor_id` (`sensor_id`),

# KEY main_history_sensor_id (sensor_id)
