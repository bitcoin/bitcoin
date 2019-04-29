#!/usr/bin/env python3
# Copyright (c) 2018 Daniel Kraft
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

# This is a simple wrapper around the merge-mining interface of the core
# daemon (createauxblock and submitauxblock).  It handles the creation of
# a "fake" auxpow, providing an external miner with a getwork-like interface.
#
# Since using this loses the ability to *actually* merge mine with a parent
# chain, this is likely not very useful in production.  But it can be used
# for testing and debugging.
#
# This needs jsonrpclib, which can be found in the 'python-jsonrpclib' Debian
# package or at https://github.com/joshmarshall/jsonrpclib.  It also imports
# auxpow from test/functional/test_framework.

import codecs
import optparse
import struct
from xmlrpclib import ProtocolError

import jsonrpclib
from jsonrpclib.SimpleJSONRPCServer import SimpleJSONRPCServer

import auxpow


class GetworkWrapper:
  """
  The main server class.  It sets up a JSON-RPC server and handles requests
  coming in.
  """

  def __init__ (self, backend, host, port):
    self.backend = backend
    self.server = SimpleJSONRPCServer ((host, port))

    def getwork (data=None):
      if data is None:
        return self.createWork ()
      return self.submitWork (data)
    self.server.register_function (getwork)

    # We use our own extra nonce to not return the same work twice if
    # asked again for new work.
    self.extraNonce = 0

    # Dictionary that holds all created works so they can be retrieved when
    # necessary for matching.  The key is the (byte-order swapped) block merkle
    # hash, which is not changed by the miner when processing the work.
    # This is cleared only once a submitted block was accepted.  We do not try
    # to detect if the chain tip changes externally.
    self.works = {}

  def keyForWork (self, data):
    """
    Returns the key used in self.works for the given, hex-encoded and
    byte-swapped, getwork 'data'.
    """

    return data[2*36 : 2*68]

  def createWork (self):
    auxblock = self.backend.getauxblock ()
    (tx, hdr) = auxpow.constructAuxpow (auxblock['hash'])

    en = self.extraNonce
    self.extraNonce = (self.extraNonce + 1) % (1 << 32)

    hdrBytes = bytearray (codecs.decode (hdr, 'hex_codec'))
    hdrBytes[0:4] = struct.pack ('<I', en)
    formatted = auxpow.getworkByteswap (hdrBytes)
    formatted += bytearray ([0] * (128 - len (formatted)))
    formatted[83] = 0x80
    formatted[-4] = 0x80
    formatted[-3] = 0x02
    work = codecs.encode (formatted, 'hex_codec')

    self.works[self.keyForWork (work)] = {"auxblock": auxblock, "tx": tx}

    return {"data": work, "target": auxblock['_target']}

  def submitWork (self, data):
    key = self.keyForWork (data)
    if not key in self.works:
      print ('Error: stale / unknown work submitted')
      return False
    w = self.works[key]

    dataBytes = codecs.decode (data, 'hex_codec')
    fixedBytes = auxpow.getworkByteswap (dataBytes[:80])
    hdrHex = codecs.encode (fixedBytes, 'hex_codec')

    auxpowHex = auxpow.finishAuxpow (w['tx'], hdrHex)
    try:
      res = self.backend.submitauxblock (w['auxblock']['hash'], auxpowHex)
    except ProtocolError as exc:
      print ('Error submitting work: %s' % exc)
      return False

    # Clear cache of created works when a new block was accepted.
    if res:
      self.works = {}

    return res

  def serve (self):
    self.server.serve_forever ()


if __name__ == '__main__':
  parser = optparse.OptionParser (usage="%prog [options]")
  parser.add_option ("--backend-url", dest="backend",
                     help="URL for the backend daemon connection")
  parser.add_option ("--port", dest="port", type="int",
                     help="Port on which to serve")
  (options, _) = parser.parse_args ()
  if options.backend is None or options.port is None:
    parser.error ("--backend-url and --port must be specified")

  backend = jsonrpclib.Server (options.backend)
  server = GetworkWrapper (backend, 'localhost', options.port)
  server.serve ()
