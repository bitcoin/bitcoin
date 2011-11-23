#
# Code for parsing the addr.dat file
# NOTE: I think you have to shutdown the Bitcoin client to
# successfully read addr.dat...
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

def dump_addresses(db_env):
  db = DB(db_env)
  try:
    r = db.open("addr.dat", "main", DB_BTREE, DB_THREAD|DB_RDONLY)
  except DBError:
    r = True

  if r is not None:
    logging.error("Couldn't open addr.dat/main. Try quitting Bitcoin and running this again.")
    sys.exit(1)

  kds = BCDataStream()
  vds = BCDataStream()

  for (key, value) in db.items():
    kds.clear(); kds.write(key)
    vds.clear(); vds.write(value)

    type = kds.read_string()

    if type == "addr":
      d = parse_CAddress(vds)
      print(deserialize_CAddress(d))

  db.close()
