#!/usr/bin/python
#
# linearize.py:  Construct a linear, no-fork, best version of the blockchain.
#
#
# Copyright (c) 2013 The Bitcoin developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

import json
import struct
import re
import base64
import httplib
import sys

ERR_SLEEP = 15
MAX_NONCE = 1000000L

settings = {}

class BitcoinRPC:
	OBJID = 1

	def __init__(self, host, port, username, password):
		authpair = "%s:%s" % (username, password)
		self.authhdr = "Basic %s" % (base64.b64encode(authpair))
		self.conn = httplib.HTTPConnection(host, port, False, 30)
	def rpc(self, method, params=None):
		self.OBJID += 1
		obj = { 'version' : '1.1',
			'method' : method,
			'id' : self.OBJID }
		if params is None:
			obj['params'] = []
		else:
			obj['params'] = params
		self.conn.request('POST', '/', json.dumps(obj),
			{ 'Authorization' : self.authhdr,
			  'Content-type' : 'application/json' })

		resp = self.conn.getresponse()
		if resp is None:
			print "JSON-RPC: no response"
			return None

		body = resp.read()
		resp_obj = json.loads(body)
		if resp_obj is None:
			print "JSON-RPC: cannot JSON-decode body"
			return None
		if 'error' in resp_obj and resp_obj['error'] != None:
			return resp_obj['error']
		if 'result' not in resp_obj:
			print "JSON-RPC: no result in object"
			return None

		return resp_obj['result']
	def getblock(self, hash, verbose=True):
		return self.rpc('getblock', [hash, verbose])
	def getblockhash(self, index):
		return self.rpc('getblockhash', [index])

def getblock(rpc, settings, n):
	hash = rpc.getblockhash(n)
	hexdata = rpc.getblock(hash, False)
	data = hexdata.decode('hex')

	return data

def get_blocks(settings):
	rpc = BitcoinRPC(settings['host'], settings['port'],
			 settings['rpcuser'], settings['rpcpassword'])

	outf = open(settings['output'], 'ab')

	for height in xrange(settings['min_height'], settings['max_height']+1):
		data = getblock(rpc, settings, height)

		outhdr = settings['netmagic']
		outhdr += struct.pack("<i", len(data))

		outf.write(outhdr)
		outf.write(data)

		if (height % 1000) == 0:
			sys.stdout.write("Wrote block " + str(height) + "\n")

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print "Usage: linearize.py CONFIG-FILE"
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

	if 'netmagic' not in settings:
		settings['netmagic'] = 'f9beb4d9'
	if 'output' not in settings:
		settings['output'] = 'bootstrap.dat'
	if 'host' not in settings:
		settings['host'] = '127.0.0.1'
	if 'port' not in settings:
		settings['port'] = 8332
	if 'min_height' not in settings:
		settings['min_height'] = 0
	if 'max_height' not in settings:
		settings['max_height'] = 279000
	if 'rpcuser' not in settings or 'rpcpassword' not in settings:
		print "Missing username and/or password in cfg file"
		sys.exit(1)

	settings['netmagic'] = settings['netmagic'].decode('hex')
	settings['port'] = int(settings['port'])
	settings['min_height'] = int(settings['min_height'])
	settings['max_height'] = int(settings['max_height'])

	get_blocks(settings)


