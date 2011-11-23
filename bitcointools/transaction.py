#
# Code for dumping a single transaction, given its ID
#

from bsddb.db import *
import logging
import os.path
import sys
import time

from BCDataStream import *
from base58 import public_key_to_bc_address
from util import short_hex
from deserialize import *

def _read_CDiskTxPos(stream):
  n_file = stream.read_uint32()
  n_block_pos = stream.read_uint32()
  n_tx_pos = stream.read_uint32()
  return (n_file, n_block_pos, n_tx_pos)

def _dump_tx(datadir, tx_hash, tx_pos):
  blockfile = open(os.path.join(datadir, "blk%04d.dat"%(tx_pos[0],)), "rb")
  ds = BCDataStream()
  ds.map_file(blockfile, tx_pos[2])
  d = parse_Transaction(ds)
  print deserialize_Transaction(d)
  ds.close_file()
  blockfile.close()

def dump_transaction(datadir, db_env, tx_id):
  """ Dump a transaction, given hexadecimal tx_id-- either the full ID
      OR a short_hex version of the id.
  """
  db = DB(db_env)
  try:
    r = db.open("blkindex.dat", "main", DB_BTREE, DB_THREAD|DB_RDONLY)
  except DBError:
    r = True

  if r is not None:
    logging.error("Couldn't open blkindex.dat/main.  Try quitting any running Bitcoin apps.")
    sys.exit(1)

  kds = BCDataStream()
  vds = BCDataStream()

  n_tx = 0
  n_blockindex = 0

  key_prefix = "\x02tx"+(tx_id[-4:].decode('hex_codec')[::-1])
  cursor = db.cursor()
  (key, value) = cursor.set_range(key_prefix)

  while key.startswith(key_prefix):
    kds.clear(); kds.write(key)
    vds.clear(); vds.write(value)

    type = kds.read_string()
    hash256 = (kds.read_bytes(32))
    hash_hex = long_hex(hash256[::-1])
    version = vds.read_uint32()
    tx_pos = _read_CDiskTxPos(vds)
    if (hash_hex.startswith(tx_id) or short_hex(hash256[::-1]).startswith(tx_id)):
      _dump_tx(datadir, hash256, tx_pos)

    (key, value) = cursor.next()

  db.close()

