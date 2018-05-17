#!/usr/bin/env python3
#
# Example code for wallet push notifications (similar to -walletnotify)
# ./example_walletnotify.py -network=regtest -datadir=~/-.bitcoin/regtest
#
# Copyright (c) 2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from __future__ import print_function
import argparse
import base64
try: # Python 3
    import http.client as httplib
except ImportError: # Python 2
    import httplib
import json
import os
import os.path
import sys

settings = {}

class BitcoinRPC:
    def __init__(self, host, port, username, password):
        authpair = "%s:%s" % (username, password)
        authpair = authpair.encode('utf-8')
        self.authhdr = b"Basic " + base64.b64encode(authpair)
        self.conn = httplib.HTTPConnection(host, port=port, timeout=30)

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
        if len(body) == 0:
            print("Invalid response, auth may failed")
            return
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

def get_rpc_cookie(datadir):
    # Open the cookie file
    with open(os.path.join(os.path.expanduser(datadir), '.cookie'), 'r') as f:
        combined = f.readline()
        combined_split = combined.split(":")
        settings['rpcuser'] = combined_split[0]
        settings['rpcpassword'] = combined_split[1]

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=
                                     'WalletNotify push example that uses listsincetx under the hood (only works if there is already a transaction)'
                                     '/example_walletnotify.py -network=regtest -datadir=~/-.bitcoin/regtest')
    parser.add_argument('-network', default="regtest",
                        help='Network to use (mainnet, testnet, regtest)')
    parser.add_argument('-host', default="127.0.0.1",
                        help='Host to access')
    parser.add_argument('-rpcuser', default="user",
                        help='RPC username')
    parser.add_argument('-rpcpassword', default="pass",
                        help='RPC password')
    parser.add_argument('-datadir',
                        help='Datadir to read the RPC cookie from (if set, rpcuser/rpcpassword is obsolete)')
    args = parser.parse_args()
    if not args.datadir:
        parser.print_help()
        exit()
    settings['host'] = args.host
    if args.network == "regtest":
        settings['port'] =  18443
    elif args.network == "mainnet":
        settings['port'] =  8332
    elif args.network == "testnet3" or args.network == "testnet" :
        settings['port'] =  18332
    settings['rpcuser'] = args.rpcuser
    settings['rpcpassword'] = args.rpcpassword
    if args.datadir:
        get_rpc_cookie(args.datadir)

    rpc = BitcoinRPC(settings['host'], settings['port'],
             settings['rpcuser'], settings['rpcpassword'])
    latest_wtx = rpc.execute(rpc.build_request(0, "listtransactions", ["*", 1]))['result'][-1]['txid']
    while True:
        res = rpc.execute(rpc.build_request(0, "listsincetx", [latest_wtx, 100]))
        for tx in res['result']:
            print("New txid"+tx['txid']+" with amount "+str(tx['amount']))
        latest_wtx = res['result'][-1]['txid'] #update latest wtx
