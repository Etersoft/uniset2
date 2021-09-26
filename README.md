UniSet ![Gihub testsuite Status](https://github.com/Etersoft/uniset2/actions/workflows/testsuite.yml/badge.svg) [![Build Status](https://travis-ci.org/Etersoft/uniset2.svg?branch=master)](https://travis-ci.org/Etersoft/uniset2) [![Coverity Scan Build Status](https://scan.coverity.com/projects/etersoft-uniset2/badge.svg)](https://scan.coverity.com/projects/etersoft-uniset2)
======

UniSet is a library for distributed control systems development.
There are set of base components to construct this kind of systems:
* base interfaces for your implementation of control algorithms.
* algorithms for a discrete and analog input/output based on COMEDI interface.
* IPC mechanism based on CORBA (omniORB).
* logging system based on MySQL, SQLite, PostgreSQL databases.
* logging to TSDB (influxdb, opentsdb)
* logging to RRD
* supported MQTT (libmosquittopp)
* fast network protocol based on udp (UNet)
* Web interface to display logging and statistic information.
* utilities for system's configuration based on XML.
* python interface
* go interface (experimental)
* REST API

UniSet have been written in C++ and IDL languages but you can use another languages in your
add-on components. The main principle of the UniSet library's design is a maximum integration
with open source third-party libraries. UniSet provides the consistent interface for all
add-on components and third-party libraries. Python wrapper helps in using the library
in python scripts.

libuniset requires minimum C++11 (without pqxx support)
libuniset requires C++17 for pqxx support

More information:
* [RU] https://habr.com/post/278535/
* [RU] https://habr.com/post/171711/
* [RU] https://wiki.etersoft.ru/UniSet2/docs/

periodically checked by [PVS-Studio Analyzer](https://www.viva64.com/en/pvs-studio/)
