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
_EOF_


#  KEY main_history_sensor_id (sensor_id)
