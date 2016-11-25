#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from pyUniSet import *
from pyUConnector import *
from pyUModbus import *
from UGlobal import *
from pyUExceptions import *

class UInterface():

    def __init__(self):

       self.itype = ""
       self.i = None
       self.ignore_nodes = False
       
    def set_ignore_nodes(self, state):
        self.ignore_nodes = state

    def getSupportedInterfaceList(self):
        return ["uniset","modbus"]

    def create_uniset_interface(self, xmlfile, params):
        self.i = UConnector(params,xmlfile)
        self.itype = "uniset"
        
    def create_modbus_interface(self):
        self.i = UModbus()
        self.itype = "modbus"

    def getIType(self):
        return self.itype

    # return [id,node,name]
    def getIDinfo(self, s_id):

        if self.itype == "uniset":
           return to_sid(s_id, self.i)

        if self.itype == "modbus":
           mbaddr,mbreg,mbfunc,nbit,vtype = get_mbquery_param(s_id,"0x04")
           return [str(mbreg),str(mbaddr),s_id]

        return ["","",""]

    # return [ RESULT, ERROR ]
    def validateParam(self, s_id):

        try:
            if self.itype == "uniset":
                s = to_sid(s_id, self.i)
                if s[0] == DefaultID:
                    return [False, "Unknown ID for '%s'"%str(s_id)]

                return [True,""]

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
                    return [False,', '.join(err)]

                return [True,""]

        except UException, e:
            return [False, "%s"%e.getError()]

        return [False, "Unknown interface %s" % self.itype]

    def getValue(self, s_id):

        try:
           if self.itype == "uniset":
              s = to_sid(s_id, self.i)
              if self.ignore_nodes == True:
                 s[1] = DefaultID
              
              return self.i.getValue( s[0], s[1] )

           if self.itype == "modbus":
              mbaddr, mbreg, mbfunc, nbit, vtype = get_mbquery_param(s_id, "0x04")
              if mbaddr == None or mbreg == None or mbfunc == None:
                 raise UValidateError( "(modbus:getValue): parse id='%s' failed. Must be 'mbreg@mbaddr:mbfunc:nbit:vtype'"%s_id )

              if self.i.isWriteFunction(mbfunc) == True:
                 raise UValidateError( "(modbus:getValue): for id='%s' mbfunc=%d is WriteFunction. Must be 'read'."%(s_id, mbfunc) )

              return self.i.mbread(mbaddr, mbreg, mbfunc, vtype, nbit)

        except UException, e:
              raise e

        raise UValidateError("(getValue): Unknown interface %s"%self.itype)

    def setValue(self, s_id, s_val, supplier = DefaultSupplerID ):
        try:
           if self.itype == "uniset":
              s = to_sid(s_id,self.i)
              if self.ignore_nodes == True:
                 s[1] = DefaultID

              self.i.setValue( s[0], s_val, s[1], supplier )
              return

           if self.itype == "modbus":
#             ip,port,mbaddr,mbreg,mbfunc,vtype,nbit = ui.get_modbus_param(s_id)
              mbaddr, mbreg, mbfunc, nbit, vtype = get_mbquery_param(s_id,"0x06")
              if mbaddr == None or mbreg == None or mbfunc == None:
                 raise UValidateError( "(modbus:setValue): parse id='%s' failed. Must be 'mbreg@mbaddr:mbfunc'"%s_id )
              
              #print "MODBUS SET VALUE: s_id=%s"%s_id
              if self.i.isWriteFunction(mbfunc) == False:
                 raise UValidateError( "(modbus:setValue): for id='%s' mbfunc=%d is NOT WriteFunction."%(s_id, mbfunc) )

              self.i.mbwrite(mbaddr, mbreg, to_int(s_val), mbfunc)
              return

        except UException, e:
              raise e

        raise UValidateError("(setValue): Unknown interface %s"%self.itype)

    def getConfFileName(self):
        if self.itype == "uniset":
           return self.i.getConfFileName()

        if self.itype == "modbus":
           return ""

        raise UValidateError("(setValue): Unknown interface %s"%self.itype)

    def getShortName(self, s_node):
        if self.itype == "uniset":
           return self.i.getShortName(s_node)

        if self.itype == "modbus":
           return ""

        raise UValidateError("(getShortName): Unknown interface %s"%self.itype)

    def getNodeID(self, s_node):
        if self.itype == "uniset":
           return self.i.getNodeID(s_node)

        if self.itype == "modbus":
           return None

        raise UValidateError("(getNodeID): Unknown interface %s"%self.itype)

    def getSensorID(self, s_name):
        if self.itype == "uniset":
           return self.i.getSensorID(s_name)

        if self.itype == "modbus":
           return None

        raise UValidateError("(getSensorID): Unknown interface %s"%self.itype)

    def getObjectID(self, o_name):
        if self.itype == "uniset":
           return self.i.getObjectID(o_name)

        if self.itype == "modbus":
           return None

        raise UValidateError("(getObjectID): Unknown interface %s"%self.itype)

    def getObjectInfo( self, o_name, params = "" ):
        '''
        get info from object
        :param o_name: [id | name@node | id@node]
        :param params: user parameters for getObjectInfo function
        :return: string
        '''

        if self.itype != "uniset":
            raise UException("(getObjectInfo): the interface does not support this feature..")

        s = to_sid(o_name,self.i)
        return self.i.getObjectInfo( s[0], params, s[1] )

    def getTimeChange( self, o_name ):
        '''
        :param o_name: [id | name@node | id@node]
        :return: UTypes::ShortIOInfo
        '''

        if self.itype != "uniset":
            raise UException("(getTimeChange): the interface does not support this feature..")

        s = to_sid(o_name,self.i)
        return self.i.getTimeChange( s[0], s[1] )