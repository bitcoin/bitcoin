#
#
#

from BCDataStream import *
from enumeration import Enumeration
from base58 import public_key_to_bc_address, hash_160_to_bc_address
import socket
import time
from util import short_hex, long_hex

def parse_CAddress(vds):
  d = {}
  d['nVersion'] = vds.read_int32()
  d['nTime'] = vds.read_uint32()
  d['nServices'] = vds.read_uint64()
  d['pchReserved'] = vds.read_bytes(12)
  d['ip'] = socket.inet_ntoa(vds.read_bytes(4))
  d['port'] = vds.read_uint16()
  return d

def deserialize_CAddress(d):
  return d['ip']+":"+str(d['port'])+" (lastseen: %s)"%(time.ctime(d['nTime']),)

def parse_setting(setting, vds):
  if setting[0] == "f":  # flag (boolean) settings
    return str(vds.read_boolean())
  elif setting == "addrIncoming":
    return "" # bitcoin 0.4 purposely breaks addrIncoming setting in encrypted wallets.
  elif setting[0:4] == "addr": # CAddress
    d = parse_CAddress(vds)
    return deserialize_CAddress(d)
  elif setting == "nTransactionFee":
    return vds.read_int64()
  elif setting == "nLimitProcessors":
    return vds.read_int32()
  return 'unknown setting'

def parse_TxIn(vds):
  d = {}
  d['prevout_hash'] = vds.read_bytes(32)
  d['prevout_n'] = vds.read_uint32()
  d['scriptSig'] = vds.read_bytes(vds.read_compact_size())
  d['sequence'] = vds.read_uint32()
  return d
def deserialize_TxIn(d, transaction_index=None, owner_keys=None):
  if d['prevout_hash'] == "\x00"*32:
    result = "TxIn: COIN GENERATED"
    result += " coinbase:"+d['scriptSig'].encode('hex_codec')
  elif transaction_index is not None and d['prevout_hash'] in transaction_index:
    p = transaction_index[d['prevout_hash']]['txOut'][d['prevout_n']]
    result = "TxIn: value: %f"%(p['value']/1.0e8,)
    result += " prev("+long_hex(d['prevout_hash'][::-1])+":"+str(d['prevout_n'])+")"
  else:
    result = "TxIn: prev("+long_hex(d['prevout_hash'][::-1])+":"+str(d['prevout_n'])+")"
    pk = extract_public_key(d['scriptSig'])
    result += " pubkey: "+pk
    result += " sig: "+decode_script(d['scriptSig'])
  if d['sequence'] < 0xffffffff: result += " sequence: "+hex(d['sequence'])
  return result

def parse_TxOut(vds):
  d = {}
  d['value'] = vds.read_int64()
  d['scriptPubKey'] = vds.read_bytes(vds.read_compact_size())
  return d

def deserialize_TxOut(d, owner_keys=None):
  result =  "TxOut: value: %f"%(d['value']/1.0e8,)
  pk = extract_public_key(d['scriptPubKey'])
  result += " pubkey: "+pk
  result += " Script: "+decode_script(d['scriptPubKey'])
  if owner_keys is not None:
    if pk in owner_keys: result += " Own: True"
    else: result += " Own: False"
  return result

def parse_Transaction(vds):
  d = {}
  d['version'] = vds.read_int32()
  n_vin = vds.read_compact_size()
  d['txIn'] = []
  for i in xrange(n_vin):
    d['txIn'].append(parse_TxIn(vds))
  n_vout = vds.read_compact_size()
  d['txOut'] = []
  for i in xrange(n_vout):
    d['txOut'].append(parse_TxOut(vds))
  d['lockTime'] = vds.read_uint32()
  return d
def deserialize_Transaction(d, transaction_index=None, owner_keys=None):
  result = "%d tx in, %d out\n"%(len(d['txIn']), len(d['txOut']))
  for txIn in d['txIn']:
    result += deserialize_TxIn(txIn, transaction_index) + "\n"
  for txOut in d['txOut']:
    result += deserialize_TxOut(txOut, owner_keys) + "\n"
  return result

def parse_MerkleTx(vds):
  d = parse_Transaction(vds)
  d['hashBlock'] = vds.read_bytes(32)
  n_merkleBranch = vds.read_compact_size()
  d['merkleBranch'] = vds.read_bytes(32*n_merkleBranch)
  d['nIndex'] = vds.read_int32()
  return d

def deserialize_MerkleTx(d, transaction_index=None, owner_keys=None):
  tx = deserialize_Transaction(d, transaction_index, owner_keys)
  result = "block: "+(d['hashBlock'][::-1]).encode('hex_codec')
  result += " %d hashes in merkle branch\n"%(len(d['merkleBranch'])/32,)
  return result+tx

def parse_WalletTx(vds):
  d = parse_MerkleTx(vds)
  n_vtxPrev = vds.read_compact_size()
  d['vtxPrev'] = []
  for i in xrange(n_vtxPrev):
    d['vtxPrev'].append(parse_MerkleTx(vds))

  d['mapValue'] = {}
  n_mapValue = vds.read_compact_size()
  for i in xrange(n_mapValue):
    key = vds.read_string()
    value = vds.read_string()
    d['mapValue'][key] = value
  n_orderForm = vds.read_compact_size()
  d['orderForm'] = []
  for i in xrange(n_orderForm):
    first = vds.read_string()
    second = vds.read_string()
    d['orderForm'].append( (first, second) )
  d['fTimeReceivedIsTxTime'] = vds.read_uint32()
  d['timeReceived'] = vds.read_uint32()
  d['fromMe'] = vds.read_boolean()
  d['spent'] = vds.read_boolean()

  return d

def deserialize_WalletTx(d, transaction_index=None, owner_keys=None):
  result = deserialize_MerkleTx(d, transaction_index, owner_keys)
  result += "%d vtxPrev txns\n"%(len(d['vtxPrev']),)
  result += "mapValue:"+str(d['mapValue'])
  if len(d['orderForm']) > 0:
    result += "\n"+" orderForm:"+str(d['orderForm'])
  result += "\n"+"timeReceived:"+time.ctime(d['timeReceived'])
  result += " fromMe:"+str(d['fromMe'])+" spent:"+str(d['spent'])
  return result

# The CAuxPow (auxiliary proof of work) structure supports merged mining.
# A flag in the block version field indicates the structure's presence.
# As of 8/2011, the Original Bitcoin Client does not use it.  CAuxPow
# originated in Namecoin; see
# https://github.com/vinced/namecoin/blob/mergedmine/doc/README_merged-mining.md.
def parse_AuxPow(vds):
  d = parse_MerkleTx(vds)
  n_chainMerkleBranch = vds.read_compact_size()
  d['chainMerkleBranch'] = vds.read_bytes(32*n_chainMerkleBranch)
  d['chainIndex'] = vds.read_int32()
  d['parentBlock'] = parse_BlockHeader(vds)
  return d

def parse_BlockHeader(vds):
  d = {}
  header_start = vds.read_cursor
  d['version'] = vds.read_int32()
  d['hashPrev'] = vds.read_bytes(32)
  d['hashMerkleRoot'] = vds.read_bytes(32)
  d['nTime'] = vds.read_uint32()
  d['nBits'] = vds.read_uint32()
  d['nNonce'] = vds.read_uint32()
  header_end = vds.read_cursor
  d['__header__'] = vds.input[header_start:header_end]
  return d

def parse_Block(vds):
  d = parse_BlockHeader(vds)
  if d['version'] & (1 << 8):
    d['auxpow'] = parse_AuxPow(vds)
  d['transactions'] = []
  nTransactions = vds.read_compact_size()
  for i in xrange(nTransactions):
    d['transactions'].append(parse_Transaction(vds))

  return d
  
def deserialize_Block(d):
  result = "Time: "+time.ctime(d['nTime'])+" Nonce: "+str(d['nNonce'])
  result += "\nnBits: 0x"+hex(d['nBits'])
  result += "\nhashMerkleRoot: 0x"+d['hashMerkleRoot'][::-1].encode('hex_codec')
  result += "\nPrevious block: "+d['hashPrev'][::-1].encode('hex_codec')
  result += "\n%d transactions:\n"%len(d['transactions'])
  for t in d['transactions']:
    result += deserialize_Transaction(t)+"\n"
  result += "\nRaw block header: "+d['__header__'].encode('hex_codec')
  return result

def parse_BlockLocator(vds):
  d = { 'hashes' : [] }
  nHashes = vds.read_compact_size()
  for i in xrange(nHashes):
    d['hashes'].append(vds.read_bytes(32))
  return d

def deserialize_BlockLocator(d):
  result = "Block Locator top: "+d['hashes'][0][::-1].encode('hex_codec')
  return result

opcodes = Enumeration("Opcodes", [
    ("OP_0", 0), ("OP_PUSHDATA1",76), "OP_PUSHDATA2", "OP_PUSHDATA4", "OP_1NEGATE", "OP_RESERVED",
    "OP_1", "OP_2", "OP_3", "OP_4", "OP_5", "OP_6", "OP_7",
    "OP_8", "OP_9", "OP_10", "OP_11", "OP_12", "OP_13", "OP_14", "OP_15", "OP_16",
    "OP_NOP", "OP_VER", "OP_IF", "OP_NOTIF", "OP_VERIF", "OP_VERNOTIF", "OP_ELSE", "OP_ENDIF", "OP_VERIFY",
    "OP_RETURN", "OP_TOALTSTACK", "OP_FROMALTSTACK", "OP_2DROP", "OP_2DUP", "OP_3DUP", "OP_2OVER", "OP_2ROT", "OP_2SWAP",
    "OP_IFDUP", "OP_DEPTH", "OP_DROP", "OP_DUP", "OP_NIP", "OP_OVER", "OP_PICK", "OP_ROLL", "OP_ROT",
    "OP_SWAP", "OP_TUCK", "OP_CAT", "OP_SUBSTR", "OP_LEFT", "OP_RIGHT", "OP_SIZE", "OP_INVERT", "OP_AND",
    "OP_OR", "OP_XOR", "OP_EQUAL", "OP_EQUALVERIFY", "OP_RESERVED1", "OP_RESERVED2", "OP_1ADD", "OP_1SUB", "OP_2MUL",
    "OP_2DIV", "OP_NEGATE", "OP_ABS", "OP_NOT", "OP_0NOTEQUAL", "OP_ADD", "OP_SUB", "OP_MUL", "OP_DIV",
    "OP_MOD", "OP_LSHIFT", "OP_RSHIFT", "OP_BOOLAND", "OP_BOOLOR",
    "OP_NUMEQUAL", "OP_NUMEQUALVERIFY", "OP_NUMNOTEQUAL", "OP_LESSTHAN",
    "OP_GREATERTHAN", "OP_LESSTHANOREQUAL", "OP_GREATERTHANOREQUAL", "OP_MIN", "OP_MAX",
    "OP_WITHIN", "OP_RIPEMD160", "OP_SHA1", "OP_SHA256", "OP_HASH160",
    "OP_HASH256", "OP_CODESEPARATOR", "OP_CHECKSIG", "OP_CHECKSIGVERIFY", "OP_CHECKMULTISIG",
    "OP_CHECKMULTISIGVERIFY",
    ("OP_SINGLEBYTE_END", 0xF0),
    ("OP_DOUBLEBYTE_BEGIN", 0xF000),
    "OP_PUBKEY", "OP_PUBKEYHASH",
    ("OP_INVALIDOPCODE", 0xFFFF),
])

def script_GetOp(bytes):
  i = 0
  while i < len(bytes):
    vch = None
    opcode = ord(bytes[i])
    i += 1
    if opcode >= opcodes.OP_SINGLEBYTE_END:
      opcode <<= 8
      opcode |= bytes[i]
      i += 1

    if opcode <= opcodes.OP_PUSHDATA4:
      nSize = opcode
      if opcode == opcodes.OP_PUSHDATA1:
        nSize = ord(bytes[i])
        i += 1
      elif opcode == opcodes.OP_PUSHDATA2:
        nSize = unpack_from('<H', bytes, i)
        i += 2
      elif opcode == opcodes.OP_PUSHDATA4:
        nSize = unpack_from('<I', bytes, i)
        i += 4
      vch = bytes[i:i+nSize]
      i += nSize

    yield (opcode, vch)

def script_GetOpName(opcode):
  return (opcodes.whatis(opcode)).replace("OP_", "")

def decode_script(bytes):
  result = ''
  for (opcode, vch) in script_GetOp(bytes):
    if len(result) > 0: result += " "
    if opcode <= opcodes.OP_PUSHDATA4:
      result += "%d:"%(opcode,)
      result += short_hex(vch)
    else:
      result += script_GetOpName(opcode)
  return result

def match_decoded(decoded, to_match):
  if len(decoded) != len(to_match):
    return False;
  for i in range(len(decoded)):
    if to_match[i] == opcodes.OP_PUSHDATA4 and decoded[i][0] <= opcodes.OP_PUSHDATA4:
      continue  # Opcodes below OP_PUSHDATA4 all just push data onto stack, and are equivalent.
    if to_match[i] != decoded[i][0]:
      return False
  return True

def extract_public_key(bytes):
  decoded = [ x for x in script_GetOp(bytes) ]

  # non-generated TxIn transactions push a signature
  # (seventy-something bytes) and then their public key
  # (65 bytes) onto the stack:
  match = [ opcodes.OP_PUSHDATA4, opcodes.OP_PUSHDATA4 ]
  if match_decoded(decoded, match):
    return public_key_to_bc_address(decoded[1][1])

  # The Genesis Block, self-payments, and pay-by-IP-address payments look like:
  # 65 BYTES:... CHECKSIG
  match = [ opcodes.OP_PUSHDATA4, opcodes.OP_CHECKSIG ]
  if match_decoded(decoded, match):
    return public_key_to_bc_address(decoded[0][1])

  # Pay-by-Bitcoin-address TxOuts look like:
  # DUP HASH160 20 BYTES:... EQUALVERIFY CHECKSIG
  match = [ opcodes.OP_DUP, opcodes.OP_HASH160, opcodes.OP_PUSHDATA4, opcodes.OP_EQUALVERIFY, opcodes.OP_CHECKSIG ]
  if match_decoded(decoded, match):
    return hash_160_to_bc_address(decoded[2][1])

  return "(None)"
