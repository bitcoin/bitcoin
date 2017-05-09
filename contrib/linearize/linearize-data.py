#!/usr/bin/python
#
# linearize-data.py: Construct a linear, no-fork version of the chain.
#
# Copyright (c) 2013-2014 The Bitcoin developers
# Distributed under the MIT/X11 software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

from __future__ import print_function, division
import json
import struct
import re
import os
import base64
import httplib
import sys
import hashlib
import datetime
import time
from collections import namedtuple

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

def get_blk_dt(blk_hdr):
	members = struct.unpack("<I", blk_hdr[68:68+4])
	nTime = members[0]
	dt = datetime.datetime.fromtimestamp(nTime)
	dt_ym = datetime.datetime(dt.year, dt.month, 1)
	return (dt_ym, nTime)

def get_block_hashes(settings):
	blkindex = []
	f = open(settings['hashlist'], "r")
	for line in f:
		line = line.rstrip()
		blkindex.append(line)

	print("Read " + str(len(blkindex)) + " hashes")

	return blkindex

def mkblockmap(blkindex):
	blkmap = {}
	for height,hash in enumerate(blkindex):
		blkmap[hash] = height
	return blkmap

# Block header and extent on disk
BlockExtent = namedtuple('BlockExtent', ['fn', 'offset', 'inhdr', 'blkhdr', 'size'])

class BlockDataCopier:
	def __init__(self, settings, blkindex, blkmap):
		self.settings = settings
		self.blkindex = blkindex
		self.blkmap = blkmap

		self.inFn = 0
		self.inF = None
		self.outFn = 0
		self.outsz = 0
		self.outF = None
		self.outFname = None
		self.blkCountIn = 0
		self.blkCountOut = 0

		self.lastDate = datetime.datetime(2000, 1, 1)
		self.highTS = 1408893517 - 315360000
		self.timestampSplit = False
		self.fileOutput = True
		self.setFileTime = False
		self.maxOutSz = settings['max_out_sz']
		if 'output' in settings:
			self.fileOutput = False
		if settings['file_timestamp'] != 0:
			self.setFileTime = True
		if settings['split_timestamp'] != 0:
			self.timestampSplit = True
        # Extents and cache for out-of-order blocks
		self.blockExtents = {}
		self.outOfOrderData = {}
		self.outOfOrderSize = 0 # running total size for items in outOfOrderData

	def writeBlock(self, inhdr, blk_hdr, rawblock):
		if not self.fileOutput and ((self.outsz + self.inLen) > self.maxOutSz):
			self.outF.close()
			if self.setFileTime:
				os.utime(outFname, (int(time.time()), highTS))
			self.outF = None
			self.outFname = None
			self.outFn = outFn + 1
			self.outsz = 0

		(blkDate, blkTS) = get_blk_dt(blk_hdr)
		if self.timestampSplit and (blkDate > self.lastDate):
			print("New month " + blkDate.strftime("%Y-%m") + " @ " + hash_str)
			lastDate = blkDate
			if outF:
				outF.close()
				if setFileTime:
					os.utime(outFname, (int(time.time()), highTS))
				self.outF = None
				self.outFname = None
				self.outFn = self.outFn + 1
				self.outsz = 0

		if not self.outF:
			if self.fileOutput:
				outFname = self.settings['output_file']
			else:
				outFname = "%s/blk%05d.dat" % (self.settings['output'], outFn)
			print("Output file" + outFname)
			self.outF = open(outFname, "wb")

		self.outF.write(inhdr)
		self.outF.write(blk_hdr)
		self.outF.write(rawblock)
		self.outsz = self.outsz + len(inhdr) + len(blk_hdr) + len(rawblock)

		self.blkCountOut = self.blkCountOut + 1
		if blkTS > self.highTS:
			self.highTS = blkTS

		if (self.blkCountOut % 1000) == 0:
			print('%i blocks scanned, %i blocks written (of %i, %.1f%% complete)' % 
					(self.blkCountIn, self.blkCountOut, len(self.blkindex), 100.0 * self.blkCountOut / len(self.blkindex)))

	def inFileName(self, fn):
		return "%s/blk%05d.dat" % (self.settings['input'], fn)

	def fetchBlock(self, extent):
		'''Fetch block contents from disk given extents'''
		with open(self.inFileName(extent.fn), "rb") as f:
			f.seek(extent.offset)
			return f.read(extent.size)

	def copyOneBlock(self):
		'''Find the next block to be written in the input, and copy it to the output.'''
		extent = self.blockExtents.pop(self.blkCountOut)
		if self.blkCountOut in self.outOfOrderData:
			# If the data is cached, use it from memory and remove from the cache
			rawblock = self.outOfOrderData.pop(self.blkCountOut)
			self.outOfOrderSize -= len(rawblock)
		else: # Otherwise look up data on disk
			rawblock = self.fetchBlock(extent)

		self.writeBlock(extent.inhdr, extent.blkhdr, rawblock)

	def run(self):
		while self.blkCountOut < len(self.blkindex):
			if not self.inF:
				fname = self.inFileName(self.inFn)
				print("Input file" + fname)
				try:
					self.inF = open(fname, "rb")
				except IOError:
					print("Premature end of block data")
					return

			inhdr = self.inF.read(8)
			if (not inhdr or (inhdr[0] == "\0")):
				self.inF.close()
				self.inF = None
				self.inFn = self.inFn + 1
				continue

			inMagic = inhdr[:4]
			if (inMagic != self.settings['netmagic']):
				print("Invalid magic:" + inMagic)
				return
			inLenLE = inhdr[4:]
			su = struct.unpack("<I", inLenLE)
			inLen = su[0] - 80 # length without header
			blk_hdr = self.inF.read(80)
			inExtent = BlockExtent(self.inFn, self.inF.tell(), inhdr, blk_hdr, inLen)

			hash_str = calc_hash_str(blk_hdr)
			if not hash_str in blkmap:
				print("Skipping unknown block " + hash_str)
				self.inF.seek(inLen, os.SEEK_CUR)
				continue

			blkHeight = self.blkmap[hash_str]
			self.blkCountIn += 1

			if self.blkCountOut == blkHeight:
				# If in-order block, just copy
				rawblock = self.inF.read(inLen)
				self.writeBlock(inhdr, blk_hdr, rawblock)

				# See if we can catch up to prior out-of-order blocks
				while self.blkCountOut in self.blockExtents:
					self.copyOneBlock()

			else: # If out-of-order, skip over block data for now
				self.blockExtents[blkHeight] = inExtent
				if self.outOfOrderSize < self.settings['out_of_order_cache_sz']:
					# If there is space in the cache, read the data
					# Reading the data in file sequence instead of seeking and fetching it later is preferred,
					# but we don't want to fill up memory
					self.outOfOrderData[blkHeight] = self.inF.read(inLen)
					self.outOfOrderSize += inLen
				else: # If no space in cache, seek forward
					self.inF.seek(inLen, os.SEEK_CUR)

		print("Done (%i blocks written)" % (self.blkCountOut))

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print("Usage: linearize-data.py CONFIG-FILE")
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
	if 'file_timestamp' not in settings:
		settings['file_timestamp'] = 0
	if 'split_timestamp' not in settings:
		settings['split_timestamp'] = 0
	if 'max_out_sz' not in settings:
		settings['max_out_sz'] = 1000L * 1000 * 1000
	if 'out_of_order_cache_sz' not in settings:
		settings['out_of_order_cache_sz'] = 100 * 1000 * 1000

	settings['max_out_sz'] = long(settings['max_out_sz'])
	settings['split_timestamp'] = int(settings['split_timestamp'])
	settings['file_timestamp'] = int(settings['file_timestamp'])
	settings['netmagic'] = settings['netmagic'].decode('hex')
	settings['out_of_order_cache_sz'] = int(settings['out_of_order_cache_sz'])

	if 'output_file' not in settings and 'output' not in settings:
		print("Missing output file / directory")
		sys.exit(1)

	blkindex = get_block_hashes(settings)
	blkmap = mkblockmap(blkindex)

	if not "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f" in blkmap:
		print("not found")
	else:
		BlockDataCopier(settings, blkindex, blkmap).run()


