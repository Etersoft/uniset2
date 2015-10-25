UniSet [![Build Status](https://travis-ci.org/Etersoft/uniset2.svg?branch=master)](https://travis-ci.org/Etersoft/uniset2) [![Coverity Scan Build Status](https://scan.coverity.com/projects/etersoft-uniset2/badge.svg)](https://scan.coverity.com/projects/etersoft-uniset2)
======

UniSet is a library for distributed control systems.
There are set of base components to construct this kind of systems:
* base interfaces for your implementation of control algorithms.
* algorithms for the discrete and analog input/output based on COMEDI interface.
* IPC mechanism based on CORBA (omniORB).
* logging system based on MySQL, SQLite, PostgreeSQL databases.
* Web interface to display logging and statistic information.
* utilities for system's configuration based on XML.

UniSet have been written in C++ and IDL languages but you can use another languages in your
add-on components. The main principle of the UniSet library's design is a maximum integration
with open source third-party libraries. UniSet provide the consistent interface for all
add-on components and third-party libraries. Python wrapper helps in using the library
in python scripts.

More information in Russian:
http://wiki.etersoft.ru/UniSet
