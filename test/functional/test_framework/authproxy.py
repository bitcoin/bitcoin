# Copyright (c) 2011 Jeff Garzik
#
# Previous copyright, from python-jsonrpc/jsonrpc/proxy.py:
#
# Copyright (c) 2007 Jan-Klaas Kollhof
#
# This file is part of jsonrpc.
#
# jsonrpc is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this software; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""HTTP proxy for opening RPC connection to bitcoind.

AuthServiceProxy has the following improvements over python-jsonrpc's
ServiceProxy class:

- HTTP connections persist for the life of the AuthServiceProxy object
  (if server supports HTTP/1.1)
- sends "jsonrpc":"2.0", per JSON-RPC 2.0
- sends proper, incrementing 'id'
- sends Basic HTTP authentication headers
- parses all JSON numbers that look like floats as Decimal
- uses standard Python json lib
"""

import base64
import decimal
from http import HTTPStatus
import http.client
import json
import logging
import pathlib
import socket
import time
import urllib.parse

HTTP_TIMEOUT = 30
USER_AGENT = "AuthServiceProxy/0.1"

log = logging.getLogger("BitcoinRPC")

class JSONRPCException(Exception):
    def __init__(self, rpc_error, http_status=None):
        try:
            errmsg = '%(message)s (%(code)i)' % rpc_error
        except (KeyError, TypeError):
            errmsg = ''
        super().__init__(errmsg)
        self.error = rpc_error
        self.http_status = http_status


def serialization_fallback(o):
    if isinstance(o, decimal.Decimal):
        return str(o)
    if isinstance(o, pathlib.Path):
        return str(o)
    raise TypeError(repr(o) + " is not JSON serializable")

class AuthServiceProxy():
    __id_count = 0

    # ensure_ascii: escape unicode as \uXXXX, passed to json.dumps
    def __init__(self, service_url, service_name=None, timeout=HTTP_TIMEOUT, connection=None, ensure_ascii=True):
        self.__service_url = service_url
        self._service_name = service_name
        self.ensure_ascii = ensure_ascii  # can be toggled on the fly by tests
        self.__url = urllib.parse.urlparse(service_url)
        user = None if self.__url.username is None else self.__url.username.encode('utf8')
        passwd = None if self.__url.password is None else self.__url.password.encode('utf8')
        authpair = user + b':' + passwd
        self.__auth_header = b'Basic ' + base64.b64encode(authpair)
        # clamp the socket timeout, since larger values can cause an
        # "Invalid argument" exception in Python's HTTP(S) client
        # library on some operating systems (e.g. OpenBSD, FreeBSD)
        self.timeout = min(timeout, 2147483)
        self._set_conn(connection)

    def __getattr__(self, name):
        if name.startswith('__') and name.endswith('__'):
            # Python internal stuff
            raise AttributeError
        if self._service_name is not None:
            name = "%s.%s" % (self._service_name, name)
        return AuthServiceProxy(self.__service_url, name, connection=self.__conn)

    def _request(self, method, path, postdata):
        '''
        Do a HTTP request.
        '''
        headers = {'Host': self.__url.hostname,
                   'User-Agent': USER_AGENT,
                   'Authorization': self.__auth_header,
                   'Content-type': 'application/json'}
        self.__conn.request(method, path, postdata, headers)
        return self._get_response()

    def _json_dumps(self, obj):
        return json.dumps(obj, default=serialization_fallback, ensure_ascii=self.ensure_ascii)

    def get_request(self, *args, **argsn):
        AuthServiceProxy.__id_count += 1

        log.debug("-{}-> {} {} {}".format(
            AuthServiceProxy.__id_count,
            self._service_name,
            self._json_dumps(args),
            self._json_dumps(argsn),
        ))

        if args and argsn:
            params = dict(args=args, **argsn)
        else:
            params = args or argsn
        return {'jsonrpc': '2.0',
                'method': self._service_name,
                'params': params,
                'id': AuthServiceProxy.__id_count}

    def __call__(self, *args, **argsn):
        postdata = self._json_dumps(self.get_request(*args, **argsn))
        response, status = self._request('POST', self.__url.path, postdata.encode('utf-8'))
        # For backwards compatibility tests, accept JSON RPC 1.1 responses
        if 'jsonrpc' not in response:
            if response['error'] is not None:
                raise JSONRPCException(response['error'], status)
            elif 'result' not in response:
                raise JSONRPCException({
                    'code': -343, 'message': 'missing JSON-RPC result'}, status)
            elif status != HTTPStatus.OK:
                raise JSONRPCException({
                    'code': -342, 'message': 'non-200 HTTP status code but no JSON-RPC error'}, status)
            else:
                return response['result']
        else:
            assert response['jsonrpc'] == '2.0'
            if status != HTTPStatus.OK:
                raise JSONRPCException({
                    'code': -342, 'message': 'non-200 HTTP status code'}, status)
            if 'error' in response:
                raise JSONRPCException(response['error'], status)
            elif 'result' not in response:
                raise JSONRPCException({
                    'code': -343, 'message': 'missing JSON-RPC 2.0 result and error'}, status)
            return response['result']

    def batch(self, rpc_call_list):
        postdata = self._json_dumps(list(rpc_call_list))
        log.debug("--> " + postdata)
        response, status = self._request('POST', self.__url.path, postdata.encode('utf-8'))
        if status != HTTPStatus.OK:
            raise JSONRPCException({
                'code': -342, 'message': 'non-200 HTTP status code'}, status)
        return response

    def _get_response(self):
        req_start_time = time.time()
        try:
            http_response = self.__conn.getresponse()
        except socket.timeout:
            raise JSONRPCException({
                'code': -344,
                'message': '%r RPC took longer than %f seconds. Consider '
                           'using larger timeout for calls that take '
                           'longer to return.' % (self._service_name,
                                                  self.__conn.timeout)})
        if http_response is None:
            raise JSONRPCException({
                'code': -342, 'message': 'missing HTTP response from server'})

        # Check for no-content HTTP status code, which can be returned when an
        # RPC client requests a JSON-RPC 2.0 "notification" with no response.
        # Currently this is only possible if clients call the _request() method
        # directly to send a raw request.
        if http_response.status == HTTPStatus.NO_CONTENT:
            if len(http_response.read()) != 0:
                raise JSONRPCException({'code': -342, 'message': 'Content received with NO CONTENT status code'})
            return None, http_response.status

        content_type = http_response.getheader('Content-Type')
        if content_type != 'application/json':
            raise JSONRPCException(
                {'code': -342, 'message': 'non-JSON HTTP response with \'%i %s\' from server' % (http_response.status, http_response.reason)},
                http_response.status)

        data = http_response.read()
        try:
            responsedata = data.decode('utf8')
        except UnicodeDecodeError as e:
            raise JSONRPCException({
                'code': -342, 'message': f'Cannot decode response in utf8 format, content: {data}, exception: {e}'})
        response = json.loads(responsedata, parse_float=decimal.Decimal)
        elapsed = time.time() - req_start_time
        if "error" in response and response["error"] is None:
            log.debug("<-%s- [%.6f] %s" % (response["id"], elapsed, self._json_dumps(response["result"])))
        else:
            log.debug("<-- [%.6f] %s" % (elapsed, responsedata))
        return response, http_response.status

    def __truediv__(self, relative_uri):
        return AuthServiceProxy("{}/{}".format(self.__service_url, relative_uri), self._service_name, connection=self.__conn)

    def _set_conn(self, connection=None):
        port = 80 if self.__url.port is None else self.__url.port
        if connection:
            self.__conn = connection
            self.timeout = connection.timeout
        elif self.__url.scheme == 'https':
            self.__conn = http.client.HTTPSConnection(self.__url.hostname, port, timeout=self.timeout)
        else:
            self.__conn = http.client.HTTPConnection(self.__url.hostname, port, timeout=self.timeout)
