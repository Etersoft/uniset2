#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

sys.path.append('../../')
sys.path.append('../../lib/pyUniSet/')
sys.path.append('../../lib/pyUniSet/.libs')

from lib import *

def myfunc( act ):
	act['test'] = 'test'
	act['result'] = True
	return True


if __name__ == "__main__":

	lst = Params_inst()
	lst2 = Params_inst()

	for i in range(0, len(sys.argv)):
		if i >= Params.max:
			break;

		lst.add( sys.argv[i] )
		lst2.add( sys.argv[i] )


	pstr = "--uniset-port"
	pstr2= "12"
	lst.add_str(pstr)
	lst.add_str(pstr2)
	lst2.add_str(pstr)
	lst2.add_str(pstr2)

	p = []
	print("lst: class: " + str(p.__class__.__name__))

	try:
		uc1 = UConnector( lst, "test.xml" )
#		uc2 = UConnector( lst2, "test.xml" )

		obj1 = UProxyObject("TestProc")
		obj1.addToAsk(10)
		#obj2 = UProxyObject("TestProc1")

		uc1.activate_objects()

		while True:
			print("getValue(10): %d" % obj1.getValue(10))
			time.sleep(1)

#		print "Info: %s"%uc1.getObjectInfo("TestProc1","")

		print("apiRequest: %s"%uc1.apiRequest(uc1.getObjectID("SharedMemory"),"get?10"))

#		tc = uc1.getTimeChange(2)
#		print "TimeChange: %s  sup=%d"%(tc.value,tc.supplier)
#		print "(0)UIType: %s" % uc1.getUIType()

		print("(1)getShortName: id=%d name=%s" % (1, uc1.getShortName(1)))
		print("(2)getObjectID('TestProc'): %d" % uc1.getObjectID("TestProc"))

#		print "     getName: id=%d name=%s" % (1, uc1.getName(101))
#		print " getTextName: id=%d name=%s" % (1, uc1.getTextName(101))
#		print "\n"
#		print "getShortName: id=%d name=%s" % (2, uc1.getShortName(109))
#		print "     getName: id=%d name=%s" % (2, uc1.getName(109))
#		print " getTextName: id=%d name=%s" % (2, uc1.getTextName(109))

		try:
			print("(1)setValue: %d=%d" % (3,22))
			uc1.setValue(3,22,DefaultID)
		except UException as e:
			print("(1)setValue exception: " + str(e.getError()))

		try:
			print("(2)getValue: %d=%d" % ( 3, uc1.getValue(3,DefaultID) ))
		except UException as e:
			print("(2)getValue exception: " + str(e.getError()))

		try:
			print("(3)getValue: %d=%d" % ( 100, uc1.getValue(100,DefaultID) ))
		except UException as e:
			print("(3)getValue exception: " + str(e.getError()))

	except UException as e:
		print("(testUI): catch exception: " + str(e.getError()))
