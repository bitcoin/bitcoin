import hashlib
import struct
import time
import binascii

def sha256(data):
    return hashlib.sha256(data).digest()

def sha256d(data):
    return sha256(sha256(data))

def compact_to_target(bits):
    exponent = bits >> 24
    mantissa = bits & 0x00ffffff
    return mantissa * (2**(8 * (exponent - 3)))

def serialize_script_num(n):
    if n == 0:
        return b''
    s = bytearray()
    neg = n < 0
    abs_n = abs(n)
    while abs_n:
        s.append(abs_n & 0xff)
        abs_n >>= 8
    if s[-1] & 0x80:
        s.append(0x80 if neg else 0)
    elif neg:
        s[-1] |= 0x80
    return bytes(s)

def push_bytes(data):
    l = len(data)
    if l < 0x4c:
        return struct.pack('B', l) + data
    elif l <= 0xff:
        return b'\x4c' + struct.pack('B', l) + data
    elif l <= 0xffff:
        return b'\x4d' + struct.pack('<H', l) + data
    else:
        return b'\x4e' + struct.pack('<I', l) + data

# --- Configuration ---
pszTimestamp = "Clitboin 2026: The Coin for the People"
nTime = 1767744000
nBits = 0x207fffff
nVersion = 1
genesisReward = 50 * 100000000

# Output Script (Public Key + CheckSig)
# 04 + 64 bytes (uncompressed key)
pubkey_hex = "04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f"
output_script = bytes.fromhex(pubkey_hex) + b'\xac' # OP_CHECKSIG
# Push it
output_script_pushed = push_bytes(output_script) 
# Wait, CScript << hex << OP_CHECKSIG.
# The hex is pushed as data.
# Then OP_CHECKSIG is appended.
output_script_bytes = push_bytes(bytes.fromhex(pubkey_hex)) + b'\xac'


# --- Construct Coinbase Transaction ---
# Version (1)
tx_ver = struct.pack('<i', 1)
# Input Count (1)
tx_vin_count = b'\x01'
# Input: PrevHash (32 bytes 0)
tx_prev_hash = b'\x00' * 32
# Input: PrevIndex (-1 -> 0xffffffff)
tx_prev_index = b'\xff\xff\xff\xff'

# Input: ScriptSig
# CScript() << 486604799 << CScriptNum(4) << pszTimestamp
# 486604799 -> 0x1d00ffff
val_bits = 0x1d00ffff
script_bits = push_bytes(serialize_script_num(val_bits))
script_four = push_bytes(serialize_script_num(4))
script_msg = push_bytes(pszTimestamp.encode('ascii'))
script_sig = script_bits + script_four + script_msg

# ScriptSig Length
tx_script_len = struct.pack('B', len(script_sig)) # VarInt (assuming small enough)

# Sequence (0xffffffff)
tx_sequence = b'\xff\xff\xff\xff'

# Output Count (1)
tx_vout_count = b'\x01'
# Output: Value
tx_value = struct.pack('<q', genesisReward)
# Output: ScriptPubKey
tx_out_script = output_script_bytes
tx_out_script_len = struct.pack('B', len(tx_out_script)) # VarInt

# LockTime (0)
tx_locktime = b'\x00\x00\x00\x00'

# Serialize Tx
tx_serialized = (
    tx_ver +
    tx_vin_count +
    tx_prev_hash +
    tx_prev_index +
    tx_script_len +
    script_sig +
    tx_sequence +
    tx_vout_count +
    tx_value +
    tx_out_script_len +
    tx_out_script +
    tx_locktime
)

# Hash Genesis Tx (Double SHA256)
genesis_tx_hash = sha256d(tx_serialized)
# Reverse for display (Little Endian)
genesis_tx_hash_hex = binascii.hexlify(genesis_tx_hash[::-1]).decode('utf-8')
print(f"Genesis Tx Hash: {genesis_tx_hash_hex}")

# --- Construct Genesis Block Header ---
# Version (1)
block_ver = struct.pack('<i', nVersion)
# Prev Block (0)
block_prev = b'\x00' * 32
# Merkle Root (Hash of Genesis Tx)
block_merkle = genesis_tx_hash # Already internal byte order
# Time
block_time = struct.pack('<I', nTime)
# Bits
block_bits = struct.pack('<I', nBits)

# Nonce (We need to mine it!)
print("Mining Genesis Block...")
target = compact_to_target(nBits)

nNonce = 0
while True:
    block_nonce = struct.pack('<I', nNonce)
    header = (
        block_ver +
        block_prev +
        block_merkle +
        block_time +
        block_bits +
        block_nonce
    )
    
    block_hash = sha256d(header)
    block_hash_int = int.from_bytes(block_hash, 'little')
    
    if block_hash_int <= target:
        print(f"Success!")
        print(f"Nonce: {nNonce}")
        print(f"Hash: {binascii.hexlify(block_hash[::-1]).decode('utf-8')}")
        print(f"Merkle Root: {genesis_tx_hash_hex}")
        break
    
    nNonce += 1
    if nNonce % 1000000 == 0:
        print(f"Scanning nonce {nNonce}...")

