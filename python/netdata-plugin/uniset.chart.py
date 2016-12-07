#!/usr/bin/env python
# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------------------------------------------
# Description: uniset netdata python.d module
# Author: Pavel Vainerman (pv)
# ----------------------------------------------------------------------------------------------------------
# Вариант на основе запуска uniset2-admin --сsv id1,id2,id3...
# ----------------------------------------------------------------------------------------------------------
# В configure.xml проекта ождается секция <netdata> описывающая
# charts и параметры.
# 		<netdata>
# 		<!-- https://github.com/firehol/netdata/wiki/External-Plugins -->
# 		<!-- CHART type.id name title units [family [context [charttype [priority [update_every]]]]] -->
# 		<chart id='unet' name='Temperature' title="Temperature" units='°С' context='' charttype='area' priority='' update_every=''>
#
# 			<!-- Чтобы иметь возможность выводить на одном чарте, разные группы можно указывать много 'line' -->
#
# 			<!-- Параметры берутся из <sensors> в виду netdata_xxx (необязательные помечены []
# 				В качестве id - берётся name.
# 				В качестве name - берётся name, если не указан netdata_name=''
# 			-->
# 			<!-- DIMENSION id [name [algorithm [multiplier [divisor [hidden]]]]] -->
# 			<lines filter_field='ndata' filter_value='temp'/>
# 		</chart>
# 	</netdata>
#
# TODO: можно сделать более оптимальный (по ресурсам) вариант со специальным UniSetNetDataServer (экономия на запуске admin каждые X секунд)
# ------------------------------------------------------------------------------------------------------------------
import os
import sys

import uniset2
from uniset2 import UniXML
from subprocess import Popen, PIPE

sys.path.append("./python_modules")
from base import ExecutableService

NAME = os.path.basename(__file__).replace(".chart.py", "")

# default module values
update_every = 5
priority = 90000
retries = 10000

class Service(ExecutableService):

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

        self.initOK = False
        self.sensors = []
        self.confile = confile
        self.info("%s: read from %s"%(name,confile))
        self.create_charts(confile)

        uniset_port = self.get_conf_param('port', '')

        idlist = ''
        num = 0
        for nid,sid in self.sensors:
            if num == 0:
                idlist="%s"%str(sid)
                num += 1
            else:
                idlist += ",%s"%str(sid)

        uniset_command = self.get_conf_param('uniset_command',"/usr/bin/uniset2-admin --confile %s"%(self.confile))

        command = "%s --csv %s"%(uniset_command,idlist)
        if uniset_port!=None and uniset_port!='':
            command += " --uniset-port %s"%uniset_port

        self.configuration['command'] = command
        self.command = command
        self.info("%s: Check %d sensors. Update command: %s"%(name,len(self.sensors),command))

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
            self.error("not found section <%s> in confile '%s'" % (secname,xml.getFileName()))
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
                    self.error("IGNORE CHART.. Unknown id=''")
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
            return True

        if fv == '' and node.prop(ff) == '':
            return False

        if fv != '' and node.prop(ff) != fv:
            return False

        return True

    def _get_data(self):

        data = {}

        try:
            raw = self._get_raw_data()

            if raw == None or len(raw) == 0:
                if self.initOK == True:
                    return {}

                # забиваем нулями.. (нужно подумать, правильно ли это, можно сделать defaultValue в настройках)
                self.initOK = True
                for id, sid in self.sensors:
                    data[id] = 0
                return data

            raw = raw[-1].split(',')

            if len(raw) < len(self.sensors):
                if len(raw) > 0:
                    self.debug("_get_data ERROR: len data=%d < len sensors=%d"%(len(raw),len(self.sensors)))
                return {}

            i = 0
            for id,sid in self.sensors:
                data[id] = raw[i]
                i += 1

        except (ValueError, AttributeError):
            return {}

        return data

    def _get_raw_data(self):
        """
        Get raw data from executed command
        :return: str
        """
        try:
            p = Popen(self.command, shell=True, stdout=PIPE, stderr=PIPE)
        except Exception as e:
            self.error("Executing command", self.command, "resulted in error:", str(e))
            return None

        data = []
        for line in p.stdout.readlines():
            data.append(str(line.decode()))

        if len(data) == 0:
            # self.error("No data collected.")
            return None

        return data

    def check(self):
        # Считаем что у нас всегда, всё хорошо, даже если SM недоступна
        return True

    # def update(self, interval):
    #     super(self.__class__, self).update(interval)
    #     return True

if __name__ == "__main__":

 config = {}
 config['confile'] = './test.xml'
 config['port'] = 53817
 config['uname'] = 'TestProc'
 config['update_every'] = update_every
 config['priority'] = priority
 config['retries'] = retries

 serv = Service(config,"test")

 # config2 = {}
 # config2['confile'] = './test.xml'
 # config2['port'] = 52809
 # config2['uname'] = 'TestProc1'
 # config2['update_every'] = update_every
 # config2['priority'] = priority
 # config2['retries'] = retries
 #
 # serv2 = Service(config2,"test")
