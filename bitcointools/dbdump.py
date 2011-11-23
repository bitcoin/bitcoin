#!/usr/bin/env python
#
# Code for dumping the bitcoin Berkeley db files in a human-readable format
#
from bsddb.db import *
import logging
import sys

from address import dump_addresses
from wallet import dump_wallet, dump_accounts
from blkindex import dump_blkindex_summary
from transaction import dump_transaction
from block import dump_block, dump_block_n, search_blocks, check_block_chain
from util import determine_db_dir, create_env

def main():
  import optparse
  parser = optparse.OptionParser(usage="%prog [options]")
  parser.add_option("--datadir", dest="datadir", default=None,
                    help="Look for files here (defaults to bitcoin default)")
  parser.add_option("--wallet", action="store_true", dest="dump_wallet", default=False,
                    help="Print out contents of the wallet.dat file")
  parser.add_option("--wallet-tx", action="store_true", dest="dump_wallet_tx", default=False,
                    help="Print transactions in the wallet.dat file")
  parser.add_option("--wallet-tx-filter", action="store", dest="wallet_tx_filter", default="",
                    help="Only print transactions that match given string/regular expression")
  parser.add_option("--accounts", action="store_true", dest="dump_accounts", default="",
                    help="Print out account names, one per line")
  parser.add_option("--blkindex", action="store_true", dest="dump_blkindex", default=False,
                    help="Print out summary of blkindex.dat file")
  parser.add_option("--check-block-chain", action="store_true", dest="check_chain", default=False,
                    help="Scan back and forward through the block chain, looking for inconsistencies")
  parser.add_option("--address", action="store_true", dest="dump_addr", default=False,
                    help="Print addresses in the addr.dat file")
  parser.add_option("--transaction", action="store", dest="dump_transaction", default=None,
                    help="Dump a single transaction, given hex transaction id (or abbreviated id)")
  parser.add_option("--block", action="store", dest="dump_block", default=None,
                    help="Dump a single block, given its hex hash (or abbreviated hex hash) OR block height")
  parser.add_option("--search-blocks", action="store", dest="search_blocks", default=None,
                    help="Search the block chain for blocks containing given regex pattern")
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

  dump_tx = options.dump_wallet_tx
  if len(options.wallet_tx_filter) > 0:
    dump_tx = True
  if options.dump_wallet or dump_tx:
    dump_wallet(db_env, options.dump_wallet, dump_tx, options.wallet_tx_filter)
  if options.dump_accounts:
    dump_accounts(db_env)

  if options.dump_addr:
    dump_addresses(db_env)

  if options.check_chain:
    check_block_chain(db_env)

  if options.dump_blkindex:
    dump_blkindex_summary(db_env)

  if options.dump_transaction is not None:
    dump_transaction(db_dir, db_env, options.dump_transaction)

  if options.dump_block is not None:
    if len(options.dump_block) < 7: # Probably an integer...
      try:
        dump_block_n(db_dir, db_env, int(options.dump_block))
      except ValueError:
        dump_block(db_dir, db_env, options.dump_block)
    else:
      dump_block(db_dir, db_env, options.dump_block)

  if options.search_blocks is not None:
    search_blocks(db_dir, db_env, options.search_blocks)

  db_env.close()

if __name__ == '__main__':
    main()
