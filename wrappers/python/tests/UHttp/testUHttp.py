#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

sys.path.append('../../')
sys.path.append('../../lib')
sys.path.append('../../lib/pyUniSet/')
sys.path.append('../../lib/pyUniSet/.libs')

# from lib import *

from UniSetHTTP import *

if __name__ == "__main__":

    try:
        settings = {}
        settings['url'] = 'http://localhost:8080'
        settings['smID'] = 'SharedMemory'

        shm = SharedMemoryAPI(settings)

        # print shm.consumers()
        # print shm.get(sensors='10,12')
        # print shm.lost()
        # print shm.help()
        print(shm.sensors(0,10))


    except UHTTPError as e:
        print(e.message)
