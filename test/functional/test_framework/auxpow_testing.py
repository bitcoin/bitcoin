#!/usr/bin/env python3
# Copyright (c) 2014-2019 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Utility routines for auxpow that are needed specifically by the regtests.
# This is mostly about actually *solving* an auxpow block (with regtest
# difficulty) or inspecting the information for verification.

import binascii

from test_framework import auxpow
# SYSCOIN
from test_framework.authproxy import JSONRPCException

# 32-byte zero hash, hex-encoded.
ZERO_HASH_HEX = "00" * 32
# Non-null BTCPREV value for tests that require a commitment.
NON_NULL_BTCPREV_HASH_HEX = "11" * 32

def createAuxBlockWithBTCPREVIfRequired(node, addr):
  """
  Wrapper around node.createauxblock that supplies btcprevhash when the node
  requires it (BTCPREV sign-offset blocks).
  """
  try:
    return node.createauxblock(addr)
  except JSONRPCException as e:
    msg = ""
    try:
      msg = e.error.get("message", "")
    except Exception:
      msg = str(e)
    if "btcprevhash is required at this height" in msg:
      auxblock = node.createauxblock(addr, NON_NULL_BTCPREV_HASH_HEX)
      auxblock["_btcprevhash"] = NON_NULL_BTCPREV_HASH_HEX
      return auxblock
    raise

# SYSCOIN
def computeAuxpow (block, target, ok, auxpowtag_script, parent_prev_hash=ZERO_HASH_HEX):
  """
  Build an auxpow object (serialised as hex string) that solves
  (ok = True) or doesn't solve (ok = False) the block.
  """

  (tx, header) = auxpow.constructAuxpow(block, auxpowtag_script, parent_prev_hash)
  (header, _) = mineBlock (header, target, ok)
  return auxpow.finishAuxpow (tx, header)

def mineAuxpowBlock (node, wallet):
  """
  Mine an auxpow block on the given RPC connection.  This uses the
  createauxblock and submitauxblock command pair.
  """

  def create ():
    if wallet is None:
      try:
        addr = node.getnewaddress ()
      except JSONRPCException as e:
        code = None
        try:
          code = e.error.get("code")
        except Exception:
          pass
        # Some test nodes run with wallet RPC disabled.
        # Use the framework deterministic address in that case.
        if code == -32601:
          addr = node.get_deterministic_priv_key().address
        else:
          raise
    else:
      addr = wallet.get_address()
    return createAuxBlockWithBTCPREVIfRequired(node, addr)

  return mineAuxpowBlockWithMethods (create, node.submitauxblock)

def mineAuxpowBlockWithMethods (create, submit):
  """
  Mine an auxpow block, using the given methods for creation and submission.
  """

  try:
    auxblock = create ()
  except JSONRPCException as e:
    msg = ""
    try:
      msg = e.error.get("message", "")
    except Exception:
      msg = str(e)
    if "btcprevhash is required at this height" in msg:
      # Try again assuming the create method supports passing btcprevhash as a single argument
      # (e.g. wallet RPC getauxblock(btcprevhash)).
      auxblock = create(ZERO_HASH_HEX)
    else:
      raise
  target = auxpow.reverseHex (auxblock['_target'])
  parent_prev_hash = auxblock.get("_btcprevhash", ZERO_HASH_HEX)
  # SYSCOIN
  apow = computeAuxpow(
      auxblock['hash'],
      target,
      True,
      auxblock['coinbasescript'],
      parent_prev_hash,
  )
  res = submit (auxblock['hash'], apow)
  assert res

  return auxblock['hash']

def getCoinbaseAddr (node, blockHash):
    """
    Extract the coinbase tx' payout address for the given block.
    """

    blockData = node.getblock (blockHash)
    txn = blockData['tx']
    assert len (txn) >= 1

    txData = node.getrawtransaction (txn[0], True, blockHash)
    assert len (txData['vout']) >= 1 and len (txData['vin']) == 1
    assert 'coinbase' in txData['vin'][0]

    addr = txData['vout'][0]['scriptPubKey']['address']
    assert len (addr) > 0
    return addr

def mineBlock (header, target, ok):
  """
  Given a block header, update the nonce until it is ok (or not)
  for the given target.
  """

  data = bytearray (binascii.unhexlify (header))
  while True:
    assert data[79] < 255
    data[79] += 1
    hexData = binascii.hexlify (data)

    blockhash = auxpow.doubleHashHex (hexData)
    if (ok and blockhash < target) or ((not ok) and blockhash > target):
      break

  return (hexData, blockhash)
