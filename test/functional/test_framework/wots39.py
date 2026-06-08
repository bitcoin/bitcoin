"""
WOTS-39 Python implementation for Bitcoin functional tests.
Must produce byte-identical output to src/crypto/wots_sha256.cpp.

Parameters: n=32, w=256, L=34 (L1=32 message chains + L2=2 checksum chains)
Hash: SHA-256
Witness version: 3 (OP_3 = 0x53)

── P2WOTS Design ─────────────────────────────────────────────────────────────

  scriptPubKey = OP_3 (0x53) | PUSH32 (0x20) | address_commitment[32]   -- 34 bytes
  address      = bech32m, witness_version=3 -> "bc1r..." on mainnet
                                               "bcrt1r..." on regtest

  address_commitment = Merkle root over 64 independent WOTS slot keys.
  Each slot key is a fresh WOTS one-time key; spending uses exactly one slot.

── Merkle Key Tree ────────────────────────────────────────────────────────────

  address_nonce  = SHA256("wots39-addr-v1" || mskWOTS || uint64_be(address_index))
  slot_nonce[i]  = SHA256("wots39-slot-v1" || mskWOTS || address_nonce || uint64_be(i))
  wots_pk[i]     = SHA256(tip[0]||...||tip[33])  where tip[j] = ChainUp(sk[j], 0, 255, slot_nonce[i], j)
  leaf[i]        = SHA256("wots39-leaf-v1" || wots_pk[i])
  address_commitment = Merkle root over leaf[0..63]

── Witness Layout (42 items for single-sig) ──────────────────────────────────

  stack[0..33]  -- 34 WOTS+ sig elements (32 bytes each)
  stack[34]     -- slot_nonce (32 bytes)
  stack[35]     -- key_index (1 byte, 0-63)
  stack[36..41] -- auth_path (6 x 32 bytes, Merkle siblings leaf-to-root)

── Multi-sig Witness Layout (1 + n + k*43 items) ─────────────────────────────

  stack[0]             -- {k, n} header (2 bytes)
  stack[1..n]          -- n Merkle roots (n x 32 bytes)
  Per signer p in [0, k):
    stack[1+n + p*43 + 0]       -- signer_index (1 byte, which of the n roots)
    stack[1+n + p*43 + 1..34]   -- sig elements (34 x 32 bytes)
    stack[1+n + p*43 + 35]      -- slot_nonce (32 bytes)
    stack[1+n + p*43 + 36]      -- key_index (1 byte, 0-63)
    stack[1+n + p*43 + 37..42]  -- auth_path (6 x 32 bytes)

── P2WOTS Sighash ────────────────────────────────────────────────────────────

  sighash = TaggedHash("P2WOTS/sighash",
              EPOCH(1B) || HASH_TYPE(1B) || nVersion(4B LE) || nLockTime(4B LE) ||
              SHA256(all_outpoints) || SHA256(all_spent_amounts) ||
              SHA256(all_spent_scriptpubkeys) || SHA256(all_sequences) ||
              SHA256(all_outputs) || SPEND_TYPE(1B) || in_pos(4B LE))

  msgHash = SHA256("wots39-msg-v1" || sighash)
"""
import hashlib
import struct

# ── Domain separators (byte counts are LOAD-BEARING, no null terminator) ──────
DOMAIN_ADDR     = b"wots39-addr-v1"     # 14 bytes  per-address nonce
DOMAIN_SLOT     = b"wots39-slot-v1"     # 14 bytes  per-slot nonce
DOMAIN_LEAF     = b"wots39-leaf-v1"     # 14 bytes  Merkle leaf hash
DOMAIN_NODE     = b"wots39-node-v1"     # 14 bytes  Merkle internal node hash
DOMAIN_MULTISIG = b"wots39-multisig-v1" # 18 bytes  multi-sig commitment
DOMAIN_MSG      = b"wots39-msg-v1"      # 13 bytes  message hash
DOMAIN_SK       = b"wots-sk-v1"         # 10 bytes  secret key element
DOMAIN_BITMASK  = b"wots-bm-v1--"       # 12 bytes  chain bitmask
DOMAIN_KEY      = b"wots-key-v1"        # 11 bytes  chain key

assert len(DOMAIN_ADDR)     == 14
assert len(DOMAIN_SLOT)     == 14
assert len(DOMAIN_LEAF)     == 14
assert len(DOMAIN_NODE)     == 14
assert len(DOMAIN_MULTISIG) == 18
assert len(DOMAIN_MSG)      == 13
assert len(DOMAIN_SK)       == 10
assert len(DOMAIN_BITMASK)  == 12
assert len(DOMAIN_KEY)      == 11

# ── WOTS-39 parameters ────────────────────────────────────────────────────────
WOTS_N            = 32    # SHA-256 output bytes
WOTS_W            = 256   # Winternitz parameter
WOTS_L1           = 32    # message chains (one per byte of hash)
WOTS_L2           = 2     # checksum chains
WOTS_L            = 34    # total chains (L1 + L2)
WOTS_SIG_SIZE     = 1088  # 34 * 32 bytes
WOTS_TREE_HEIGHT  = 6     # log2(WOTS_TREE_SLOTS)
WOTS_TREE_SLOTS   = 64    # 2^WOTS_TREE_HEIGHT
WOTS_WITNESS_ITEMS = 42   # L + 1 (slot_nonce) + 1 (key_index) + WOTS_TREE_HEIGHT
WOTS_MULTISIG_ITEMS_PER_SIGNER = 43  # 1 + L + 1 + 1 + WOTS_TREE_HEIGHT
WOTS_MULTISIG_MAX_N = 20

# ── Low-level helpers ─────────────────────────────────────────────────────────

def sha256(data: bytes) -> bytes:
    return hashlib.sha256(data).digest()


def uint64_be(n: int) -> bytes:
    return struct.pack(">Q", n)


def xor_bytes(a: bytes, b: bytes) -> bytes:
    assert len(a) == len(b) == 32
    return bytes(x ^ y for x, y in zip(a, b))


# ── Address and slot nonce derivation ─────────────────────────────────────────

def compute_address_nonce(msk_wots: bytes, address_index: int) -> bytes:
    """
    address_nonce = SHA256("wots39-addr-v1" || mskWOTS || uint64_be(address_index))
    """
    assert len(msk_wots) == 32
    return sha256(DOMAIN_ADDR + msk_wots + uint64_be(address_index))


def compute_slot_nonce(msk_wots: bytes, address_nonce: bytes, slot_index: int) -> bytes:
    """
    slot_nonce = SHA256("wots39-slot-v1" || mskWOTS || address_nonce || uint64_be(slot_index))
    """
    assert len(msk_wots) == 32
    assert len(address_nonce) == 32
    return sha256(DOMAIN_SLOT + msk_wots + address_nonce + uint64_be(slot_index))


# ── WOTS+ chain primitives ────────────────────────────────────────────────────

def derive_sk(msk_wots: bytes, nonce: bytes, i: int) -> bytes:
    """
    sk[i] = SHA256(mskWOTS || "wots-sk-v1" || nonce || uint64_be(i))
    NOTE: mskWOTS comes FIRST, then domain separator.
    """
    assert len(msk_wots) == 32
    assert len(nonce) == 32
    return sha256(msk_wots + DOMAIN_SK + nonce + uint64_be(i))


def chain_step(x: bytes, j: int, nonce: bytes, chain_idx: int) -> bytes:
    """
    One WOTS+ chain step at position j:
      bitmask = SHA256("wots-bm-v1--" || nonce || chain_idx_be8 || j_be8)
      key     = SHA256("wots-key-v1"  || nonce || chain_idx_be8 || j_be8)
      result  = SHA256(key || (x XOR bitmask))
    """
    idx_bytes = uint64_be(chain_idx)
    j_bytes   = uint64_be(j)
    bitmask = sha256(DOMAIN_BITMASK + nonce + idx_bytes + j_bytes)
    key     = sha256(DOMAIN_KEY     + nonce + idx_bytes + j_bytes)
    xored   = xor_bytes(x, bitmask)
    return sha256(key + xored)


def chain_up(x: bytes, from_: int, to_: int, nonce: bytes, chain_idx: int) -> bytes:
    """
    Apply chain steps from from_ (inclusive) to to_ (exclusive).
    ChainUp(x, j, j, ...) == x  (identity: zero steps)
    """
    current = x
    for j in range(from_, to_):
        current = chain_step(current, j, nonce, chain_idx)
    return current


def base_w_encode(msg_hash: bytes) -> list:
    """
    Encode 32-byte message hash into 34 base-256 digits (w=256):
      digits[0..31]  = the 32 raw bytes of msg_hash  (range 0..255 each)
      digits[32..33] = 2-byte big-endian checksum
        checksum = sum(255 - digits[i] for i in 0..31)
        max checksum = 32 * 255 = 8160 = 0x1FE0  (fits in 2 bytes)
        digits[32] = (checksum >> 8) & 0xFF
        digits[33] = checksum & 0xFF
    """
    assert len(msg_hash) == 32
    digits = list(msg_hash)  # 32 bytes, each 0..255

    csum = sum(255 - d for d in digits)  # 0 .. 8160
    digits.append((csum >> 8) & 0xFF)
    digits.append(csum & 0xFF)

    assert len(digits) == WOTS_L
    return digits


# ── WOTS+ public key, sign, verify ───────────────────────────────────────────

def generate_wots_pk(msk_wots: bytes, nonce: bytes) -> bytes:
    """
    wots_pk = SHA256(tip[0] || ... || tip[33])
    tip[i]  = ChainUp(sk[i], 0, 255, nonce, i)  -- run full chain (255 steps)
    """
    parts = b""
    for i in range(WOTS_L):
        sk  = derive_sk(msk_wots, nonce, i)
        tip = chain_up(sk, 0, WOTS_W - 1, nonce, i)
        parts += tip
    return sha256(parts)


def sign_wots(msk_wots: bytes, nonce: bytes, msg_hash: bytes) -> bytes:
    """
    Produce flat WOTS+ signature (1088 bytes = 34 x 32).
    sig[i] = ChainUp(sk[i], 0, digits[i], nonce, i)
    """
    assert len(msg_hash) == 32
    digits = base_w_encode(msg_hash)
    sig = b""
    for i in range(WOTS_L):
        sk   = derive_sk(msk_wots, nonce, i)
        elem = chain_up(sk, 0, digits[i], nonce, i)
        sig += elem
    assert len(sig) == WOTS_SIG_SIZE
    return sig


def verify_wots(wots_pk: bytes, nonce: bytes, msg_hash: bytes, sig: bytes) -> bool:
    """
    Verify a flat WOTS+ signature (sig must be exactly 1088 bytes).
    Returns True iff SHA256(reconstructed_tips) == wots_pk.
    """
    if len(sig) != WOTS_SIG_SIZE:
        return False
    assert len(msg_hash) == 32
    digits  = base_w_encode(msg_hash)
    parts   = b""
    for i in range(WOTS_L):
        elem = sig[i * 32:(i + 1) * 32]
        tip  = chain_up(elem, digits[i], WOTS_W - 1, nonce, i)
        parts += tip
    return sha256(parts) == wots_pk


def verify_from_stack_elements(wots_pk: bytes, nonce: bytes, msg_hash: bytes,
                                sig_elements: list) -> bool:
    """
    Verify from WOTS_L separate 32-byte stack elements (witness stack form).
    sig_elements: list of 34 bytes objects each of length 32.
    """
    if len(sig_elements) != WOTS_L:
        return False
    digits = base_w_encode(msg_hash)
    parts  = b""
    for i in range(WOTS_L):
        if len(sig_elements[i]) != 32:
            return False
        tip = chain_up(bytes(sig_elements[i]), digits[i], WOTS_W - 1, nonce, i)
        parts += tip
    return sha256(parts) == wots_pk


# ── Merkle Key Tree ───────────────────────────────────────────────────────────

def wots_merkle_leaf(wots_pk: bytes) -> bytes:
    """leaf = SHA256("wots39-leaf-v1" || wots_pk)"""
    assert len(wots_pk) == 32
    return sha256(DOMAIN_LEAF + wots_pk)


def wots_merkle_node(left: bytes, right: bytes) -> bytes:
    """node = SHA256("wots39-node-v1" || left || right)"""
    assert len(left) == 32 and len(right) == 32
    return sha256(DOMAIN_NODE + left + right)


def compute_merkle_root(msk_wots: bytes, address_nonce: bytes) -> bytes:
    """
    Build the 64-leaf Merkle tree and return the root.
    leaf[i] = wots_merkle_leaf(generate_wots_pk(msk_wots, slot_nonce[i]))
    Fold level-by-level until single root.
    """
    layer = []
    for i in range(WOTS_TREE_SLOTS):
        slot_n = compute_slot_nonce(msk_wots, address_nonce, i)
        pk     = generate_wots_pk(msk_wots, slot_n)
        layer.append(wots_merkle_leaf(pk))

    width = WOTS_TREE_SLOTS
    while width > 1:
        for i in range(width // 2):
            layer[i] = wots_merkle_node(layer[i * 2], layer[i * 2 + 1])
        width //= 2

    return layer[0]


def generate_auth_path(msk_wots: bytes, address_nonce: bytes,
                       slot_index: int) -> list:
    """
    Generate authentication path (WOTS_TREE_HEIGHT=6 sibling nodes) that
    proves slot key at slot_index is a member of the Merkle tree.
    Returns list of 6 bytes objects (each 32 bytes), leaf level first.
    """
    assert 0 <= slot_index < WOTS_TREE_SLOTS

    layer = []
    for i in range(WOTS_TREE_SLOTS):
        slot_n = compute_slot_nonce(msk_wots, address_nonce, i)
        pk     = generate_wots_pk(msk_wots, slot_n)
        layer.append(wots_merkle_leaf(pk))

    auth_path = []
    idx   = slot_index
    width = WOTS_TREE_SLOTS

    for _level in range(WOTS_TREE_HEIGHT):
        sibling = idx ^ 1
        auth_path.append(layer[sibling])

        # Fold to next level
        for i in range(width // 2):
            layer[i] = wots_merkle_node(layer[i * 2], layer[i * 2 + 1])
        width //= 2
        idx >>= 1

    assert len(auth_path) == WOTS_TREE_HEIGHT
    return auth_path


def verify_auth_path(wots_pk: bytes, slot_index: int, auth_path: list,
                     merkle_root: bytes) -> bool:
    """
    Verify that wots_pk at position slot_index belongs to the Merkle tree
    with the given root.  Returns True iff the authentication path is valid.
    auth_path: list of WOTS_TREE_HEIGHT bytes objects (each 32 bytes).
    """
    if len(auth_path) != WOTS_TREE_HEIGHT:
        return False
    node = wots_merkle_leaf(wots_pk)
    idx  = slot_index

    for level in range(WOTS_TREE_HEIGHT):
        sibling = auth_path[level]
        if (idx & 1) == 0:
            node = wots_merkle_node(node, sibling)
        else:
            node = wots_merkle_node(sibling, node)
        idx >>= 1

    return node == merkle_root


# ── Multi-sig commitment ──────────────────────────────────────────────────────

def compute_multisig_commitment(k: int, n: int,
                                merkle_roots: list) -> bytes:
    """
    multisig_commitment = SHA256("wots39-multisig-v1" || k || n || root[0] || ... || root[n-1])
    k, n are single bytes; 1 <= k <= n <= WOTS_MULTISIG_MAX_N.
    merkle_roots: list of n bytes objects each 32 bytes.
    """
    assert 1 <= k <= n <= WOTS_MULTISIG_MAX_N
    assert len(merkle_roots) == n
    data = DOMAIN_MULTISIG + bytes([k, n])
    for root in merkle_roots:
        assert len(root) == 32
        data += root
    return sha256(data)


# ── Message hash ──────────────────────────────────────────────────────────────

def compute_msg_hash(sighash: bytes) -> bytes:
    """
    msgHash = SHA256("wots39-msg-v1" || sighash)
    The interpreter calls this before passing the message to WOTS verify.
    """
    assert len(sighash) == 32
    return sha256(DOMAIN_MSG + sighash)


# ── P2WOTS sighash ────────────────────────────────────────────────────────────

def _ser_compact_size(n: int) -> bytes:
    if n < 253:
        return bytes([n])
    elif n < 0x10000:
        return bytes([253]) + n.to_bytes(2, 'little')
    elif n < 0x100000000:
        return bytes([254]) + n.to_bytes(4, 'little')
    else:
        return bytes([255]) + n.to_bytes(8, 'little')


def compute_p2wots_sighash(tx, in_pos: int, spent_outputs: list) -> bytes:
    """
    Compute the P2WOTS sighash covering the full transaction.
    Matches SignatureHashP2WOTS() in interpreter.cpp exactly.

    tx:            CTransaction (from test_framework.messages)
    in_pos:        index of the input being signed (0-based)
    spent_outputs: list of CTxOut for ALL transaction inputs (same order as tx.vin)
                   i.e. the UTXOs being spent by each vin

    Returns 32-byte sighash (NOT yet msgHash; call compute_msg_hash to get msgHash).
    """
    assert in_pos < len(tx.vin)
    assert len(spent_outputs) == len(tx.vin)

    # --- Cache hashes (matching PrecomputedTransactionData) ---

    # m_prevouts_single_hash = SHA256(all outpoints)
    # outpoint = ser_uint256(txid_int) || n_le4  (COutPoint.serialize())
    prevouts_data = b""
    for inp in tx.vin:
        prevouts_data += inp.prevout.hash.to_bytes(32, 'little')
        prevouts_data += inp.prevout.n.to_bytes(4, 'little')
    prevouts_hash = sha256(prevouts_data)

    # m_spent_amounts_single_hash = SHA256(all spent amounts as int64 LE)
    amounts_data = b""
    for out in spent_outputs:
        amounts_data += out.nValue.to_bytes(8, 'little', signed=True)
    amounts_hash = sha256(amounts_data)

    # m_spent_scripts_single_hash = SHA256(all spent scriptPubKeys as ser_string)
    scripts_data = b""
    for out in spent_outputs:
        scripts_data += _ser_compact_size(len(out.scriptPubKey)) + out.scriptPubKey
    scripts_hash = sha256(scripts_data)

    # m_sequences_single_hash = SHA256(all sequences as uint32 LE)
    seqs_data = b""
    for inp in tx.vin:
        seqs_data += inp.nSequence.to_bytes(4, 'little')
    seqs_hash = sha256(seqs_data)

    # m_outputs_single_hash = SHA256(all tx outputs serialized)
    # CTxOut.serialize() = amount_le8 || ser_string(scriptPubKey)
    outputs_data = b""
    for out in tx.vout:
        outputs_data += out.nValue.to_bytes(8, 'little', signed=True)
        outputs_data += _ser_compact_size(len(out.scriptPubKey)) + out.scriptPubKey
    outputs_hash = sha256(outputs_data)

    # --- Build payload ---
    EPOCH      = bytes([0])
    HASH_TYPE  = bytes([1])  # SIGHASH_ALL
    SPEND_TYPE = bytes([0])  # no annex, no ext_flag

    payload = (
        EPOCH
        + HASH_TYPE
        + tx.version.to_bytes(4, 'little')
        + tx.nLockTime.to_bytes(4, 'little')
        + prevouts_hash
        + amounts_hash
        + scripts_hash
        + seqs_hash
        + outputs_hash
        + SPEND_TYPE
        + in_pos.to_bytes(4, 'little')
    )

    # TaggedHash("P2WOTS/sighash", payload) = SHA256(SHA256(tag) || SHA256(tag) || payload)
    tag_hash = sha256(b"P2WOTS/sighash")
    return sha256(tag_hash + tag_hash + payload)


# ── Script and witness builders ───────────────────────────────────────────────

def build_p2wots_script(address_commitment: bytes) -> bytes:
    """
    Build the P2WOTS scriptPubKey: OP_3 PUSH32 <address_commitment>
    Encoded as: 0x53 0x20 <32 bytes>  -- 34 bytes total.
    Witness version 3; bech32m address = "bc1r..." (mainnet) / "bcrt1r..." (regtest).
    """
    assert len(address_commitment) == 32
    return bytes([0x53, 0x20]) + address_commitment


def build_p2wots_witness(wots_sig: bytes, slot_nonce: bytes,
                         key_index: int, auth_path: list) -> list:
    """
    Build the P2WOTS witness stack (42 items).

    wots_sig:   flat 1088-byte WOTS+ signature (34 x 32 bytes)
    slot_nonce: 32-byte domain separator for the chosen slot key
    key_index:  which of the 64 slots is being spent (0-63)
    auth_path:  list of 6 bytes objects (each 32 bytes), leaf-level first

    Returns list of 42 bytes objects:
      items[0..33]  = WOTS+ sig elements (34 x 32B)
      items[34]     = slot_nonce (32B)
      items[35]     = key_index (1B)
      items[36..41] = auth_path (6 x 32B)
    """
    assert len(wots_sig) == WOTS_SIG_SIZE
    assert len(slot_nonce) == 32
    assert 0 <= key_index < WOTS_TREE_SLOTS
    assert len(auth_path) == WOTS_TREE_HEIGHT

    items = []
    for i in range(WOTS_L):
        items.append(wots_sig[i * 32:(i + 1) * 32])  # 34 items
    items.append(slot_nonce)                           # item 34
    items.append(bytes([key_index]))                   # item 35
    for ap in auth_path:
        assert len(ap) == 32
        items.append(ap)                               # items 36-41

    assert len(items) == WOTS_WITNESS_ITEMS
    return items


def build_p2wots_multisig_witness(k: int, n: int, merkle_roots: list,
                                  signers: list) -> list:
    """
    Build the P2WOTS multi-sig witness stack (1 + n + k*43 items).

    k, n: policy parameters (k-of-n)
    merkle_roots: list of n bytes objects (each 32 bytes)
    signers: list of k dicts, each with keys:
        'signer_index': int (0 .. n-1, which policy root)
        'sig':          bytes (1088-byte flat WOTS+ signature)
        'slot_nonce':   bytes (32B)
        'key_index':    int (0-63)
        'auth_path':    list of 6 bytes objects (each 32B)

    Returns list of 1 + n + k*43 bytes objects.
    """
    assert 1 <= k <= n <= WOTS_MULTISIG_MAX_N
    assert len(merkle_roots) == n
    assert len(signers) == k

    items = []
    items.append(bytes([k, n]))       # item 0: {k, n} header
    for root in merkle_roots:
        assert len(root) == 32
        items.append(root)            # items 1..n

    for signer in signers:
        si    = signer['signer_index']
        sig   = signer['sig']
        snonce = signer['slot_nonce']
        kidx  = signer['key_index']
        apath = signer['auth_path']

        assert 0 <= si < n
        assert len(sig) == WOTS_SIG_SIZE
        assert len(snonce) == 32
        assert 0 <= kidx < WOTS_TREE_SLOTS
        assert len(apath) == WOTS_TREE_HEIGHT

        items.append(bytes([si]))     # signer_index
        for i in range(WOTS_L):
            items.append(sig[i * 32:(i + 1) * 32])  # 34 sig elements
        items.append(snonce)          # slot_nonce
        items.append(bytes([kidx]))   # key_index
        for ap in apath:
            assert len(ap) == 32
            items.append(ap)          # 6 auth_path items

    expected = 1 + n + k * WOTS_MULTISIG_ITEMS_PER_SIGNER
    assert len(items) == expected
    return items


# ── Address encoding ──────────────────────────────────────────────────────────

def p2wots_address(address_commitment: bytes, hrp: str = "bcrt") -> str:
    """
    Encode a P2WOTS address using bech32m (witness version 3).
      hrp="bcrt"  -> "bcrt1r..."  (regtest)
      hrp="bc"    -> "bc1r..."    (mainnet)
      hrp="tb"    -> "tb1r..."    (testnet)
    """
    from test_framework.segwit_addr import encode as segwit_encode
    assert len(address_commitment) == 32
    return segwit_encode(hrp, 3, list(address_commitment))


# ── High-level signing helper ─────────────────────────────────────────────────

def sign_p2wots_input(msk_wots: bytes, address_nonce: bytes,
                      slot_index: int, msg_hash: bytes) -> dict:
    """
    High-level helper: sign a P2WOTS input using slot slot_index.

    Returns dict with:
        'sig':        flat 1088-byte WOTS+ signature
        'slot_nonce': 32-byte slot nonce (include in witness)
        'key_index':  int (== slot_index, 0-63)
        'auth_path':  list of 6 x 32-byte sibling nodes
        'wots_pk':    32-byte WOTS public key (for verification)
    """
    slot_nonce = compute_slot_nonce(msk_wots, address_nonce, slot_index)
    sig        = sign_wots(msk_wots, slot_nonce, msg_hash)
    auth_path  = generate_auth_path(msk_wots, address_nonce, slot_index)
    wots_pk    = generate_wots_pk(msk_wots, slot_nonce)
    return {
        'sig':        sig,
        'slot_nonce': slot_nonce,
        'key_index':  slot_index,
        'auth_path':  auth_path,
        'wots_pk':    wots_pk,
    }


def sign_and_build_witness(msk_wots: bytes, address_nonce: bytes,
                           slot_index: int, tx, in_pos: int,
                           spent_outputs: list) -> list:
    """
    Convenience: compute sighash, sign with slot_index, return 42-item witness.

    tx, in_pos, spent_outputs: same as compute_p2wots_sighash.
    """
    sighash  = compute_p2wots_sighash(tx, in_pos, spent_outputs)
    msg_hash = compute_msg_hash(sighash)
    result   = sign_p2wots_input(msk_wots, address_nonce, slot_index, msg_hash)
    return build_p2wots_witness(
        result['sig'],
        result['slot_nonce'],
        result['key_index'],
        result['auth_path'],
    )


# ── Legacy alias (for test compatibility) ─────────────────────────────────────

def compute_wots_nonce(msk_wots: bytes, index: int) -> bytes:
    """Legacy: same as compute_address_nonce."""
    return compute_address_nonce(msk_wots, index)
