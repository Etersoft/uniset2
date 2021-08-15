#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from UInterface import *
from UGlobal import *
from pyUConnector import *

class UInterfaceUniSet(UInterface):
    def __init__(self, xmlfile, params):
        UInterface.__init__(self)

        self.itype = "uniset"
        self.i = UConnector(params, xmlfile)

    # return [id,node,name]
    def getIDinfo(self, s_id):
        return to_sid(s_id, self.i)

    # return [ RESULT, ERROR ]
    def validateParam(self, s_id):

        try:
            s = to_sid(s_id, self.i)
            if s[0] == DefaultID:
                return [False, "Unknown ID for '%s'" % str(s_id)]

            return [True, ""]

        except UException as e:
            return [False, "%s" % e.getError()]

    def getValue(self, s_id):

        try:
            s = to_sid(s_id, self.i)
            if self.ignore_nodes == True:
                s[1] = DefaultID

            return self.i.getValue(s[0], s[1])

        except UException as e:
            raise e

    def setValue(self, s_id, s_val, supplier=DefaultSupplerID):
        try:
            s = to_sid(s_id, self.i)
            if self.ignore_nodes == True:
                s[1] = DefaultID

            self.i.setValue(s[0], s_val, s[1], supplier)
            return

        except UException as e:
            raise e

    def getConfFileName(self):

        return self.i.getConfFileName()

    def getShortName(self, s_node):
        return self.i.getShortName(s_node)

    def getNodeID(self, s_node):
        return self.i.getNodeID(s_node)

    def getSensorID(self, s_name):
        return self.i.getSensorID(s_name)

    def getObjectID(self, o_name):
        return self.i.getObjectID(o_name)

    def getObjectInfo(self, o_name, params=""):
        '''
        get information from object
        :param o_name: [id | name@node | id@node]
        :param params: user parameters for getObjectInfo function
        :return: string
        '''

        s = to_sid(o_name, self.i)
        return self.i.getObjectInfo(s[0], params, s[1])

    def getTimeChange(self, o_name):
        '''
        :param o_name: [id | name@node | id@node]
        :return: UTypes::ShortIOInfo
        '''

        s = to_sid(o_name, self.i)
        return self.i.getTimeChange(s[0], s[1])

    def apiRequest(self, o_name, query=""):
        '''
        call REST API for object
        :param o_name: [id | name@node | id@node]
        :param query: http query. example: /api/version/query_for_object?params или просто query_for_object?params
        :return: string [The default response format - json].
        '''
        s = to_sid(o_name, self.i)
        return self.i.apiRequest(s[0], query, s[1])
