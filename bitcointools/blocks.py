#
# Code for parsing the blkindex.dat file
#

from bsddb.db import *
import logging
from operator import itemgetter
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

def dump_blockindex(db_env, owner=None, n_to_dump=1000):
  db = DB(db_env)
  r = db.open("blkindex.dat", "main", DB_BTREE, DB_THREAD|DB_RDONLY)
  if r is not None:
    logging.error("Couldn't open blkindex.dat/main")
    sys.exit(1)

  kds = BCDataStream()
  vds = BCDataStream()

  wallet_transactions = []

  for (i, (key, value)) in enumerate(db.items()):
    if i > n_to_dump:
      break

    kds.clear(); kds.write(key)
    vds.clear(); vds.write(value)

    type = kds.read_string()

    if type == "tx":
      hash256 = kds.read_bytes(32)
      version = vds.read_uint32()
      tx_pos = _read_CDiskTxPos(vds)
      print("Tx(%s:%d %d %d)"%((short_hex(hash256),)+tx_pos))
      n_tx_out = vds.read_compact_size()
      for i in range(0,n_tx_out):
        tx_out = _read_CDiskTxPos(vds)
        if tx_out[0] != 0xffffffffL:  # UINT_MAX means no TxOuts (unspent)
          print("  ==> TxOut(%d %d %d)"%tx_out)
      
    else:
      logging.warn("blkindex: type %s"%(type,))
      continue

  db.close()

