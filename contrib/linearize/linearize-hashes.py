#!/usr/bin/env python3
#
# linearize-hashes.py:  List blocks in a linear, no-fork version of the chain.
#
# Copyright (c) 2013-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from http.client import HTTPConnection
import json
import re
import base64
import sys
import os
import os.path

settings = {}

def hex_switchEndian(s):
    """ Switches the endianness of a hex string (in pairs of hex chars) """
    pairList = [s[i:i+2].encode() for i in range(0, len(s), 2)]
    return b''.join(pairList[::-1]).decode()

class BitcoinRPC:
    def __init__(self, host, port, username, password):
        authpair = "%s:%s" % (username, password)
        authpair = authpair.encode('utf-8')
        self.authhdr = b"Basic " + base64.b64encode(authpair)
        self.conn = HTTPConnection(host, port=port, timeout=30)

    def execute(self, obj):
        try:
            self.conn.request('POST', '/', json.dumps(obj),
                { 'Authorization' : self.authhdr,
                  'Content-type' : 'application/json' })
        except ConnectionRefusedError:
            print('RPC connection refused. Check RPC settings and the server status.',
                  file=sys.stderr)
            return None

        resp = self.conn.getresponse()
        if resp is None:
            print("JSON-RPC: no response", file=sys.stderr)
            return None

        body = resp.read().decode('utf-8')
        resp_obj = json.loads(body)
        return resp_obj

    @staticmethod
    def build_request(idx, method, params):
        obj = { 'version' : '1.1',
            'method' : method,
            'id' : idx }
        if params is None:
            obj['params'] = []
        else:
            obj['params'] = params
        return obj

    @staticmethod
    def response_is_error(resp_obj):
        return 'error' in resp_obj and resp_obj['error'] is not None

def get_block_hashes(settings, max_blocks_per_call=10000):
    rpc = BitcoinRPC(settings['host'], settings['port'],
             settings['rpcuser'], settings['rpcpassword'])

    height = settings['min_height']
    while height < settings['max_height']+1:
        num_blocks = min(settings['max_height']+1-height, max_blocks_per_call)
        batch = []
        for x in range(num_blocks):
            batch.append(rpc.build_request(x, 'getblockhash', [height + x]))

        reply = rpc.execute(batch)
        if reply is None:
            print('Cannot continue. Program will halt.')
            return None

        for x,resp_obj in enumerate(reply):
            if rpc.response_is_error(resp_obj):
                print('JSON-RPC: error at height', height+x, ': ', resp_obj['error'], file=sys.stderr)
                sys.exit(1)
            assert(resp_obj['id'] == x) # assume replies are in-sequence
            if settings['rev_hash_bytes'] == 'true':
                resp_obj['result'] = hex_switchEndian(resp_obj['result'])
            print(resp_obj['result'])

        height += num_blocks

def get_rpc_cookie():
    # Open the cookie file
    with open(os.path.join(os.path.expanduser(settings['datadir']), '.cookie'), 'r', encoding="ascii") as f:
        combined = f.readline()
        combined_split = combined.split(":")
        settings['rpcuser'] = combined_split[0]
        settings['rpcpassword'] = combined_split[1]

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: linearize-hashes.py CONFIG-FILE")
        sys.exit(1)

    f = open(sys.argv[1], encoding="utf8")
    for line in f:
        # skip comment lines
        m = re.search(r'^\s*#', line)
        if m:
            continue

        # parse key=value lines
        m = re.search(r'^(\w+)\s*=\s*(\S.*)$', line)
        if m is None:
            continue
        settings[m.group(1)] = m.group(2)
    f.close()

    if 'host' not in settings:
        settings['host'] = '127.0.0.1'
    if 'port' not in settings:
        settings['port'] = 26145
    if 'min_height' not in settings:
        settings['min_height'] = 0
    if 'max_height' not in settings:
        settings['max_height'] = 313000
    if 'rev_hash_bytes' not in settings:
        settings['rev_hash_bytes'] = 'false'

    use_userpass = True
    use_datadir = False
    if 'rpcuser' not in settings or 'rpcpassword' not in settings:
        use_userpass = False
    if 'datadir' in settings and not use_userpass:
        use_datadir = True
    if not use_userpass and not use_datadir:
        print("Missing datadir or username and/or password in cfg file", file=sys.stderr)
        sys.exit(1)

    settings['port'] = int(settings['port'])
    settings['min_height'] = int(settings['min_height'])
    settings['max_height'] = int(settings['max_height'])

    # Force hash byte format setting to be lowercase to make comparisons easier.
    settings['rev_hash_bytes'] = settings['rev_hash_bytes'].lower()

    # Get the rpc user and pass from the cookie if the datadir is set
    if use_datadir:
        get_rpc_cookie()

    get_block_hashes(settings)
