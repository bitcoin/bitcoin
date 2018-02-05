#!/usr/bin/env python3
# Copyright (c) 2015-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""BlockStore and TxStore helper classes."""

from .mininode import *
from io import BytesIO
import dbm.dumb as dbmd

logger = logging.getLogger("TestFramework.blockstore")

class BlockStore():
    """BlockStore helper class.

    BlockStore keeps a map of blocks and implements helper functions for
    responding to getheaders and getdata, and for constructing a getheaders
    message.
    """

    def __init__(self, datadir):
        self.blockDB = dbmd.open(datadir + "/blocks", 'c')
        self.currentBlock = 0
        self.headers_map = dict()

    def close(self):
        self.blockDB.close()

    def erase(self, blockhash):
        del self.blockDB[repr(blockhash)]

    # lookup an entry and return the item as raw bytes
    def get(self, blockhash):
        value = None
        try:
            value = self.blockDB[repr(blockhash)]
        except KeyError:
            return None
        return value

    # lookup an entry and return it as a CBlock
    def get_block(self, blockhash):
        ret = None
        serialized_block = self.get(blockhash)
        if serialized_block is not None:
            f = BytesIO(serialized_block)
            ret = CBlock()
            ret.deserialize(f)
            ret.calc_sha256()
        return ret

    def get_header(self, blockhash):
        try:
            return self.headers_map[blockhash]
        except KeyError:
            return None

    # Note: this pulls full blocks out of the database just to retrieve
    # the headers -- perhaps we could keep a separate data structure
    # to avoid this overhead.
    def headers_for(self, locator, hash_stop, current_tip=None):
        if current_tip is None:
            current_tip = self.currentBlock
        current_block_header = self.get_header(current_tip)
        if current_block_header is None:
            return None

        response = msg_headers()
        headersList = [ current_block_header ]
        maxheaders = 2000
        while (headersList[0].sha256 not in locator.vHave):
            prevBlockHash = headersList[0].hashPrevBlock
            prevBlockHeader = self.get_header(prevBlockHash)
            if prevBlockHeader is not None:
                headersList.insert(0, prevBlockHeader)
            else:
                break
        headersList = headersList[:maxheaders] # truncate if we have too many
        hashList = [x.sha256 for x in headersList]
        index = len(headersList)
        if (hash_stop in hashList):
            index = hashList.index(hash_stop)+1
        response.headers = headersList[:index]
        return response

    def add_block(self, block):
        block.calc_sha256()
        try:
            self.blockDB[repr(block.sha256)] = bytes(block.serialize())
        except TypeError as e:
            logger.exception("Unexpected error")
        self.currentBlock = block.sha256
        self.headers_map[block.sha256] = CBlockHeader(block)

    def add_header(self, header):
        self.headers_map[header.sha256] = header

    # lookup the hashes in "inv", and return p2p messages for delivering
    # blocks found.
    def get_blocks(self, inv):
        responses = []
        for i in inv:
            if (i.type == 2 or i.type == (2 | (1 << 30))): # MSG_BLOCK or MSG_WITNESS_BLOCK
                data = self.get(i.hash)
                if data is not None:
                    # Use msg_generic to avoid re-serialization
                    responses.append(msg_generic(b"block", data))
        return responses

    def get_locator(self, current_tip=None):
        if current_tip is None:
            current_tip = self.currentBlock
        r = []
        counter = 0
        step = 1
        lastBlock = self.get_block(current_tip)
        while lastBlock is not None:
            r.append(lastBlock.hashPrevBlock)
            for i in range(step):
                lastBlock = self.get_block(lastBlock.hashPrevBlock)
                if lastBlock is None:
                    break
            counter += 1
            if counter > 10:
                step *= 2
        locator = CBlockLocator()
        locator.vHave = r
        return locator

class TxStore():
    def __init__(self, datadir):
        self.txDB = dbmd.open(datadir + "/transactions", 'c')

    def close(self):
        self.txDB.close()

    # lookup an entry and return the item as raw bytes
    def get(self, txhash):
        value = None
        try:
            value = self.txDB[repr(txhash)]
        except KeyError:
            return None
        return value

    def add_transaction(self, tx):
        tx.calc_sha256()
        try:
            self.txDB[repr(tx.sha256)] = bytes(tx.serialize())
        except TypeError as e:
            logger.exception("Unexpected error")

    def get_transactions(self, inv):
        responses = []
        for i in inv:
            if (i.type == 1 or i.type == (1 | (1 << 30))): # MSG_TX or MSG_WITNESS_TX
                tx = self.get(i.hash)
                if tx is not None:
                    responses.append(msg_generic(b"tx", tx))
        return responses
