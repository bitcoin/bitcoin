#!/usr/bin/python
#
# linearize-data.py: Construct a linear, no-fork version of the chain.
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
import hashlib
import datetime

settings = {}


def uint32(x):
	return x & 0xffffffffL

def bytereverse(x):
	return uint32(( ((x) << 24) | (((x) << 8) & 0x00ff0000) |
		       (((x) >> 8) & 0x0000ff00) | ((x) >> 24) ))

def bufreverse(in_buf):
	out_words = []
	for i in range(0, len(in_buf), 4):
		word = struct.unpack('@I', in_buf[i:i+4])[0]
		out_words.append(struct.pack('@I', bytereverse(word)))
	return ''.join(out_words)

def wordreverse(in_buf):
	out_words = []
	for i in range(0, len(in_buf), 4):
		out_words.append(in_buf[i:i+4])
	out_words.reverse()
	return ''.join(out_words)

def calc_hdr_hash(blk_hdr):
	hash1 = hashlib.sha256()
	hash1.update(blk_hdr)
	hash1_o = hash1.digest()

	hash2 = hashlib.sha256()
	hash2.update(hash1_o)
	hash2_o = hash2.digest()

	return hash2_o

def calc_hash_str(blk_hdr):
	hash = calc_hdr_hash(blk_hdr)
	hash = bufreverse(hash)
	hash = wordreverse(hash)
	hash_str = hash.encode('hex')
	return hash_str

def get_blk_year(blk_hdr):
	members = struct.unpack("<I", blk_hdr[68:68+4])
	dt = datetime.datetime.fromtimestamp(members[0])
	return dt.year

def get_block_hashes(settings):
	blkindex = []
	f = open(settings['hashlist'], "r")
	for line in f:
		line = line.rstrip()
		blkindex.append(line)

	print("Read " + str(len(blkindex)) + " hashes")

	return blkindex

def mkblockset(blkindex):
	blkmap = {}
	for hash in blkindex:
		blkmap[hash] = True
	return blkmap

def copydata(settings, blkindex, blkset):
	inFn = 0
	inF = None
	outFn = 0
	outsz = 0
	outF = None
	blkCount = 0

	lastYear = 0
	splitYear = False
	fileOutput = True
	maxOutSz = settings['max_out_sz']
	if 'output' in settings:
		fileOutput = False
	if settings['split_year'] != 0:
		splitYear = True

	while True:
		if not inF:
			fname = "%s/blk%05d.dat" % (settings['input'], inFn)
			print("Input file" + fname)
			inF = open(fname, "rb")

		inhdr = inF.read(8)
		if (not inhdr or (inhdr[0] == "\0")):
			inF.close()
			inF = None
			inFn = inFn + 1
			continue

		inMagic = inhdr[:4]
		if (inMagic != settings['netmagic']):
			print("Invalid magic:" + inMagic)
			return
		inLenLE = inhdr[4:]
		su = struct.unpack("<I", inLenLE)
		inLen = su[0]
		rawblock = inF.read(inLen)
		blk_hdr = rawblock[:80]

		hash_str = calc_hash_str(blk_hdr)
		if not hash_str in blkset:
			print("Skipping unknown block " + hash_str)
			continue

		if not fileOutput and ((outsz + inLen) > maxOutSz):
			outF.close()
			outF = None
			outFn = outFn + 1
			outsz = 0

		if splitYear:
			blkYear = get_blk_year(blk_hdr)
			if blkYear > lastYear:
				print("New year " + str(blkYear) + " @ " + hash_str)
				lastYear = blkYear
				if outF:
					outF.close()
					outF = None
					outFn = outFn + 1
					outsz = 0

		if not outF:
			if fileOutput:
				fname = settings['output_file']
			else:
				fname = "%s/blk%05d.dat" % (settings['output'], outFn)
			print("Output file" + fname)
			outF = open(fname, "wb")

		outF.write(inhdr)
		outF.write(rawblock)
		outsz = outsz + inLen + 8

		blkCount = blkCount + 1

		if (blkCount % 1000) == 0:
			print("Wrote " + str(blkCount) + " blocks")

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print "Usage: linearize-data.py CONFIG-FILE"
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
	if 'input' not in settings:
		settings['input'] = 'input'
	if 'hashlist' not in settings:
		settings['hashlist'] = 'hashlist.txt'
	if 'split_year' not in settings:
		settings['split_year'] = 0
	if 'max_out_sz' not in settings:
		settings['max_out_sz'] = 1000L * 1000 * 1000

	settings['max_out_sz'] = long(settings['max_out_sz'])
	settings['split_year'] = int(settings['split_year'])
	settings['netmagic'] = settings['netmagic'].decode('hex')

	if 'output_file' not in settings and 'output' not in settings:
		print("Missing output file / directory")
		sys.exit(1)

	blkindex = get_block_hashes(settings)
	blkset = mkblockset(blkindex)

	if not "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f" in blkset:
		print("not found")
	else:
		copydata(settings, blkindex, blkset)


