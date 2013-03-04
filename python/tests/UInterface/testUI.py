#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

sys.path.append('./.libs/')
sys.path.append('../../lib/pyUniSet/.libs/')
sys.path.append('../../lib/pyUniSet/')

from pyUniSet import *

if __name__ == "__main__":

	lst = Params_inst()

	for i in range(0, len(sys.argv)):
		if i >= Params.max:
			break;

		lst.add( sys.argv[i] )

	try:
		uniset_init_params( lst, str("test.xml") )


		print "getShortName: id=%d name=%s" % (1, getShortName(1))
		print "     getName: id=%d name=%s" % (1, getName(1))
		print " getTextName: id=%d name=%s" % (1, getTextName(1))
		print "\n"
		print "getShortName: id=%d name=%s" % (2, getShortName(2))
		print "     getName: id=%d name=%s" % (2, getName(2))
		print " getTextName: id=%d name=%s" % (2, getTextName(2))

		try:
			print "getValue: %d=%d" % ( 1, getValue(1) )
		except UException, e:
			print "getValue exception: " + str(e.getError())

		try:
			print "setValue: %d=%d" % (14,setValue(14,22))
		except UException, e:
			print "setValue exception: " + str(e.getError())

	except UException, e:
		print "(testUI): catch exception: " + str(e.getError())
