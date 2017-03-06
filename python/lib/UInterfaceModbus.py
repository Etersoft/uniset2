#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from UInterface import *
from UGlobal import *
from pyUModbus import *


class UInterfaceModbus(UInterface):
    def __init__(self, ip, port):
        UInterface.__init__(self)

        self.itype = "modbus"
        self.i = UModbus()
        self.i.prepare(ip,port)

    # return [id,node,name]
    def getIDinfo(self, s_id):

        mbaddr, mbreg, mbfunc, nbit, vtype = get_mbquery_param(s_id, "0x04")
        return [str(mbreg), str(mbaddr), s_id]

    # return [ RESULT, ERROR ]
    def validateParam(self, s_id):

        try:
            if self.itype == "modbus":
                mbaddr, mbreg, mbfunc, nbit, vtype = get_mbquery_param(s_id, "0x04")
                err = []
                if mbaddr == None:
                    err.append("Unknown mbaddr")
                if mbfunc == None:
                    err.append("Unknown mbfunc")
                if mbreg == None:
                    err.append("Unknown mbreg")

                if len(err) > 0:
                    return [False, ', '.join(err)]

                return [True, ""]

        except UException, e:
            return [False, "%s" % e.getError()]

    def getValue(self, s_id):

        try:
            mbaddr, mbreg, mbfunc, nbit, vtype = get_mbquery_param(s_id, "0x04")
            if mbaddr == None or mbreg == None or mbfunc == None:
                raise UValidateError(
                    "(modbus:getValue): parse id='%s' failed. Must be 'mbreg@mbaddr:mbfunc:nbit:vtype'" % s_id)

            if self.i.isWriteFunction(mbfunc) == True:
                raise UValidateError(
                    "(modbus:getValue): for id='%s' mbfunc=%d is WriteFunction. Must be 'read'." % (s_id, mbfunc))

            return self.i.mbread(mbaddr, mbreg, mbfunc, vtype, nbit)

        except UException, e:
            raise e

    def setValue(self, s_id, s_val, supplier=DefaultSupplerID):
        try:
            # ip,port,mbaddr,mbreg,mbfunc,vtype,nbit = ui.get_modbus_param(s_id)
            mbaddr, mbreg, mbfunc, nbit, vtype = get_mbquery_param(s_id, "0x06")
            if mbaddr == None or mbreg == None or mbfunc == None:
                raise UValidateError(
                    "(modbus:setValue): parse id='%s' failed. Must be 'mbreg@mbaddr:mbfunc'" % s_id)

            # print "MODBUS SET VALUE: s_id=%s"%s_id
            if self.i.isWriteFunction(mbfunc) == False:
                raise UValidateError(
                    "(modbus:setValue): for id='%s' mbfunc=%d is NOT WriteFunction." % (s_id, mbfunc))

            self.i.mbwrite(mbaddr, mbreg, to_int(s_val), mbfunc)
            return

        except UException, e:
            raise e

    def getConfFileName(self):
        return ""

    def getShortName(self, s_node):
        return ""

    def getNodeID(self, s_node):
        return None

    def getSensorID(self, s_name):
        return None

    def getObjectID(self, o_name):
        return None
