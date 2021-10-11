#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

sys.path.append('../../')
sys.path.append('../../lib/pyUniSet/')
sys.path.append('../../lib/pyUniSet/.libs')

from lib import *

if __name__ == "__main__":

    try:
        mb = UModbus()

        print("UIType: %s" % mb.getUIType())

        mb.connect("localhost",2048)
        try:
            print("Test READ functions...")
            for f in range(1,5):
                print("func=%d reg=%d" % (f,22))
                val = mb.mbread(0x01,22,f,"unsigned",-1)
#               val = mb.mbread(0x01,22)
                print("val=%d"%val)

            print("getWord: %d" % mb.getWord(0x01,22))
            print("getByte: %d" % mb.getByte(0x01,22))
            print("getBit: %d" % mb.getBit(0x01,22,3))
#            print ""
#            print "Test WRITE functions..."
#            for f in range(1,6):
#                print "func=%d reg=%d" % (f,22)
#                val = mb.getValue(f,0x01,22,"unsigned",-1)
        except UException as e:
            print("exception: " + str(e.getError()))

    except UException as e:
         print("(testUModbus): catch exception: " + str(e.getError()))
