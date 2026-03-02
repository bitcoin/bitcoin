#!/usr/bin/env python3
# Copyright (c) 2014-2018 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Basic code for working with auxpow.  This is used for the regtests (e.g. from
# auxpow_testing.py), but also for contrib/auxpow/getwork-wrapper.py.

import binascii
import codecs
import hashlib

def constructAuxpow(block, auxpowtag_script, parent_prev_hash=None):
  """
  Starts to construct a minimal auxpow, ready to be mined.  Returns the
  coinbase tx (with one OP_RETURN output containing a Syscoin-like commitment)
  and the unmined parent block header as hex strings.

  This differs minimally from the old version: we only add a vout with a single
  OP_RETURN to hold "SYS" + 32 block bytes + 4 height bytes.
  """

  # Convert the block from ASCII to bytes for hex manipulation
  block = codecs.encode(block, 'ascii')

  # Original partial coinbase script for merged mining:
  # "fa be 'm' 'm'"
  coinbase = b"fabe" + binascii.hexlify(b"m" * 2)
  coinbase += block
  coinbase += b"01000000" + (b"00" * 4)

  # Construct "vector" of transaction inputs as before
  vin = b"01"
  vin += (b"00" * 32) + (b"ff" * 4)
  # scriptSig length
  vin += codecs.encode("%02x" % (len(coinbase) // 2), "ascii") + coinbase
  vin += (b"ff" * 4)

  # Construct tx outputs (vout)
  vout = b"01"  # one output
  vout += b"0000000000000000"  # value=0 (coinbase)

  # Use provided auxpowtag directly as scriptPubKey
  vout += codecs.encode("%02x" % (len(auxpowtag_script) // 2), "ascii") + codecs.encode(auxpowtag_script, 'ascii')

  # Build the full coinbase tx (version=01000000, vin, vout, locktime=00000000)
  tx = b"01000000" + vin + vout + b"00000000"
  txHash = doubleHashHex(tx)

  # Construct the parent block header
  header  = b"01000000"
  if parent_prev_hash is None:
    header += b"00" * 32
  else:
    # Header fields are serialized little-endian in the hex payload.
    header += reverseHex(parent_prev_hash)
  # The coinbase is the only tx, so merkle root is the coinbase txid reversed
  header += reverseHex(txHash)
  header += b"00" * 4
  header += b"00" * 4
  header += b"00" * 4

  return (tx.decode("ascii"), header.decode("ascii"))

def finishAuxpow (tx, header):
  """
  Constructs the finished auxpow hex string based on the mined header.
  """

  blockhash = doubleHashHex (header)

  # Build the MerkleTx part of the auxpow.
  auxpow = codecs.encode (tx, 'ascii')
  auxpow += blockhash
  auxpow += b"00"
  auxpow += b"00" * 4

  # Extend to full auxpow.
  auxpow += b"00"
  auxpow += b"00" * 4
  auxpow += header

  return auxpow.decode ("ascii")

def doubleHashHex (data):
  """
  Perform Bitcoin's Double-SHA256 hash on the given hex string.
  """

  hasher = hashlib.sha256 ()
  hasher.update (binascii.unhexlify (data))
  data = hasher.digest ()

  hasher = hashlib.sha256 ()
  hasher.update (data)

  return reverseHex (hasher.hexdigest ())

def reverseHex (data):
  """
  Flip byte order in the given data (hex string).
  """

  b = bytearray (binascii.unhexlify (data))
  b.reverse ()

  return binascii.hexlify (b)

def getworkByteswap (data):
  """
  Run the byte-order swapping step necessary for working with getwork.
  """

  data = bytearray (data)
  assert len (data) % 4 == 0
  for i in range (0, len (data), 4):
    data[i], data[i + 3] = data[i + 3], data[i]
    data[i + 1], data[i + 2] = data[i + 2], data[i + 1]

  return data
