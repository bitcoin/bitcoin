#!/usr/bin/python
#
# linearize-hashes.py:  List blocks in a linear, no-fork version of the chain.
#
# Copyright (c) 2013-2014 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from __future__ import print_function
import json
import struct
import re
import base64
import httplib
import sys

settings = {}

class BitcoinRPC:
	def __init__(self, host, port, username, password):
		authpair = "%s:%s" % (username, password)
		self.authhdr = "Basic %s" % (base64.b64encode(authpair))
		self.conn = httplib.HTTPConnection(host, port, False, 30)

	def execute(self, obj):
		self.conn.request('POST', '/', json.dumps(obj),
			{ 'Authorization' : self.authhdr,
			  'Content-type' : 'application/json' })

		resp = self.conn.getresponse()
		if resp is None:
			print("JSON-RPC: no response", file=sys.stderr)
			return None

		body = resp.read()
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

		for x,resp_obj in enumerate(reply):
			if rpc.response_is_error(resp_obj):
				print('JSON-RPC: error at height', height+x, ': ', resp_obj['error'], file=sys.stderr)
				exit(1)
			assert(resp_obj['id'] == x) # assume replies are in-sequence
			print(resp_obj['result'])

		height += num_blocks

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print("Usage: linearize-hashes.py CONFIG-FILE")
		sys.exit(1)

	f = open(sys.argv[1])
	for line in f:
		# skip comment lines
		m = re.search('^\s*#', line)
		if m:
			continue

		# parse key=value lines
		m = re.search('^(\w+)\s*=\s*(\S.*)$', line)
		if m is None:
			continue
		settings[m.group(1)] = m.group(2)
	f.close()

	if 'host' not in settings:
		settings['host'] = '127.0.0.1'
	if 'port' not in settings:
		settings['port'] = 9998
	if 'min_height' not in settings:
		settings['min_height'] = 0
	if 'max_height' not in settings:
		settings['max_height'] = 313000
	if 'rpcuser' not in settings or 'rpcpassword' not in settings:
		print("Missing username and/or password in cfg file", file=stderr)
		sys.exit(1)

	settings['port'] = int(settings['port'])
	settings['min_height'] = int(settings['min_height'])
	settings['max_height'] = int(settings['max_height'])

	get_block_hashes(settings)

