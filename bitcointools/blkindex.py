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

def dump_blkindex_summary(db_env):
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

  print("blkindex file summary:")
  for (key, value) in db.items():
    kds.clear(); kds.write(key)
    vds.clear(); vds.write(value)

    type = kds.read_string()

    if type == "tx":
      n_tx += 1
    elif type == "blockindex":
      n_blockindex += 1
    elif type == "version":
      version = vds.read_int32()
      print(" Version: %d"%(version,))
    elif type == "hashBestChain":
      hash = vds.read_bytes(32)
      print(" HashBestChain: %s"%(hash.encode('hex_codec'),))
    else:
      logging.warn("blkindex: unknown type '%s'"%(type,))
      continue

  print(" %d transactions, %d blocks."%(n_tx, n_blockindex))
  db.close()
