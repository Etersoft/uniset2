#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from uniset import *

if __name__ == "__main__":
	
	lst = Params_inst()

	for i in range(0, len(sys.argv)):
		if i >= Params.max:
			break;
	
		lst.add( sys.argv[i] )

	p = []
	print "lst: class: " + str(p.__class__.__name__)

	try:	
		uc1 = UConnector( lst, "test.xml" )
		
#		print "(0)UIType: %s" % uc1.getUIType()

		print "(1)getShortName: id=%d name=%s" % (1, uc1.getShortName(1))

#		print "     getName: id=%d name=%s" % (1, uc1.getName(101))
#		print " getTextName: id=%d name=%s" % (1, uc1.getTextName(101))
#		print "\n"
#		print "getShortName: id=%d name=%s" % (2, uc1.getShortName(109))
#		print "     getName: id=%d name=%s" % (2, uc1.getName(109))
#		print " getTextName: id=%d name=%s" % (2, uc1.getTextName(109))

		try:
			print "(1)setValue: %d=%d" % (3,22)
			uc1.setValue(3,22,DefaultID)
		except UException, e:
			print "(1)setValue exception: " + str(e.getError())

		try:
			print "(1)getValue: %d=%d" % ( 3, uc1.getValue(3,DefaultID) )
		except UException, e:
			print "(1)getValue exception: " + str(e.getError())

	except UException, e:
		print "(testUI): catch exception: " + str(e.getError())
