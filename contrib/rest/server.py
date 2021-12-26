#!/usr/bin/env python3
# Copyright (c) 2021 Jeremy Rubin (added REST API)
#
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
- sends protocol 'version', per JSON-RPC 1.1
- sends proper, incrementing 'id'
- sends Basic HTTP authentication headers
- parses all JSON numbers that look like floats as Decimal
- uses standard Python json lib
"""

import base64
import re
import decimal
from http import HTTPStatus
import http.client
import json
import logging
import os
import socket
import sys
import time
import urllib.parse
import tornado.ioloop
import tornado.web
import tornado.httpclient
from tornado.httpclient import AsyncHTTPClient, HTTPRequest, HTTPError

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


def EncodeDecimal(o):
    if isinstance(o, decimal.Decimal):
        return str(o)
    raise TypeError(repr(o) + " is not JSON serializable")

class AuthServiceProxy():
    __id_count = 0

    # ensure_ascii: escape unicode as \uXXXX, passed to json.dumps
    def __init__(self, service_url, rpc_call=None, timeout=HTTP_TIMEOUT, http_client=None, ensure_ascii=True):
        self.__service_url = service_url
        self._rpc_call = rpc_call
        self.ensure_ascii = ensure_ascii  # can be toggled on the fly by tests
        self.__url = urllib.parse.urlparse(service_url)
        self.user = None if self.__url.username is None else self.__url.username.encode('utf8')
        self.passwd = None if self.__url.password is None else self.__url.password.encode('utf8')
        self.url = urllib.parse.urlunparse(self.__url)
        self.timeout = timeout
        port = 80 if self.__url.port is None else self.__url.port
        if http_client == None:
            self._set_client(AsyncHTTPClient())
        else:
            self._set_client(http_client)
    def _set_client(self, client):
        self.http_client = client


    def __getattr__(self, name):
        if name.startswith('__') and name.endswith('__'):
            # Python internal stuff
            raise AttributeError
        if self._rpc_call is not None:
            name = "%s.%s" % (self._rpc_call, name)
        return AuthServiceProxy(self.__service_url, name, http_client=self.http_client)

    async def _request(self, method, path, postdata):
        '''
        Do a HTTP request, with retry if we get disconnected (e.g. due to a timeout).
        This is a workaround for https://bugs.python.org/issue3566 which is fixed in Python 3.5.
        '''
        headers = {'Host': self.__url.hostname,
                   'User-Agent': USER_AGENT,
                   'Content-type': 'application/json'}

        req = HTTPRequest(self.url, method, headers, auth_username=self.user,
                          auth_password=self.passwd, request_timeout=self.timeout, body=postdata)
        try:
            resp = await self.http_client.fetch(req)
        except HTTPError as e:
            if e.code == 599:
                raise JSONRPCException({
                    'code': -344,
                    'message': '%r RPC took longer than %f seconds. Consider '
                               'using larger timeout for calls that take '
                               'longer to return.' % (self._rpc_call,
                                                      self.timeout)})
            else:
                e.rethrow()
        req_start_time = time.time()
        body = resp.body
        if body == b"":
            raise JSONRPCException({
                'code': -342, 'message': 'missing HTTP response from server'})

        content_type = resp.headers.get_list('Content-Type')
        if content_type != ['application/json']:
            raise JSONRPCException(
                {'code': -342, 'message': 'non-JSON HTTP response with \'%i %s\' from server' % (resp.code, resp.reason)},
                resp.code)

        responsedata = body.decode('utf8')
        response = json.loads(responsedata, parse_float=decimal.Decimal)
        elapsed = time.time() - req_start_time
        if "error" in response and response["error"] is None:
            log.debug("<-%s- [%.6f] %s" % (response["id"], elapsed, json.dumps(response["result"], default=EncodeDecimal, ensure_ascii=self.ensure_ascii)))
        else:
            log.debug("<-- [%.6f] %s" % (elapsed, responsedata))
        return response, resp.code



    def get_request(self, *args, **argsn):
        AuthServiceProxy.__id_count += 1

        log.debug("-{}-> {} {}".format(
            AuthServiceProxy.__id_count,
            self._rpc_call,
            json.dumps(args or argsn, default=EncodeDecimal, ensure_ascii=self.ensure_ascii),
        ))
        if args and argsn:
            raise ValueError('Cannot handle both named and positional arguments')
        return {'version': '1.1',
                'method': self._rpc_call,
                'params': args or argsn,
                'id': AuthServiceProxy.__id_count}

    async def __call__(self, *args, **argsn):
        postdata = json.dumps(self.get_request(*args, **argsn), default=EncodeDecimal, ensure_ascii=self.ensure_ascii)
        response, status = await self._request('POST', self.__url.path, postdata.encode('utf-8'))
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

    async def batch(self, rpc_call_list):
        postdata = json.dumps(list(rpc_call_list), default=EncodeDecimal, ensure_ascii=self.ensure_ascii)
        log.debug("--> " + postdata)
        response, status = await self._request('POST', self.__url.path, postdata.encode('utf-8'))
        if status != HTTPStatus.OK:
            raise JSONRPCException({
                'code': -342, 'message': 'non-200 HTTP status code but no JSON-RPC error'}, status)
        return response


app_log = logging.getLogger('tornado.application')


class Fetch:
    _node = AuthServiceProxy(sys.argv[1])
    @classmethod
    @property
    def node(cls):
        return cls._node
class rest_tx(tornado.web.RequestHandler):
    rpc = Fetch.node.getrawtransaction
    async def get(self, txhash, encoding):
        try:
            result = await self.rpc(txhash, 1 if encoding == ".json" else 0)
        except JSONRPCException as e:
            msg = f"Transaction Not Found: {e}"
            raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
        if encoding == ".json":
            self.set_header("Content-Type", 'application/json')
            self.write(json.dumps(result, default=EncodeDecimal))
        if encoding == ".bin":
            self.set_header("Content-Type", 'application/octet-stream')
            self.write(bytes.fromhex(result))
        if encoding == ".hex":
            self.set_header("Content-Type", 'application/plain')
            self.write(result)

def rest_block(details, rpc_shared = Fetch.node.getblock):
    class inner(tornado.web.RequestHandler):
        rpc = rpc_shared
        async def get(self, blockhash, encoding):
            try:
                result = await self.rpc(blockhash, 0 if encoding in [".hex", ".bin"] else (2 if details else 1))
            except JSONRPCException as e:
                msg = f"Block Not Found: {e}"
                raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
            if encoding == ".json":
                self.set_header("Content-Type", 'application/json')
                self.write(json.dumps(result, default=EncodeDecimal))
            if encoding == ".bin":
                self.set_header("Content-Type", 'application/octet-stream')
                self.write(bytes.fromhex(result))
            if encoding == ".hex":
                self.set_header("Content-Type", 'application/plain')
                self.write(result)
    return inner

class rest_chaininfo(tornado.web.RequestHandler):
    rpc = Fetch.node.getblockchaininfo
    async def get(self):
        try:
            result = await self.rpc()
        except JSONRPCException as e:
            msg = f"{e}"
            raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
        self.set_header("Content-Type", 'application/json')
        self.write(json.dumps(result, default=EncodeDecimal))
class rest_mempool_info(tornado.web.RequestHandler):
    rpc = Fetch.node.getmempoolinfo
    async def get(self):
        try:
            result = await self.rpc()
        except JSONRPCException as e:
            msg = f"{e}"
            raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
        self.set_header("Content-Type", 'application/json')
        self.write(json.dumps(result, default=EncodeDecimal))
class rest_mempool_contents(tornado.web.RequestHandler):
    rpc = Fetch.node.getrawmempool
    async def get(self):
        try:
            result = await self.rpc(1) 
        except JSONRPCException as e:
            msg = f"Mempool Error: {e}"
            raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
        self.set_header("Content-Type", 'application/json')
        self.write(json.dumps(result, default=EncodeDecimal))

class rest_headers(tornado.web.RequestHandler):
    rpc = Fetch.node.getblockheader
    async def get(self, strcount, starthash, encoding):
        count = int(strcount)
        if count > 2000 or count < 1:
            msg = "Header Count out of range ("+strcount+")"
            raise tornado.web.HTTPError(status_code=400, reason=msg, log_message=msg)
        result = []
        h = starthash
        for _ in range(count):
            try:
                res = await self.rpc(h, 1 if encoding == ".json" else 0)
            except JSONRPCException as e:
                msg = f"Header {h} Not Found: {e}"
                raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
            result.append(res)
            h = result[-1]["previousblockhash"] if encoding == ".json" else result[-1][4:4+32]
            if all(ch == '0' for ch in h):
                # Is Genesis Block
                break

        if encoding == ".bin":
            r = b"".join(map(bytes.fromhex, result))
            self.set_header("Content-Type", 'application/octet-stream')
            self.write(r)
        if encoding == ".hex":
            self.set_header("Content-Type", 'application/plain')
            self.write("".join(result))
        if encoding == ".json":
            self.set_header("Content-Type", 'application/json')
            self.write(json.dumps(result, default=EncodeDecimal))

class rest_blockhash_by_height(tornado.web.RequestHandler):
    rpc = Fetch.node.getblockhash
    async def get(self, str_height, encoding):
        height = int(str_height)
        try:
            result = await self.rpc(height)
        except JSONRPCException as e:
            msg = f"Block {str_height} Not Found: {e}"
            raise tornado.web.HTTPError(status_code=404, log_message=msg, reason=msg)
        if encoding == ".json":
            self.set_header("Content-Type", 'application/json')
            self.write(json.dumps({"blockhash":result}, default=EncodeDecimal))
        if encoding == ".bin":
            self.set_header("Content-Type", 'application/octet-stream')
            self.write(bytes.fromhex(result))
        if encoding == ".hex":
            self.set_header("Content-Type", 'application/plain')
            self.write(result)

class rest_getutxos(tornado.web.RequestHandler):
    rpc = Fetch.node.gettxout
    match_txid = re.compile("([0-9a-fA-F]{64}):[0-9]+")
    MAX_QUERY = 15
    async def get(self, encoding):
        txids = self.get_query_arguments("txid")
        if len(txids) > self.MAX_QUERY:
            msg = f"Requested too many TXIDs, max is {MAX_QUERY}"
            raise tornado.web.HTTPError(status_code=400, log_message=msg, reason=msg)
        if len(txids) == 0 :
            msg = f"Must request one or more txid"
            raise tornado.web.HTTPError(status_code=400, log_message=msg, reason=msg)
        check_mempool = int(self.get_query_argument("checkmempool", 0))
        if check_mempool not in [0,1]:
            msg = f"check_mempool must be either unset, 0, or 1"
            raise tornado.web.HTTPError(status_code=400, log_message=msg, reason=msg)

        if any(self.match_txid.fullmatch(txid) is None for txid in txids):
            msg = f"TXIDs must all be <64 bytes hex id>:<int>"
            raise tornado.web.HTTPError(status_code=400, log_message=msg, reason=msg)
        def req(txid_s):
            (txid, n) = txid_s.split(":")
            data = [txid, n, check_mempool]
            return self.rpc.get_request(data)
        reqs = map(req, txids)
        result = await self.rpc.batch(reqs)
        if encoding == ".json":
            self.set_header("Content-Type", 'application/json')
            self.write(json.dumps({"blockhash":result}, default=EncodeDecimal))
        if encoding == ".bin":
            self.set_header("Content-Type", 'application/octet-stream')
            self.write(bytes.fromhex(result))
        if encoding == ".hex":
            self.set_header("Content-Type", 'application/plain')
            print(result)
            self.write(result)


def make_app():
    format_hash = "([0-9a-fA-F]{64})(\\.json|\\.hex|\\.bin)"
    format_ext = "(\\.json|\\.hex|\\.bin)"
    return tornado.web.Application([
        (f"/rest/tx/{format_hash}", rest_tx),
        (f"/rest/block/notxdetails/{format_hash}", rest_block(False)),
        (f"/rest/block/{format_hash}", rest_block(True)),
        (f"/rest/chaininfo.json", rest_chaininfo),
        (f"/rest/mempool/info.json", rest_mempool_info),
        (f"/rest/mempool/contents.json", rest_mempool_contents),
        (f"/rest/headers/([0-9])+/{format_hash}", rest_headers),
        (f"/rest/blockhashbyheight/([0-9]+){format_ext}", rest_blockhash_by_height),
        (f"/rest/getutxos/{format_ext}", rest_getutxos), 
        ])

port = int(sys.argv[2])
if __name__ == "__main__":
    app = make_app()
    app.listen(port)
    tornado.ioloop.IOLoop.current().start()
