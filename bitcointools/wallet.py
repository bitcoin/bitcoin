#
# Code for parsing the wallet.dat file
#

from bsddb.db import *
import logging
import re
import sys
import time

from BCDataStream import *
from base58 import public_key_to_bc_address, bc_address_to_hash_160, hash_160
from util import short_hex, long_hex
from deserialize import *

def open_wallet(db_env, writable=False):
  db = DB(db_env)
  flags = DB_THREAD | (DB_CREATE if writable else DB_RDONLY)
  try:
    r = db.open("wallet.dat", "main", DB_BTREE, flags)
  except DBError:
    r = True

  if r is not None:
    logging.error("Couldn't open wallet.dat/main. Try quitting Bitcoin and running this again.")
    sys.exit(1)
  
  return db

def parse_wallet(db, item_callback):
  kds = BCDataStream()
  vds = BCDataStream()

  for (key, value) in db.items():
    d = { }

    kds.clear(); kds.write(key)
    vds.clear(); vds.write(value)

    type = kds.read_string()

    d["__key__"] = key
    d["__value__"] = value
    d["__type__"] = type

    try:
      if type == "tx":
        d["tx_id"] = kds.read_bytes(32)
        d.update(parse_WalletTx(vds))
      elif type == "name":
        d['hash'] = kds.read_string()
        d['name'] = vds.read_string()
      elif type == "version":
        d['version'] = vds.read_uint32()
      elif type == "setting":
        d['setting'] = kds.read_string()
        d['value'] = parse_setting(d['setting'], vds)
      elif type == "key":
        d['public_key'] = kds.read_bytes(kds.read_compact_size())
        d['private_key'] = vds.read_bytes(vds.read_compact_size())
      elif type == "wkey":
        d['public_key'] = kds.read_bytes(kds.read_compact_size())
        d['private_key'] = vds.read_bytes(vds.read_compact_size())
        d['created'] = vds.read_int64()
        d['expires'] = vds.read_int64()
        d['comment'] = vds.read_string()
      elif type == "ckey":
        d['public_key'] = kds.read_bytes(kds.read_compact_size())
        d['crypted_key'] = vds.read_bytes(vds.read_compact_size())
      elif type == "mkey":
        d['nID'] = kds.read_int32()
        d['crypted_key'] = vds.read_bytes(vds.read_compact_size())
        d['salt'] = vds.read_bytes(vds.read_compact_size())
        d['nDerivationMethod'] = vds.read_int32()
        d['nDeriveIterations'] = vds.read_int32()
        d['vchOtherDerivationParameters'] = vds.read_bytes(vds.read_compact_size())
      elif type == "defaultkey":
        d['key'] = vds.read_bytes(vds.read_compact_size())
      elif type == "pool":
        d['n'] = kds.read_int64()
        d['nVersion'] = vds.read_int32()
        d['nTime'] = vds.read_int64()
        d['public_key'] = vds.read_bytes(vds.read_compact_size())
      elif type == "acc":
        d['account'] = kds.read_string()
        d['nVersion'] = vds.read_int32()
        d['public_key'] = vds.read_bytes(vds.read_compact_size())
      elif type == "acentry":
        d['account'] = kds.read_string()
        d['n'] = kds.read_uint64()
        d['nVersion'] = vds.read_int32()
        d['nCreditDebit'] = vds.read_int64()
        d['nTime'] = vds.read_int64()
        d['otherAccount'] = vds.read_string()
        d['comment'] = vds.read_string()
      elif type == "bestblock":
        d['nVersion'] = vds.read_int32()
        d.update(parse_BlockLocator(vds))
      else:
        print "Unknown key type: "+type
      
      item_callback(type, d)

    except Exception, e:
      print("ERROR parsing wallet.dat, type %s"%type)
      print("key data in hex: %s"%key.encode('hex_codec'))
      print("value data in hex: %s"%value.encode('hex_codec'))
  
def update_wallet(db, type, data):
  """Write a single item to the wallet.
  db must be open with writable=True.
  type and data are the type code and data dictionary as parse_wallet would
  give to item_callback.
  data's __key__, __value__ and __type__ are ignored; only the primary data
  fields are used.
  """
  d = data
  kds = BCDataStream()
  vds = BCDataStream()

  # Write the type code to the key
  kds.write_string(type)
  vds.write("")             # Ensure there is something

  try:
    if type == "tx":
      raise NotImplementedError("Writing items of type 'tx'")
      kds.write(d['tx_id'])
      #d.update(parse_WalletTx(vds))
    elif type == "name":
      kds.write(d['hash'])
      vds.write(d['name'])
    elif type == "version":
      vds.write_uint32(d['version'])
    elif type == "setting":
      raise NotImplementedError("Writing items of type 'setting'")
      kds.write_string(d['setting'])
      #d['value'] = parse_setting(d['setting'], vds)
    elif type == "key":
      kds.write_string(d['public_key'])
      vds.write_string(d['private_key'])
    elif type == "wkey":
      kds.write_string(d['public_key'])
      vds.write_string(d['private_key'])
      vds.write_int64(d['created'])
      vds.write_int64(d['expires'])
      vds.write_string(d['comment'])
    elif type == "ckey":
      kds.write_string(d['public_key'])
      kds.write_string(d['crypted_key'])
    elif type == "mkey":
      kds.write_int32(d['nID'])
      vds.write_string(d['crypted_key'])
      vds.write_string(d['salt'])
      vds.write_int32(d['nDeriveIterations'])
      vds.write_int32(d['nDerivationMethod'])
      vds.write_string(d['vchOtherDerivationParameters'])
    elif type == "defaultkey":
      vds.write_string(d['key'])
    elif type == "pool":
      kds.write_int64(d['n'])
      vds.write_int32(d['nVersion'])
      vds.write_int64(d['nTime'])
      vds.write_string(d['public_key'])
    elif type == "acc":
      kds.write_string(d['account'])
      vds.write_int32(d['nVersion'])
      vds.write_string(d['public_key'])
    elif type == "acentry":
      kds.write_string(d['account'])
      kds.write_uint64(d['n'])
      vds.write_int32(d['nVersion'])
      vds.write_int64(d['nCreditDebit'])
      vds.write_int64(d['nTime'])
      vds.write_string(d['otherAccount'])
      vds.write_string(d['comment'])
    elif type == "bestblock":
      vds.write_int32(d['nVersion'])
      vds.write_compact_size(len(d['hashes']))
      for h in d['hashes']:
        vds.write(h)
    else:
      print "Unknown key type: "+type

    # Write the key/value pair to the database
    db.put(kds.input, vds.input)

  except Exception, e:
    print("ERROR writing to wallet.dat, type %s"%type)
    print("data dictionary: %r"%data)

def dump_wallet(db_env, print_wallet, print_wallet_transactions, transaction_filter):
  db = open_wallet(db_env)

  wallet_transactions = []
  transaction_index = { }
  owner_keys = { }

  def item_callback(type, d):
    if type == "tx":
      wallet_transactions.append( d )
      transaction_index[d['tx_id']] = d
    elif type == "key":
      owner_keys[public_key_to_bc_address(d['public_key'])] = d['private_key']
    elif type == "ckey":
      owner_keys[public_key_to_bc_address(d['public_key'])] = d['crypted_key']

    if not print_wallet:
      return
    if type == "tx":
      return
    elif type == "name":
      print("ADDRESS "+d['hash']+" : "+d['name'])
    elif type == "version":
      print("Version: %d"%(d['version'],))
    elif type == "setting":
      print(d['setting']+": "+str(d['value']))
    elif type == "key":
      print("PubKey "+ short_hex(d['public_key']) + " " + public_key_to_bc_address(d['public_key']) +
            ": PriKey "+ short_hex(d['private_key']))
    elif type == "wkey":
      print("WPubKey 0x"+ short_hex(d['public_key']) + " " + public_key_to_bc_address(d['public_key']) +
            ": WPriKey 0x"+ short_hex(d['crypted_key']))
      print(" Created: "+time.ctime(d['created'])+" Expires: "+time.ctime(d['expires'])+" Comment: "+d['comment'])
    elif type == "ckey":
      print("PubKey "+ short_hex(d['public_key']) + " " + public_key_to_bc_address(d['public_key']) +
            ": Encrypted PriKey "+ short_hex(d['crypted_key']))
    elif type == "mkey":
      print("Master Key %d"%(d['nID']) + ": 0x"+ short_hex(d['crypted_key']) +
            ", Salt: 0x"+ short_hex(d['salt']) +
            ". Passphrase hashed %d times with method %d with other parameters 0x"%(d['nDeriveIterations'], d['nDerivationMethod']) +
            long_hex(d['vchOtherDerivationParameters']))
    elif type == "defaultkey":
      print("Default Key: 0x"+ short_hex(d['key']) + " " + public_key_to_bc_address(d['key']))
    elif type == "pool":
      print("Change Pool key %d: %s (Time: %s)"% (d['n'], public_key_to_bc_address(d['public_key']), time.ctime(d['nTime'])))
    elif type == "acc":
      print("Account %s (current key: %s)"%(d['account'], public_key_to_bc_address(d['public_key'])))
    elif type == "acentry":
      print("Move '%s' %d (other: '%s', time: %s, entry %d) %s"%
            (d['account'], d['nCreditDebit'], d['otherAccount'], time.ctime(d['nTime']), d['n'], d['comment']))
    elif type == "bestblock":
      print deserialize_BlockLocator(d)
    else:
      print "Unknown key type: "+type

  parse_wallet(db, item_callback)

  if print_wallet_transactions:
    keyfunc = lambda i: i['timeReceived']
    for d in sorted(wallet_transactions, key=keyfunc):
      tx_value = deserialize_WalletTx(d, transaction_index, owner_keys)
      if len(transaction_filter) > 0 and re.search(transaction_filter, tx_value) is None: continue

      print("==WalletTransaction== "+long_hex(d['tx_id'][::-1]))
      print(tx_value)

  db.close()

def dump_accounts(db_env):
  db = open_wallet(db_env)

  kds = BCDataStream()
  vds = BCDataStream()

  accounts = set()

  for (key, value) in db.items():
    kds.clear(); kds.write(key)
    vds.clear(); vds.write(value)

    type = kds.read_string()

    if type == "acc":
      accounts.add(kds.read_string())
    elif type == "name":
      accounts.add(vds.read_string())
    elif type == "acentry":
      accounts.add(kds.read_string())
      # Note: don't need to add otheraccount, because moves are
      # always double-entry

  for name in sorted(accounts):
    print(name)

  db.close()

def rewrite_wallet(db_env, destFileName, pre_put_callback=None):
  db = open_wallet(db_env)

  db_out = DB(db_env)
  try:
    r = db_out.open(destFileName, "main", DB_BTREE, DB_CREATE)
  except DBError:
    r = True

  if r is not None:
    logging.error("Couldn't open %s."%destFileName)
    sys.exit(1)

  def item_callback(type, d):
    if (pre_put_callback is None or pre_put_callback(type, d)):
      db_out.put(d["__key__"], d["__value__"])

  parse_wallet(db, item_callback)

  db_out.close()
  db.close()

def trim_wallet(db_env, destFileName, pre_put_callback=None):
  """Write out ONLY address book public/private keys
     THIS WILL NOT WRITE OUT 'change' KEYS-- you should
     send all of your bitcoins to one of your public addresses
     before calling this.
  """
  db = open_wallet(db_env)
  
  pubkeys = []
  def gather_pubkeys(type, d):
    if type == "name":
      pubkeys.append(bc_address_to_hash_160(d['hash']))
  
  parse_wallet(db, gather_pubkeys)

  db_out = DB(db_env)
  try:
    r = db_out.open(destFileName, "main", DB_BTREE, DB_CREATE)
  except DBError:
    r = True

  if r is not None:
    logging.error("Couldn't open %s."%destFileName)
    sys.exit(1)

  def item_callback(type, d):
    should_write = False
    if type in [ 'version', 'name', 'acc' ]:
      should_write = True
    if type in [ 'key', 'wkey', 'ckey' ] and hash_160(d['public_key']) in pubkeys:
      should_write = True
    if pre_put_callback is not None:
      should_write = pre_put_callback(type, d, pubkeys)
    if should_write:
      db_out.put(d["__key__"], d["__value__"])

  parse_wallet(db, item_callback)

  db_out.close()
  db.close()
