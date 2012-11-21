-- MySQL dump 10.13  Distrib 5.5.28, for Linux (i686)
--
-- Host: localhost    Database: ueps_control
-- ------------------------------------------------------
-- Server version	5.5.28-alt6

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `main_actionmessages`
--

DROP TABLE IF EXISTS `main_actionmessages`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_actionmessages` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `sensor_id` int(10) unsigned NOT NULL,
  `text` varchar(256) NOT NULL,
  `atype` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_actionmessages_sensor_id` (`sensor_id`),
  CONSTRAINT `sensor_id_refs_id_161082b` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_actionmessages`
--

LOCK TABLES `main_actionmessages` WRITE;
/*!40000 ALTER TABLE `main_actionmessages` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_actionmessages` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_emergencylog`
--

DROP TABLE IF EXISTS `main_emergencylog`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_emergencylog` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` int(10) unsigned NOT NULL,
  `type_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_emergencylog_type_id` (`type_id`),
  CONSTRAINT `type_id_refs_id_a3133ca` FOREIGN KEY (`type_id`) REFERENCES `main_emergencytype` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_emergencylog`
--

LOCK TABLES `main_emergencylog` WRITE;
/*!40000 ALTER TABLE `main_emergencylog` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_emergencylog` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_emergencyrecords`
--

DROP TABLE IF EXISTS `main_emergencyrecords`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_emergencyrecords` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` int(10) unsigned NOT NULL,
  `log_id` int(11) NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  `value` double NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_emergencyrecords_log_id` (`log_id`),
  KEY `main_emergencyrecords_sensor_id` (`sensor_id`),
  CONSTRAINT `log_id_refs_id_77a37ea9` FOREIGN KEY (`log_id`) REFERENCES `main_emergencylog` (`id`),
  CONSTRAINT `sensor_id_refs_id_436bab5e` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_emergencyrecords`
--

LOCK TABLES `main_emergencyrecords` WRITE;
/*!40000 ALTER TABLE `main_emergencyrecords` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_emergencyrecords` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_emergencytype`
--

DROP TABLE IF EXISTS `main_emergencytype`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_emergencytype` (
  `id` int(10) unsigned NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  `fuse_value` double NOT NULL,
  `fuse_invert` int(11) NOT NULL,
  `size` int(11) NOT NULL,
  `savetime` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_emergencytype_sensor_id` (`sensor_id`),
  CONSTRAINT `sensor_id_refs_id_23cd955f` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_emergencytype`
--

LOCK TABLES `main_emergencytype` WRITE;
/*!40000 ALTER TABLE `main_emergencytype` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_emergencytype` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_emergencytype_filter_on`
--

DROP TABLE IF EXISTS `main_emergencytype_filter_on`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_emergencytype_filter_on` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `emergencytype_id` int(10) unsigned NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `emergencytype_id` (`emergencytype_id`,`sensor_id`),
  KEY `sensor_id_refs_id_55cdefec` (`sensor_id`),
  CONSTRAINT `emergencytype_id_refs_id_3175ff98` FOREIGN KEY (`emergencytype_id`) REFERENCES `main_emergencytype` (`id`),
  CONSTRAINT `sensor_id_refs_id_55cdefec` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_emergencytype_filter_on`
--

LOCK TABLES `main_emergencytype_filter_on` WRITE;
/*!40000 ALTER TABLE `main_emergencytype_filter_on` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_emergencytype_filter_on` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_history`
--

DROP TABLE IF EXISTS `main_history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_history` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` date NOT NULL,
  `time` time NOT NULL,
  `time_usec` int(10) unsigned NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  `value` double NOT NULL,
  `node` int(10) unsigned NOT NULL,
  `confirm` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `main_history_sensor_id` (`sensor_id`),
  CONSTRAINT `sensor_id_refs_id_3d679168` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_history`
--

LOCK TABLES `main_history` WRITE;
/*!40000 ALTER TABLE `main_history` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_history` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_object`
--

DROP TABLE IF EXISTS `main_object`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_object` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) NOT NULL,
  `textname` varchar(256) NOT NULL,
  `subsystem_id` int(11) DEFAULT NULL,
  `group_filter` varchar(256) NOT NULL,
  `group_filter_data` varchar(256) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `main_object_subsystem_id` (`subsystem_id`),
  CONSTRAINT `subsystem_id_refs_id_5e2b559c` FOREIGN KEY (`subsystem_id`) REFERENCES `main_subsystem` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=13 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `main_object_depends_on`
--

DROP TABLE IF EXISTS `main_object_depends_on`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_object_depends_on` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `from_object_id` int(11) NOT NULL,
  `to_object_id` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `from_object_id` (`from_object_id`,`to_object_id`),
  KEY `to_object_id_refs_id_75314740` (`to_object_id`),
  CONSTRAINT `from_object_id_refs_id_75314740` FOREIGN KEY (`from_object_id`) REFERENCES `main_object` (`id`),
  CONSTRAINT `to_object_id_refs_id_75314740` FOREIGN KEY (`to_object_id`) REFERENCES `main_object` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_object_depends_on`
--

LOCK TABLES `main_object_depends_on` WRITE;
/*!40000 ALTER TABLE `main_object_depends_on` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_object_depends_on` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_sensor`
--

DROP TABLE IF EXISTS `main_sensor`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_sensor` (
  `id` int(10) unsigned NOT NULL,
  `name` varchar(50) NOT NULL,
  `textname` varchar(256) NOT NULL,
  `type_id` int(11) DEFAULT NULL,
  `iotype` varchar(1) NOT NULL,
  `el_range` varchar(256) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_sensor_type_id` (`type_id`),
  CONSTRAINT `type_id_refs_id_41936a3b` FOREIGN KEY (`type_id`) REFERENCES `main_sensorstype` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `main_sensor_object`
--

DROP TABLE IF EXISTS `main_sensor_object`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_sensor_object` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `sensor_id` int(10) unsigned NOT NULL,
  `object_id` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `sensor_id` (`sensor_id`,`object_id`),
  KEY `object_id_refs_id_6b741c43` (`object_id`),
  CONSTRAINT `object_id_refs_id_6b741c43` FOREIGN KEY (`object_id`) REFERENCES `main_object` (`id`),
  CONSTRAINT `sensor_id_refs_id_6e8a4dc2` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_sensor_object`
--

LOCK TABLES `main_sensor_object` WRITE;
/*!40000 ALTER TABLE `main_sensor_object` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_sensor_object` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_sensormessages`
--

DROP TABLE IF EXISTS `main_sensormessages`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_sensormessages` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `sensor_id` int(10) unsigned NOT NULL,
  `value` double NOT NULL,
  `text` varchar(256) NOT NULL,
  `mtype` int(11) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `main_sensormessages_sensor_id` (`sensor_id`),
  CONSTRAINT `sensor_id_refs_id_475c9249` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `main_sensorstype`
--

DROP TABLE IF EXISTS `main_sensorstype`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_sensorstype` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `textname` varchar(64) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_sensorstype`
--

LOCK TABLES `main_sensorstype` WRITE;
/*!40000 ALTER TABLE `main_sensorstype` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_sensorstype` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_set`
--

DROP TABLE IF EXISTS `main_set`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_set` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(256) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_set`
--

LOCK TABLES `main_set` WRITE;
/*!40000 ALTER TABLE `main_set` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_set` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_set_sensors`
--

DROP TABLE IF EXISTS `main_set_sensors`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_set_sensors` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `set_id` int(11) NOT NULL,
  `sensor_id` int(10) unsigned NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `set_id` (`set_id`,`sensor_id`),
  KEY `sensor_id_refs_id_300d9690` (`sensor_id`),
  CONSTRAINT `sensor_id_refs_id_300d9690` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`),
  CONSTRAINT `set_id_refs_id_6324f2eb` FOREIGN KEY (`set_id`) REFERENCES `main_set` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_set_sensors`
--

LOCK TABLES `main_set_sensors` WRITE;
/*!40000 ALTER TABLE `main_set_sensors` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_set_sensors` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_statnode`
--

DROP TABLE IF EXISTS `main_statnode`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_statnode` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `parent_id` int(11) DEFAULT NULL,
  `name` varchar(256) NOT NULL,
  `type` varchar(1) NOT NULL,
  `object_id` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `main_statnode_parent_id` (`parent_id`),
  KEY `main_statnode_object_id` (`object_id`),
  CONSTRAINT `object_id_refs_id_3cdd9396` FOREIGN KEY (`object_id`) REFERENCES `main_object` (`id`),
  CONSTRAINT `parent_id_refs_id_34acb125` FOREIGN KEY (`parent_id`) REFERENCES `main_statnode` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_statnode`
--

LOCK TABLES `main_statnode` WRITE;
/*!40000 ALTER TABLE `main_statnode` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_statnode` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_statsensor`
--

DROP TABLE IF EXISTS `main_statsensor`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_statsensor` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(256) NOT NULL,
  `node_id` int(11) NOT NULL,
  `sensor_id` int(10) unsigned DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `main_statsensor_node_id` (`node_id`),
  KEY `main_statsensor_sensor_id` (`sensor_id`),
  CONSTRAINT `node_id_refs_id_3d620b87` FOREIGN KEY (`node_id`) REFERENCES `main_statnode` (`id`),
  CONSTRAINT `sensor_id_refs_id_7ca5983d` FOREIGN KEY (`sensor_id`) REFERENCES `main_sensor` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `main_statsensor`
--

LOCK TABLES `main_statsensor` WRITE;
/*!40000 ALTER TABLE `main_statsensor` DISABLE KEYS */;
/*!40000 ALTER TABLE `main_statsensor` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `main_subsystem`
--

DROP TABLE IF EXISTS `main_subsystem`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `main_subsystem` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) NOT NULL,
  `textname` varchar(256) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2012-11-21 10:14:34
