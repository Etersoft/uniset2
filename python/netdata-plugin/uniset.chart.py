#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Description: uniset netdata python.d module
# Author: Pavel Vainerman (pv)

import os
import sys

sys.path.append("./python_modules")

import random
import uniset2
from uniset2 import UniXML
from base import SimpleService

NAME = os.path.basename(__file__).replace(".chart.py", "")

# default module values
update_every = 5
priority = 90000
retries = 60

class Service(SimpleService):

    sensors = []

    def __init__(self, configuration=None, name=None):
        super(self.__class__, self).__init__(configuration=configuration, name=name)

        # if 'confile' not in self.configuration:
        #     self.error("uniset plugin: Unknown confile..")
        #     raise RuntimeError

        self.info("uniset plugin: read from /home/pv/Projects.com/uniset-2.0/conf/test.xml")
        self.create_charts("/home/pv/Projects.com/uniset-2.0/conf/test.xml")

# class Service():
#
#     sensors = []
#
#     def error(self,txt):
#         print txt
#
#     def __init__(self, configuration=None, name=None):
#         self.create_charts("./test.xml")

    def find_section(self, xml, secname):
        node = xml.findNode(xml.getDoc(), secname)[0]
        if node == None:
            self.error("uniset plugin: not found %s section" % secname)
            raise RuntimeError

        return node.children

    def create_charts(self,confile):
        try:
            xml = UniXML(confile)
            snode = self.find_section(xml, "sensors")
            cnode = self.find_section(xml, "netdata")

            myCharts = {}
            myOrder = []

            node = xml.firstNode(cnode)

            while node != None:

                # CHART type.id name title units [family [context [charttype [priority [update_every]]]]]
                id = node.prop('id')
                if id == '' or id == None:
                    self.error("uniset plugin: IGNORE CHART.. Unknown id=''.")
                    node = xml.nextNode(node)
                    continue

                chart = {}
                options = []
                #options.append(node.prop('id'))
                options.append(None)
                options.append(node.prop('name'))
                options.append(node.prop('units'))
                options.append(node.prop('title'))
                options.append(node.prop('family'))
                # options.append(node.prop('context'))
                options.append(self.get_param(node, 'charttype', 'area'))
                # options.append(node.prop('priority'))
                # options.append(node.prop('update_every'))
                chart['options'] = options
                chart['lines'] = []

                self.build_lines(chart, node, xml, snode)

                if len(chart['lines']) > 0:
                    myOrder.append(id)
                    myCharts[id] = chart

                node = xml.nextNode(node)

            self.definitions = myCharts
            self.order = myOrder

        except uniset2.UniXMLException, e:
            self.error("uniset plugin: FAILED load xmlfile=%s err='%s'" % (confile, e.getError()))
            raise RuntimeError

    def build_lines(self, chart, lnode, xml, snode):

        node = xml.firstNode(lnode.children)

        while node != None:

            ff = node.prop('filter_field')
            fv = node.prop('filter_value')

            if ff == '' or ff == None:
                self.error("uniset plugin: Unknown filter_fileld for chart id='%s'" % node.parent.prop('id'))
                raise RuntimeError

            self.read_sensors(snode,ff,fv,xml, chart)
            node = xml.nextNode(node)

    def read_sensors(self, snode, ff, fv, xml, chart ):

        node = xml.firstNode(snode)
        while node != None:

            if self.check_filter(node, ff, fv) == False:
                node = xml.nextNode(node)
                continue

            # DIMENSION id[name[algorithm[multiplier[divisor[hidden]]]]]
            # Параметры берутся из <sensors> по полям netdata_xxx (необязательные помечены []
            # В качестве id - берётся name.
            # В качестве name - берётся name, если не указан netdata_name=''

            params = []
            id = self.get_param(node, 'netdata_id', node.prop('name'))
            if id == '' or id == None:
                self.error("uniset plugin: Unknown 'id' for sensor %s" % node.prop('name'))
                raise RuntimeError

            params.append(id)
            params.append(self.get_param(node, 'netdata_name', node.prop('name')))
            params.append(self.get_param(node, 'netdata_alg', 'absolute'))
            # params.append(self.get_param(node, 'netdata_multiplier', None))
            # params.append(self.get_param(node, 'netdata_divisor', None))
            # params.append(self.get_param(node, 'netdata_hidden', None))

            self.sensors.append([id,node.prop('id')])

            chart['lines'].append(params)

            node = xml.nextNode(node)

    def get_param(self,node,name,defval):

        val = node.prop(name)
        if val != None and len(val) > 0:
            return val

        return defval

    def check_filter(self, node, ff, fv ):

        if ff == None:
            return False

        if ff == '':
            return True;

        if fv == '' and node.prop(ff) == '':
            return False

        if fv != '' and node.prop(ff) != fv:
            return False

        return True

    def _get_data(self):
        data = {}

        for netid,id in self.sensors:
            data[netid] = random.randint(0, 100)

        if len(data) == 0:
            return None

        return data

# if __name__ == "__main__":
#
#     serv = Service(None,"test")