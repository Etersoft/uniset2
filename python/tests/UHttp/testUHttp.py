#!/usr/bin/env python
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

        print shm.consumers()

    except UHTTPError, e:
        print e.message
