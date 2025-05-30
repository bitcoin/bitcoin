#!/usr/bin/env python3
#
# linearize-data.py: Construct a linear, no-fork version of the chain.
#
# Copyright (c) 2013-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
#

import struct
import re
import os
import os.path
import sys
import hashlib
import datetime
import time
import glob
from collections import namedtuple

settings = {}

def calc_hash_str(blk_hdr):
    blk_hdr_hash = hashlib.sha256(hashlib.sha256(blk_hdr).digest()).digest()
    return blk_hdr_hash[::-1].hex()

def get_blk_dt(blk_hdr):
    members = struct.unpack("<I", blk_hdr[68:68+4])
    nTime = members[0]
    dt = datetime.datetime.fromtimestamp(nTime)
    dt_ym = datetime.datetime(dt.year, dt.month, 1)
    return (dt_ym, nTime)

# When getting the list of block hashes, undo any byte reversals.
def get_block_hashes(settings):
    blkindex = []
    with open(settings['hashlist'], "r", encoding="utf8") as f:
        for line in f:
            line = line.rstrip()
            if settings['rev_hash_bytes'] == 'true':
                line = bytes.fromhex(line)[::-1].hex()
            blkindex.append(line)

    print("Read " + str(len(blkindex)) + " hashes")

    return blkindex

# The block map shouldn't give or receive byte-reversed hashes.
def mkblockmap(blkindex):
    blkmap = {}
    for height,hash in enumerate(blkindex):
        blkmap[hash] = height
    return blkmap

# This gets the first block file ID that exists from the input block
# file directory.
def getFirstBlockFileId(block_dir_path):
    # First, this sets up a pattern to search for block files, for
    # example 'blkNNNNN.dat'.
    blkFilePattern = os.path.join(block_dir_path, "blk[0-9][0-9][0-9][0-9][0-9].dat")

    # This search is done with glob
    blkFnList = glob.glob(blkFilePattern)

    if len(blkFnList) == 0:
        print("blocks not pruned - starting at 0")
        return 0
    # We then get the lexicographic minimum, which should be the first
    # block file name.
    firstBlkFilePath = min(blkFnList)
    firstBlkFn = os.path.basename(firstBlkFilePath)

    # now, the string should be ['b','l','k','N','N','N','N','N','.','d','a','t']
    # So get the ID by choosing:              3   4   5   6   7
    # The ID is not necessarily 0 if this is a pruned node.
    blkId = int(firstBlkFn[3:8])
    return blkId

def read_xor_key(blocks_path):
    NUM_XOR_BYTES = 8  # From InitBlocksdirXorKey::xor_key.size()
    try:
        xor_filename = os.path.join(blocks_path, "xor.dat")
        with open(xor_filename, "rb") as xor_file:
            return xor_file.read(NUM_XOR_BYTES)
    # support also blockdirs created with pre-v28 versions, where no xor key exists yet
    except FileNotFoundError:
        return bytes([0] * NUM_XOR_BYTES)

# Block header and extent on disk
BlockExtent = namedtuple('BlockExtent', ['fn', 'offset', 'inhdr', 'blkhdr', 'size'])

class BlockDataCopier:
    def __init__(self, settings, blkindex, blkmap):
        self.settings = settings
        self.blkindex = blkindex
        self.blkmap = blkmap

        # Get first occurring block file id - for pruned nodes this
        # will not necessarily be 0
        self.inFn = getFirstBlockFileId(self.settings['input'])
        self.inF = None
        self.outFn = 0
        self.outsz = 0
        self.outF = None
        self.outFname = None
        self.blkCountIn = 0
        self.blkCountOut = 0
        self.xor_key = read_xor_key(self.settings['input'])

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

    def read_xored(self, f, size):
        offset = f.tell()
        data = bytearray(f.read(size))
        for i in range(len(data)):
            data[i] ^= self.xor_key[(i + offset) % len(self.xor_key)]
        return bytes(data)

    def writeBlock(self, inhdr, blk_hdr, rawblock):
        blockSizeOnDisk = len(inhdr) + len(blk_hdr) + len(rawblock)
        if not self.fileOutput and ((self.outsz + blockSizeOnDisk) > self.maxOutSz):
            self.outF.close()
            if self.setFileTime:
                os.utime(self.outFname, (int(time.time()), self.highTS))
            self.outF = None
            self.outFname = None
            self.outFn = self.outFn + 1
            self.outsz = 0

        (blkDate, blkTS) = get_blk_dt(blk_hdr)
        if self.timestampSplit and (blkDate > self.lastDate):
            print("New month " + blkDate.strftime("%Y-%m") + " @ " + self.hash_str)
            self.lastDate = blkDate
            if self.outF:
                self.outF.close()
                if self.setFileTime:
                    os.utime(self.outFname, (int(time.time()), self.highTS))
                self.outF = None
                self.outFname = None
                self.outFn = self.outFn + 1
                self.outsz = 0

        if not self.outF:
            if self.fileOutput:
                self.outFname = self.settings['output_file']
            else:
                self.outFname = os.path.join(self.settings['output'], "blk%05d.dat" % self.outFn)
            print("Output file " + self.outFname)
            self.outF = open(self.outFname, "wb")

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
        return os.path.join(self.settings['input'], "blk%05d.dat" % fn)

    def fetchBlock(self, extent):
        '''Fetch block contents from disk given extents'''
        with open(self.inFileName(extent.fn), "rb") as f:
            f.seek(extent.offset)
            return self.read_xored(f, extent.size)

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
                print("Input file " + fname)
                try:
                    self.inF = open(fname, "rb")
                except IOError:
                    print("Premature end of block data")
                    return

            inhdr = self.read_xored(self.inF, 8)
            if (not inhdr or (inhdr[0] == "\0")):
                self.inF.close()
                self.inF = None
                self.inFn = self.inFn + 1
                continue

            inMagic = inhdr[:4]
            if (inMagic != self.settings['netmagic']):
                # Seek backwards 7 bytes (skipping the first byte in the previous search)
                # and continue searching from the new position if the magic bytes are not
                # found.
                self.inF.seek(-7, os.SEEK_CUR)
                continue
            inLenLE = inhdr[4:]
            su = struct.unpack("<I", inLenLE)
            inLen = su[0] - 80 # length without header
            blk_hdr = self.read_xored(self.inF, 80)
            inExtent = BlockExtent(self.inFn, self.inF.tell(), inhdr, blk_hdr, inLen)

            self.hash_str = calc_hash_str(blk_hdr)
            if not self.hash_str in blkmap:
                # Because blocks can be written to files out-of-order as of 0.10, the script
                # may encounter blocks it doesn't know about. Treat as debug output.
                if settings['debug_output'] == 'true':
                    print("Skipping unknown block " + self.hash_str)
                self.inF.seek(inLen, os.SEEK_CUR)
                continue

            blkHeight = self.blkmap[self.hash_str]
            self.blkCountIn += 1

            if self.blkCountOut == blkHeight:
                # If in-order block, just copy
                rawblock = self.read_xored(self.inF, inLen)
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
                    self.outOfOrderData[blkHeight] = self.read_xored(self.inF, inLen)
                    self.outOfOrderSize += inLen
                else: # If no space in cache, seek forward
                    self.inF.seek(inLen, os.SEEK_CUR)

        print("Done (%i blocks written)" % (self.blkCountOut))

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: linearize-data.py CONFIG-FILE")
        sys.exit(1)

    with open(sys.argv[1], encoding="utf8") as f:
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

    # Force hash byte format setting to be lowercase to make comparisons easier.
    # Also place upfront in case any settings need to know about it.
    if 'rev_hash_bytes' not in settings:
        settings['rev_hash_bytes'] = 'false'
    settings['rev_hash_bytes'] = settings['rev_hash_bytes'].lower()

    if 'netmagic' not in settings:
        settings['netmagic'] = 'f9beb4d9'
    if 'genesis' not in settings:
        settings['genesis'] = '000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f'
    if 'input' not in settings:
        settings['input'] = 'input'
    if 'hashlist' not in settings:
        settings['hashlist'] = 'hashlist.txt'
    if 'file_timestamp' not in settings:
        settings['file_timestamp'] = 0
    if 'split_timestamp' not in settings:
        settings['split_timestamp'] = 0
    if 'max_out_sz' not in settings:
        settings['max_out_sz'] = 1000 * 1000 * 1000
    if 'out_of_order_cache_sz' not in settings:
        settings['out_of_order_cache_sz'] = 100 * 1000 * 1000
    if 'debug_output' not in settings:
        settings['debug_output'] = 'false'

    settings['max_out_sz'] = int(settings['max_out_sz'])
    settings['split_timestamp'] = int(settings['split_timestamp'])
    settings['file_timestamp'] = int(settings['file_timestamp'])
    settings['netmagic'] = bytes.fromhex(settings['netmagic'])
    settings['out_of_order_cache_sz'] = int(settings['out_of_order_cache_sz'])
    settings['debug_output'] = settings['debug_output'].lower()

    if 'output_file' not in settings and 'output' not in settings:
        print("Missing output file / directory")
        sys.exit(1)

    blkindex = get_block_hashes(settings)
    blkmap = mkblockmap(blkindex)

    # Block hash map won't be byte-reversed. Neither should the genesis hash.
    if not settings['genesis'] in blkmap:
        print("Genesis block not found in hashlist")
    else:
        BlockDataCopier(settings, blkindex, blkmap).run()
