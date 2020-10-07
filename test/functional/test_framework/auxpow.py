#!/usr/bin/env python3
# Copyright (c) 2014-2018 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# Basic code for working with auxpow.  This is used for the regtests (e.g. from
# auxpow_testing.py), but also for contrib/auxpow/getwork-wrapper.py.

import binascii
import codecs
import hashlib

def constructAuxpow (block):
  """
  Starts to construct a minimal auxpow, ready to be mined.  Returns the fake
  coinbase tx and the unmined parent block header as hex strings.
  """

  block = codecs.encode (block, 'ascii')

  # Start by building the merge-mining coinbase.  The merkle tree
  # consists only of the block hash as root.
  coinbase = b"fabe" + binascii.hexlify (b"m" * 2)
  coinbase += block
  coinbase += b"01000000" + (b"00" * 4)

  # Construct "vector" of transaction inputs.
  vin = b"01"
  vin += (b"00" * 32) + (b"ff" * 4)
  vin += codecs.encode ("%02x" % (len (coinbase) // 2), "ascii") + coinbase
  vin += (b"ff" * 4)

  # Build up the full coinbase transaction.  It consists only
  # of the input and has no outputs.
  tx = b"01000000" + vin + b"00" + (b"00" * 4)
  txHash = doubleHashHex (tx)

  # Construct the parent block header.  It need not be valid, just good
  # enough for auxpow purposes.
  header = b"01000000"
  header += b"00" * 32
  header += reverseHex (txHash)
  header += b"00" * 4
  header += b"00" * 4
  header += b"00" * 4

  return (tx.decode ('ascii'), header.decode ('ascii'))

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
