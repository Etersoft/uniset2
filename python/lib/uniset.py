#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import os

_py_dir = os.path.dirname(os.path.abspath(os.path.normpath(__file__)))
_py_uniset_dir = os.path.normpath('%s/uniset' % _py_dir)

sys.path.append( _py_uniset_dir )
