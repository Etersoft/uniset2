-- MySQL dump 9.10
--
-- Host: localhost    Database: Gidrograph
-- ------------------------------------------------------
-- Server version	4.0.18-log

--  Создание пользователя для dbadmin-а
GRANT SELECT,INSERT,UPDATE,DELETE,INDEX,LOCK TABLES,CREATE,DROP ON G19910B.* TO dbadmin@localhost IDENTIFIED BY 'dbadmin';
-- Создание пользователя для просмотра
GRANT SELECT ON G19910B.* TO dbreader@"%" IDENTIFIED BY 'dbreader';

--
-- Table structure for table `AnalogSensors`
--
DROP TABLE IF EXISTS AnalogSensors;
CREATE TABLE AnalogSensors (
  num int(11) NOT NULL auto_increment,
  node int(3) default NULL,
  id int(4) default NULL,
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  time_usec int(3) unsigned default '0',
  value int(6) default NULL,
  PRIMARY KEY  (num),
  KEY date (date,time,time_usec),
  KEY node (node,id)
) TYPE=MyISAM;


--
-- Table structure for table `DigitalSensors`
--
DROP TABLE IF EXISTS DigitalSensors;
CREATE TABLE DigitalSensors (
  num int(11) NOT NULL auto_increment,
  node int(3) default NULL,
  id int(4) default NULL,
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  time_usec int(3) unsigned default '0',
  state char(1) default NULL,
  confirm time NOT NULL default '00:00:00',
  PRIMARY KEY  (num),
  KEY date (date,time,time_usec),
  KEY node (node,id),
  KEY confirm(confirm)
) TYPE=MyISAM;


--
-- Table structure for table `EmergencyLog`
--
DROP TABLE IF EXISTS EmergencyLog;
CREATE TABLE EmergencyLog (
  num int(11) NOT NULL auto_increment,
  eid int(3) default NULL,
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  time_usec int(3) unsigned default '0',
  PRIMARY KEY  (num),
  KEY eid (eid,date,time,time_usec)
) TYPE=MyISAM;

--
-- Dumping data for table `EmergencyLog`
--


--
-- Table structure for table `EmergencyRecords`
--
DROP TABLE IF EXISTS EmergencyRecords;
CREATE TABLE EmergencyRecords (
  num int(11) NOT NULL auto_increment,
  enum int(11) default '-1',
  sid int(4) default NULL,
  time time NOT NULL default '00:00:00',
  time_usec int(3) unsigned default '0',
  value int(6) default NULL,
  PRIMARY KEY  (num),
  KEY enum (enum),
  KEY sid (sid,time,time_usec)
) TYPE=MyISAM;

--
-- Dumping data for table `EmergencyRecords`
--

--
-- Table structure for table `ObjectsMap`
--
DROP TABLE IF EXISTS ObjectsMap;
CREATE TABLE ObjectsMap (
  name varchar(80) NOT NULL default '',
  rep_name varchar(80) default NULL,
  id int(4) NOT NULL default '0',
  msg int(1) default 0,
  PRIMARY KEY  (id),
  KEY rep_name (rep_name),
  KEY msg (msg)
) TYPE=MyISAM;

--
-- Table structure for table `RSChannel`
--
DROP TABLE IF EXISTS RSChannel;
CREATE TABLE RSChannel (
  num int(11) NOT NULL auto_increment,
  node int(3) default NULL,
  channel int(2) default NULL,
  date date NOT NULL default '0000-00-00',
  time time NOT NULL default '00:00:00',
  time_usec int(8) unsigned default '0',
  query int(3) default NULL,
  errquery int(3) default NULL,
  notaskquery int(3) default NULL,
  PRIMARY KEY  (num),
  KEY node (node,channel),
  KEY time (time,date,time_usec)
) TYPE=MyISAM;


DROP TABLE IF EXISTS Network;
CREATE TABLE Network(
	num int(11) NOT NULL auto_increment,
	date date NOT NULL default '0000-00-00',
	time time NOT NULL default '00:00:00',
	time_usec int(8) unsigned default 0,  	
	master int(3) default NULL,
	slave int(3) default NULL,
	connection int(2) default NULL,
	PRIMARY KEY (num),
	KEY (master, slave, connection),
	KEY (time, date,time_usec)
) TYPE=MyISAM;

--
-- Table structure for table `SensorsThreshold`
--
DROP TABLE IF EXISTS SensorsThreshold;
CREATE TABLE SensorsThreshold (
  sid int(11) NOT NULL default '0',
  alarm int(8) NOT NULL default '0',
  warning int(8) NOT NULL default '0'
) TYPE=MyISAM;


