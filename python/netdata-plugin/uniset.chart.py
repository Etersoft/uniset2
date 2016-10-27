#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Description: uniset netdata python.d module
# Author: Pavel Vainerman (pv)

import os
import sys
import random
import uniset2
from uniset2 import UniXML
from uniset2 import UProxyObject
from uniset2 import UConnector

sys.path.append("./python_modules")
from base import SimpleService

NAME = os.path.basename(__file__).replace(".chart.py", "")

# default module values
update_every = 5
priority = 90000
retries = 60

class Service(SimpleService):

    sensors = []
    uproxy = None
    uconnector = None
    smOK = False

    def __init__(self, configuration=None, name=None):
        super(self.__class__, self).__init__(configuration=configuration, name=name)

        conf = self.configuration.pop
        confile = None
        try:
            confile = conf('confile')
        except KeyError:
            self.error("uniset plugin: Unknown uniset config file. Set 'confile: /xxx/zzz/xxxx.xml' in config")
            raise RuntimeError

        if not os.path.isfile(confile):
            self.error("uniset plugin: Not found confile '%s'"%confile)
            raise RuntimeError

        #self.name = self.get_conf_param('name', name)
        self.info("%s: read from %s"%(name,confile))
        self.create_charts(confile)
        self.init_uniset(confile)
        # добавляем датчики в опрос..
        for s in self.sensors:
            self.uproxy.addToAsk(s[1])

    def init_uniset(self, confile):

        arglist= uniset2.Params_inst()
        for i in range(0, len(sys.argv)):
            if i >= uniset2.Params.max:
                break;
            arglist.add(sys.argv[i])

        port = self.get_conf_param('port', '')
        if port != '':
            p = '--uniset-port'
            arglist.add_str(p)
            arglist.add_str( str(port) )

        uname = self.get_conf_param('uname', 'TestProc')

        try:
            self.uconnector = UConnector(arglist,confile)
            self.uproxy = UProxyObject(uname)
        except uniset2.UException, e:
            self.error("uniset plugin: error: %s"% e.getError())
            raise RuntimeError

    def get_conf_param(self, propname, defval):
        try:
            pop = self.configuration.pop
            return pop(propname)
        except KeyError:
            pass

        return defval

    def find_section(self, xml, secname):
        node = xml.findNode(xml.getDoc(), secname)[0]
        if node == None:
            self.error("not found '%s' section in %s" % (secname,xml.getFileName()))
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
                    self.error("IGNORE CHART.. Unknown id=''.")
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
            self.error("FAILED load xmlfile=%s err='%s'" % (confile, e.getError()))
            raise RuntimeError

    def build_lines(self, chart, lnode, xml, snode):

        node = xml.firstNode(lnode.children)

        while node != None:

            ff = node.prop('filter_field')
            fv = node.prop('filter_value')

            if ff == '' or ff == None:
                self.error("Unknown filter_fileld for chart id='%s'" % node.parent.prop('id'))
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
                self.error("Unknown 'id' for sensor %s" % node.prop('name'))
                raise RuntimeError

            params.append(id)
            params.append(self.get_param(node, 'netdata_name', node.prop('name')))
            params.append(self.get_param(node, 'netdata_alg', 'absolute'))
            # params.append(self.get_param(node, 'netdata_multiplier', None))
            # params.append(self.get_param(node, 'netdata_divisor', None))
            # params.append(self.get_param(node, 'netdata_hidden', None))

            self.sensors.append([id, uniset2.to_int(node.prop('id'))])

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

    def check(self):

        self.info("**** uniset_activate_objects")
        # ret = super(self.__class__, self).check()
        try:
            self.uconnector.activate_objects()
        except uniset2.UException, e:
            self.error("%s"% e.getError())
            raise False

        return True

    def _get_data(self):

        data = {}

        for netid,id in self.sensors:
            data[netid] = self.uproxy.getValue(id)

        if len(data) == 0:
            return None

        return data

    def update(self, interval):

        if not self.uproxy.askIsOK() and not self.uproxy.reaskSensors():
            return False

        prev_smOK = self.smOK
        self.smOK  = self.uproxy.smIsOK()

        if prev_smOK != self.smOK and self.smOK:
            self.info("SM exist OK. Reask sensors..")
            self.uproxy.reaskSensors()

        return super(self.__class__, self).update(interval)

if __name__ == "__main__":

    config = {}
    config['confile'] = './test.xml'
    config['port'] = 2809
    config['uname'] = 'TestProc'
    config['update_every'] = update_every
    config['priority'] = priority
    config['retries'] = retries

    serv = Service(config,"test")

    config2 = {}
    config2['confile'] = './test.xml'
    config2['port'] = 2809
    config2['uname'] = 'TestProc1'
    config2['update_every'] = update_every
    config2['priority'] = priority
    config2['retries'] = retries

    serv2 = Service(config2,"test")
