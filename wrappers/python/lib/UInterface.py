#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from .pyUniSet.pyUniSet import *


class UInterface():
    def __init__(self):
        self.itype = ""
        self.i = None
        self.ignore_nodes = False

    def set_ignore_nodes(self, state):
        self.ignore_nodes = state

    def getIType(self):
        return self.itype

    # return [id,node,name]
    def getIDinfo(self, s_id):
        return ["", "", ""]

    # return [ RESULT, ERROR ]
    def validateParam(self, s_id):
        return [False, "Unknown interface %s" % self.itype]

    def getValue(self, s_id):
        raise UValidateError("(getValue): Unknown interface %s" % self.itype)

    def setValue(self, s_id, s_val, supplier=DefaultSupplerID):
        raise UValidateError("(setValue): Unknown interface %s" % self.itype)

    def getConfFileName(self):
        raise UValidateError("(getConfFileName): Unknown interface %s" % self.itype)

    def getShortName(self, s_node):
        raise UValidateError("(getShortName): Unknown interface %s" % self.itype)

    def getNodeID(self, s_node):
        raise UValidateError("(getNodeID): Unknown interface %s" % self.itype)

    def getSensorID(self, s_name):
        raise UValidateError("(getSensorID): Unknown interface %s" % self.itype)

    def getObjectID(self, o_name):
        raise UValidateError("(getObjectID): Unknown interface %s" % self.itype)
