#!/usr/bin/env python3
# Copyright (c) 2019 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Tests what happens if we submit a valid block with invalid auxpow.  Obviously
# the block should not be accepted, but it should also not be marked as
# permanently invalid.  So resubmitting the same block with a valid auxpow
# should then work fine.

from test_framework.test_framework import SyscoinTestFramework
from test_framework.blocktools import (
  create_block,
  create_coinbase,
)
from test_framework.messages import (
  CAuxPow,
  uint256_from_compact,
)
from test_framework.p2p import P2PDataStore
from test_framework.util import (
  assert_equal,
)

from test_framework.auxpow_testing import computeAuxpow

from io import BytesIO

class AuxpowInvalidPoWTest (SyscoinTestFramework):

  def set_test_params (self):
    self.num_nodes = 1
    self.setup_clean_chain = True
    self.extra_args = [["-whitelist=127.0.0.1"]]

  def run_test (self):
    node = self.nodes[0]
    node.add_p2p_connection (P2PDataStore ())

    self.log.info ("Sending block with invalid auxpow over P2P...")
    tip = node.getbestblockhash ()
    blk, blkHash = self.createBlock ()
    blk = self.addAuxpow (blk, blkHash, False)
    node.p2ps[0].send_blocks_and_test ([blk], node, force_send=True,
                                   success=False, reject_reason="high-hash")
    assert_equal (node.getbestblockhash (), tip)

    self.log.info ("Sending the same block with valid auxpow...")
    blk = self.addAuxpow (blk, blkHash, True)
    node.p2ps[0].send_blocks_and_test ([blk], node, success=True)
    assert_equal (node.getbestblockhash (), blkHash)

    self.log.info ("Submitting block with invalid auxpow...")
    tip = node.getbestblockhash ()
    blk, blkHash = self.createBlock ()
    blk = self.addAuxpow (blk, blkHash, False)
    assert_equal (node.submitblock (blk.serialize ().hex ()), "high-hash")
    assert_equal (node.getbestblockhash (), tip)

    self.log.info ("Submitting block with valid auxpow...")
    blk = self.addAuxpow (blk, blkHash, True)
    assert_equal (node.submitblock (blk.serialize ().hex ()), None)
    assert_equal (node.getbestblockhash (), blkHash)

  def createBlock (self):
    """
    Creates a new block that is valid for the current tip.  It is marked as
    auxpow, but the auxpow is not yet filled in.
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

    return block, newHash


  def addAuxpow (self, block, blkHash, ok):
    """
    Fills in the auxpow for the given block message.  It is either
    chosen to be valid (ok = True) or invalid (ok = False).
    """

    target = b"%064x" % uint256_from_compact (block.nBits)
    auxpowHex = computeAuxpow (blkHash, target, ok)
    block.auxpow = CAuxPow ()
    block.auxpow.deserialize (BytesIO (bytes.fromhex(auxpowHex)))

    return block

if __name__ == '__main__':
  AuxpowInvalidPoWTest ().main ()
