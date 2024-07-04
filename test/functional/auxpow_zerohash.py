#!/usr/bin/env python3
# Copyright (c) 2019 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests that the "hashBlock" field of the Merkle coinbase tx in an auxpow
# is zero'ed when the block is processed/retrieved from a node, and that
# auxpows with a zero hashBlock are actually accepted just fine.  (This has
# always been the case, but the test just makes sure this is explicitly
# tested for the future as well.)

from test_framework.test_framework import SyscoinTestFramework
from test_framework.blocktools import (
  create_block,
  create_coinbase,
)
from test_framework.messages import (
  CAuxPow,
  CBlock,
  CInv,
  msg_getdata,
  uint256_from_compact,
)
from test_framework.p2p import (
  P2PDataStore,
  P2PInterface,
)
from test_framework.util import (
  assert_equal,
  wait_until_helper_internal,
)

from test_framework.auxpow_testing import computeAuxpow

from io import BytesIO


class P2PBlockGetter (P2PInterface):
  """
  P2P connection class that allows requesting blocks from a node's P2P
  connection to verify how they are sent.
  """

  def on_block (self, msg):
    self.block = msg.block

  def getBlock (self, blkHash):
    self.block = None
    inv = CInv (t=2, h=int (blkHash, 16))
    self.send_message (msg_getdata (inv=[inv]))
    wait_until_helper_internal (lambda: self.block is not None)
    return self.block


class AuxpowZeroHashTest (SyscoinTestFramework):

  def set_test_params (self):
    self.num_nodes = 1
    self.setup_clean_chain = True
    self.extra_args = [["-whitelist=127.0.0.1"]]

  def run_test (self):
    node = self.nodes[0]
    p2pStore = node.add_p2p_connection (P2PDataStore ())
    p2pGetter = node.add_p2p_connection (P2PBlockGetter ())

    self.log.info ("Adding a block with non-zero hash in the auxpow...")
    blk, blkHash = self.createBlock ()
    blk.auxpow.hashBlock = 12345678
    blkHex = blk.serialize ().hex ()
    assert_equal (node.submitblock (blkHex), None)
    assert_equal (node.getbestblockhash (), blkHash)

    self.log.info ("Retrieving block through RPC...")
    gotHex = node.getblock (blkHash, 0)
    assert gotHex != blkHex
    gotBlk = CBlock ()
    gotBlk.deserialize (BytesIO (bytes.fromhex(gotHex)))
    assert_equal (gotBlk.auxpow.hashBlock, 0)

    self.log.info ("Retrieving block through P2P...")
    gotBlk = p2pGetter.getBlock (blkHash)
    assert_equal (gotBlk.auxpow.hashBlock, 0)

    self.log.info ("Sending zero-hash auxpow through RPC...")
    blk, blkHash = self.createBlock ()
    blk.auxpow.hashBlock = 0
    assert_equal (node.submitblock (blk.serialize ().hex ()), None)
    assert_equal (node.getbestblockhash (), blkHash)

    self.log.info ("Sending zero-hash auxpow through P2P...")
    blk, blkHash = self.createBlock ()
    blk.auxpow.hashBlock = 0
    p2pStore.send_blocks_and_test ([blk], node, success=True)
    assert_equal (node.getbestblockhash (), blkHash)

    self.log.info ("Sending non-zero nIndex auxpow through RPC...")
    blk, blkHash = self.createBlock ()
    blk.auxpow.nIndex = 42
    assert_equal (node.submitblock (blk.serialize ().hex ()), None)
    assert_equal (node.getbestblockhash (), blkHash)

    self.log.info ("Sending non-zero nIndex auxpow through P2P...")
    blk, blkHash = self.createBlock ()
    blk.auxpow.nIndex = 42
    p2pStore.send_blocks_and_test ([blk], node, success=True)
    assert_equal (node.getbestblockhash (), blkHash)

  def createBlock (self):
    """
    Creates and mines a new block with auxpow.
    """

    bestHash = self.nodes[0].getbestblockhash ()
    bestBlock = self.nodes[0].getblock (bestHash)
    tip = int (bestHash, 16)
    height = bestBlock["height"] + 1
    time = bestBlock["time"] + 1

    block = create_block (tip, create_coinbase (height), time)
    block.mark_auxpow ()
    block.rehash ()
    newHash = "%064x" % block.sha256

    target = b"%064x" % uint256_from_compact (block.nBits)
    auxpowHex = computeAuxpow (newHash, target, True)
    block.auxpow = CAuxPow ()
    block.auxpow.deserialize (BytesIO (bytes.fromhex(auxpowHex)))

    return block, newHash


if __name__ == '__main__':
  AuxpowZeroHashTest ().main ()
