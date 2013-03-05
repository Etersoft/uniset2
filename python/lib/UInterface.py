#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

from pyUExceptions import *
from pyUConnector import *
from pyUModbus import *
from UGlobal import *

class UInterface():

    def __init__(self):

       self.utype = ""
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

    # return [id,node,name]
    def getIDinfo(self, s_id):

        if self.itype == "uniset":
           return to_sid(s_id,self.i)

        if self.itype == "modbus":
           mbaddr,mbreg,mbfunc,nbit,vtype = get_mbquery_param(s_id,"0x04")
           return [str(mbreg),str(mbaddr),s_id]

        return ["","",""]

    def getValue(self, s_id):

        try:
           if self.itype == "uniset":
              s = to_sid(s_id,self.i)
              if self.ignore_nodes == True:
                 s[1] = DefaultID
              
              return self.i.getValue(s[0],s[1])

           if self.itype == "modbus":
              mbaddr,mbreg,mbfunc,nbit,vtype = get_mbquery_param(s_id,"0x04")
              if mbaddr == None or mbreg == None or mbfunc == None:
                 raise UException( "(modbus:getValue): parse id='%s' failed. Must be 'mbreg@mbaddr:mbfunc:nbit:vtype'"%s_id )

              if self.i.isWriteFunction(mbfunc) == True:
                 raise UException( "(modbus:getValue): for id='%s' mbfunc=%d is WriteFunction. Must be 'read'."%(s_id,mbfunc) )

              return self.i.mbread(mbaddr,mbreg,mbfunc,vtype,nbit)

        except UException, e:
              raise e

        raise UException("(getValue): Unknown interface %s"%self.utype)

    def setValue(self, s_id, s_val):
        try:
           if self.itype == "uniset":
              s = to_sid(s_id,self.i)
              if self.ignore_nodes == True:
                 s[1] = DefaultID

              self.i.setValue(s[0],s_val,s[1])
              return

           if self.itype == "modbus":
#             ip,port,mbaddr,mbreg,mbfunc,vtype,nbit = ui.get_modbus_param(s_id)
              mbaddr,mbreg,mbfunc,nbit,vtype = get_mbquery_param(s_id,"0x06")
              if mbaddr == None or mbreg == None or mbfunc == None:
                 raise UException( "(modbus:setValue): parse id='%s' failed. Must be 'mbreg@mbaddr:mbfunc'"%s_id )
              
              #print "MODBUS SET VALUE: s_id=%s"%s_id
              if self.i.isWriteFunction(mbfunc) == False:
                 raise UException( "(modbus:setValue): for id='%s' mbfunc=%d is NOT WriteFunction."%(s_id,mbfunc) )

              self.i.mbwrite(mbaddr,mbreg,to_int(s_val),mbfunc)
              return

        except UException, e:
              raise e

        raise UException("(setValue): Unknown interface %s"%self.utype)

    def getConfFileName(self):
        if self.itype == "uniset":
           return self.i.getConfFileName()

        if self.itype == "modbus":
           return ""

        raise UException("(setValue): Unknown interface %s"%self.utype)
