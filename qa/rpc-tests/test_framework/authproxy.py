
"""
  Copyright (c) 2011 Jeff Garzik

  AuthServiceProxy has the following improvements over python-jsonrpc's
  ServiceProxy class:

  - HTTP connections persist for the life of the AuthServiceProxy object
    (if server supports HTTP/1.1)
  - sends protocol 'version', per JSON-RPC 1.1
  - sends proper, incrementing 'id'
  - sends Basic HTTP authentication headers
  - parses all JSON numbers that look like floats as Decimal
  - uses standard Python json lib

  Previous copyright, from python-jsonrpc/jsonrpc/proxy.py:

  Copyright (c) 2007 Jan-Klaas Kollhof

  This file is part of jsonrpc.

  jsonrpc is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this software; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""

try:
    import http.client as httplib
except ImportError:
    import httplib
import base64
import decimal
import json
import logging
import socket
try:
    import urllib.parse as urlparse
except ImportError:
    import urlparse

USER_AGENT = "AuthServiceProxy/0.1"

HTTP_TIMEOUT = 30

log = logging.getLogger("BitcoinRPC")

class JSONRPCException(Exception):
    def __init__(self, rpc_error):
        try:
            errmsg = '%(message)s (%(code)i)' % rpc_error
        except (KeyError, TypeError):
            errmsg = ''
        Exception.__init__(self, errmsg)
        self.error = rpc_error


def EncodeDecimal(o):
    if isinstance(o, decimal.Decimal):
        return str(o)
    raise TypeError(repr(o) + " is not JSON serializable")

class AuthServiceProxy(object):
    __id_count = 0

    # ensure_ascii: escape unicode as \uXXXX, passed to json.dumps
    def __init__(self, service_url, service_name=None, timeout=HTTP_TIMEOUT, connection=None, ensure_ascii=True):
        self.__service_url = service_url
        self._service_name = service_name
        self.ensure_ascii = ensure_ascii # can be toggled on the fly by tests
        self.__url = urlparse.urlparse(service_url)
        if self.__url.port is None:
            port = 80
        else:
            port = self.__url.port
        (user, passwd) = (self.__url.username, self.__url.password)
        try:
            user = user.encode('utf8')
        except AttributeError:
            pass
        try:
            passwd = passwd.encode('utf8')
        except AttributeError:
            pass
        authpair = user + b':' + passwd
        self.__auth_header = b'Basic ' + base64.b64encode(authpair)

        if connection:
            # Callables re-use the connection of the original proxy
            self.__conn = connection
        elif self.__url.scheme == 'https':
            self.__conn = httplib.HTTPSConnection(self.__url.hostname, port,
                                                  timeout=timeout)
        else:
            self.__conn = httplib.HTTPConnection(self.__url.hostname, port,
                                                 timeout=timeout)

    def __getattr__(self, name):
        if name.startswith('__') and name.endswith('__'):
            # Python internal stuff
            raise AttributeError
        if self._service_name is not None:
            name = "%s.%s" % (self._service_name, name)
        return AuthServiceProxy(self.__service_url, name, connection=self.__conn)

    def _request(self, method, path, postdata):
        '''
        Do a HTTP request, with retry if we get disconnected (e.g. due to a timeout).
        This is a workaround for https://bugs.python.org/issue3566 which is fixed in Python 3.5.
        '''
        headers = {'Host': self.__url.hostname,
                   'User-Agent': USER_AGENT,
                   'Authorization': self.__auth_header,
                   'Content-type': 'application/json'}
        try:
            self.__conn.request(method, path, postdata, headers)
            return self._get_response()
        except httplib.BadStatusLine as e:
            if e.line == "''": # if connection was closed, try again
                self.__conn.close()
                self.__conn.request(method, path, postdata, headers)
                return self._get_response()
            else:
                raise
        except (BrokenPipeError,ConnectionResetError):
            # Python 3.5+ raises BrokenPipeError instead of BadStatusLine when the connection was reset
            # ConnectionResetError happens on FreeBSD with Python 3.4
            self.__conn.close()
            self.__conn.request(method, path, postdata, headers)
            return self._get_response()

    def __call__(self, *args, **argsn):
        AuthServiceProxy.__id_count += 1

        log.debug("-%s-> %s %s"%(AuthServiceProxy.__id_count, self._service_name,
                                 json.dumps(args, default=EncodeDecimal, ensure_ascii=self.ensure_ascii)))
        if args and argsn:
            raise ValueError('Cannot handle both named and positional arguments')
        postdata = json.dumps({'version': '1.1',
                               'method': self._service_name,
                               'params': args or argsn,
                               'id': AuthServiceProxy.__id_count}, default=EncodeDecimal, ensure_ascii=self.ensure_ascii)
        response = self._request('POST', self.__url.path, postdata.encode('utf-8'))
        if response['error'] is not None:
            raise JSONRPCException(response['error'])
        elif 'result' not in response:
            raise JSONRPCException({
                'code': -343, 'message': 'missing JSON-RPC result'})
        else:
            return response['result']

    def _batch(self, rpc_call_list):
        postdata = json.dumps(list(rpc_call_list), default=EncodeDecimal, ensure_ascii=self.ensure_ascii)
        log.debug("--> "+postdata)
        return self._request('POST', self.__url.path, postdata.encode('utf-8'))

    def _get_response(self):
        try:
            http_response = self.__conn.getresponse()
        except socket.timeout as e:
            raise JSONRPCException({
                'code': -344,
                'message': '%r RPC took longer than %f seconds. Consider '
                           'using larger timeout for calls that take '
                           'longer to return.' % (self._service_name,
                                                  self.__conn.timeout)})
        if http_response is None:
            raise JSONRPCException({
                'code': -342, 'message': 'missing HTTP response from server'})

        content_type = http_response.getheader('Content-Type')
        if content_type != 'application/json':
            raise JSONRPCException({
                'code': -342, 'message': 'non-JSON HTTP response with \'%i %s\' from server' % (http_response.status, http_response.reason)})

        responsedata = http_response.read().decode('utf8')
        response = json.loads(responsedata, parse_float=decimal.Decimal)
        if "error" in response and response["error"] is None:
            log.debug("<-%s- %s"%(response["id"], json.dumps(response["result"], default=EncodeDecimal, ensure_ascii=self.ensure_ascii)))
        else:
            log.debug("<-- "+responsedata)
        return response
