#!/usr/bin/env python
#
# Recover from a semi-corrupt wallet
#
from bsddb.db import *
import logging
import sys

from wallet import rewrite_wallet, trim_wallet
from util import determine_db_dir, create_env

def main():
  import optparse
  parser = optparse.OptionParser(usage="%prog [options]")
  parser.add_option("--datadir", dest="datadir", default=None,
                    help="Look for files here (defaults to bitcoin default)")
  parser.add_option("--out", dest="outfile", default="walletNEW.dat",
                    help="Name of output file (default: walletNEW.dat)")
  parser.add_option("--clean", action="store_true", dest="clean", default=False,
                    help="Clean out old, spent change addresses and transactions")
  parser.add_option("--skipkey", dest="skipkey",
                    help="Skip entries with keys that contain given string")
  parser.add_option("--tweakspent", dest="tweakspent",
                    help="Tweak transaction to mark unspent")
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

  if options.clean:
    trim_wallet(db_env, options.outfile)

  elif options.skipkey:
    def pre_put_callback(type, data):
      if options.skipkey in data['__key__']:
        return False
      return True
    rewrite_wallet(db_env, options.outfile, pre_put_callback)
  elif options.tweakspent:
    txid = options.tweakspent.decode('hex_codec')[::-1]
    def tweak_spent_callback(type, data):
      if txid in data['__key__']:
        data['__value__'] = data['__value__'][:-1]+'\0'
      return True
    rewrite_wallet(db_env, options.outfile, tweak_spent_callback)
    pass
  else:
    rewrite_wallet(db_env, options.outfile)

  db_env.close()

if __name__ == '__main__':
    main()
