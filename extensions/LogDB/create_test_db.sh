#!/bin/sh

dbname=test.db

[ -n "$1" ] && dbname="$1"

sqlite3 $dbname <<"_EOF_"

PRAGMA foreign_keys=ON;

DROP TABLE IF EXISTS log;
CREATE TABLE log (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  tms timestamp KEY default (strftime('%s', 'now')),
  usec INTEGER(5) NOT NULL,
  name TEXT KEY NOT NULL,
  text TEXT
);

_EOF_
