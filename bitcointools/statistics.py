#!/usr/bin/env python
#
# Read the block database, generate monthly statistics and dump out
# a CSV file.
#
from bsddb.db import *
from datetime import date
import logging
import os
import sys

from BCDataStream import *
from block import scan_blocks
from collections import defaultdict
from deserialize import parse_Block
from util import determine_db_dir, create_env

def main():
  import optparse
  parser = optparse.OptionParser(usage="%prog [options]")
  parser.add_option("--datadir", dest="datadir", default=None,
                    help="Look for files here (defaults to bitcoin default)")
  (options, args) = parser.parse_args()

  if options.datadir is None:
    db_dir = determine_db_dir()
  else:
    db_dir = options.datadir

  try:
    db_env = create_env(db_dir)
  except DBNoSuchFileError:
    logging.error("Couldn't open " + db_dir)
    sys.exit(1)

  blockfile = open(os.path.join(db_dir, "blk%04d.dat"%(1,)), "rb")
  block_datastream = BCDataStream()
  block_datastream.map_file(blockfile, 0)

  n_transactions = defaultdict(int)
  v_transactions = defaultdict(float)
  v_transactions_min = defaultdict(float)
  v_transactions_max = defaultdict(float)
  def gather_stats(block_data):
    block_datastream.seek_file(block_data['nBlockPos'])
    data = parse_Block(block_datastream)
    block_date = date.fromtimestamp(data['nTime'])
    key = "%d-%02d"%(block_date.year, block_date.month)
    for txn in data['transactions'][1:]:
      values = []
      for txout in txn['txOut']:
        n_transactions[key] += 1
        v_transactions[key] += txout['value'] 
        values.append(txout['value'])
      v_transactions_min[key] += min(values)
      v_transactions_max[key] += max(values)
    return True

  scan_blocks(db_dir, db_env, gather_stats)

  db_env.close()

  keys = n_transactions.keys()
  keys.sort()
  for k in keys:
    v = v_transactions[k]/1.0e8
    v_min = v_transactions_min[k]/1.0e8
    v_max = v_transactions_max[k]/1.0e8
    # Columns are:
    # month n_transactions min max total
    # ... where min and max add up just the smallest or largest
    # output in each transaction; the true value of bitcoins
    # transferred will be somewhere between min and max.
    # We don't know how many are transfers-to-self, though, and
    # this will undercount multi-txout-transactions (which is good
    # right now, because they're mostly used for mining pool
    # payouts that arguably shouldn't count).
    print "%s,%d,%.2f,%.2f,%.2f"%(k, n_transactions[k], v_min, v_max, v)

if __name__ == '__main__':
    main()
