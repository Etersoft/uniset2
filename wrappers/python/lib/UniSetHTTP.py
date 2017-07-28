#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright (c) 2016 Pavel Vainerman <pv@etersoft.ru>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, version 2.1.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Lesser Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import json
import urllib2
import urllib

UHTTP_API_VERSION = 'v0'


class UHTTPError:
    def __init__(self, err=''):
        self.message = err


class UniSetHTTPService:
    def __init__(self, _settings):

        self.settings = _settings
        if 'url' not in self.settings:
            raise ValueError('В настройках нужно задать url')

        self.apiver = UHTTP_API_VERSION
        if 'api' in self.settings:
            self.aviver = self.settings['api']

    def request(self, query, method='GET', data=None):
        '''
        Послать запрос и получить ответ.
        :param query: запрос /xxx?params..
        :param method: метод запроса.
        :param data: данные для POST запросов
        :return: распарсенный json
        '''

        url = self.settings.get('url') + "/api/" + self.apiver + query
        headers = {}
        if data is not None:
            data = urllib.urlencode(data)

        try:
            request = urllib2.Request(url, data, headers)
            request.get_method = lambda: method
            resp = urllib2.urlopen(request)
            return json.loads(resp.read())
        except urllib2.URLError, e:
            if hasattr(e, 'reason'):
                err = 'We failed to reach a server. Reason: %s' % e.reason
                raise UHTTPError(err)

            elif hasattr(e, 'code'):
                err = 'The server couldn\'t fulfill the request. Error code: %s' % e.code
                raise UHTTPError(err)
            raise UHTTPError('Unknown error')


class SharedMemoryAPI(UniSetHTTPService):

    def __init__(self, _settings):
        UniSetHTTPService.__init__(self, _settings)
        if 'smID' not in self.settings:
            raise ValueError('(SharedMemoryAPI): В настройках нужно задать smID')

    def request(self, query, method='GET', data=None):
        return UniSetHTTPService.request(self, '/' + self.settings['smID'] + '/' + query)

    def consumers(self, sens=''):
        '''
        Получить список заказчиков
        :param sens: для указанных датчиков
        :return: список..
        '''
        query = '/consumers'
        if sens != '':
            query += '?%s' % sens

        return self.request(query)[self.settings['smID']]['consumers']

    def get(self, sensors='', shortInfo=True):
        '''
        Получить список заказчиков
        :param sensors:   для указанных датчиков (по умолчанию для всех)
        :param shortInfo: выдать только основную информацию по каждому датчику
        :return: список..
        '''
        query = '/get'
        params = None
        if sensors != '':
            params = '?%s' % sensors

        if shortInfo:
            if params is not None:
                params += '&shortInfo'
            else:
                params = '?shortInfo'

        if params:
            query += params

        return self.request(query)[self.settings['smID']]['sensors']

    def sensors(self, offset=None, limit=None):
        '''
        Получить список датчиков
        :param offset: начальное смещение в списке датчиков
        :param limit:  сколько датиков выдать в ответе
        :return: список..
        '''
        query = '/sensors'
        params = None
        if offset:
            params = '?offset=%d' % offset

        if limit:
            if params is not None:
                params += '&limit=%d' % limit
            else:
                params = '?limit=%d' % limit

        if params:
            query += params

        return self.request(query)

    def lost(self):
        '''
        Получить список 'пропавших' заказчиков
        :return: список
        '''
        return self.request('/lost')[self.settings['smID']]

    def help(self):
        return self.request('/help')[self.settings['smID']]