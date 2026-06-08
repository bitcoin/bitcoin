#!/usr/bin/env python3
"""
WOTS+ w-parameter sweep benchmark.

Tests w in {4, 16, 256} (the three RFC 8391 standard values: 2-bit, 4-bit, 8-bit digits).
For each w, measures: keygen / sign / verify time, sig size, and block throughput.

Run standalone: python bench_wots_w_sweep.py
"""

import hashlib
import math
import statistics
import struct
import time

# ── WOTS+ primitives (parametric w) ──────────────────────────────────────────

def sha256(data: bytes) -> bytes:
    return hashlib.sha256(data).digest()

def uint64_be(n: int) -> bytes:
    return struct.pack(">Q", n)

# Fixed per-instantiation domain tags (same as wots_sha256.cpp)
DOMAIN_ANCHOR  = b"wots39-btc-v1"
DOMAIN_SK      = b"wots-sk-v1"
DOMAIN_BITMASK = b"wots-bm-v1--"
DOMAIN_KEY     = b"wots-key-v1"

def compute_anchor(txid: bytes, vout: int) -> bytes:
    return sha256(DOMAIN_ANCHOR + txid + struct.pack("<I", vout))

def derive_sk(msk: bytes, anchor: bytes, i: int) -> bytes:
    return sha256(msk + DOMAIN_SK + anchor + uint64_be(i))

def chain_step(x: bytes, j: int, anchor: bytes, chain_idx: int) -> bytes:
    idx = uint64_be(chain_idx)
    jb  = uint64_be(j)
    bitmask = sha256(DOMAIN_BITMASK + anchor + idx + jb)
    key     = sha256(DOMAIN_KEY     + anchor + idx + jb)
    xored   = bytes(a ^ b for a, b in zip(x, bitmask))
    return sha256(key + xored)

def chain_up(x: bytes, from_: int, to_: int, anchor: bytes, chain_idx: int) -> bytes:
    cur = x
    for j in range(from_, to_):
        cur = chain_step(cur, j, anchor, chain_idx)
    return cur

# ── WOTS+ parameter set ───────────────────────────────────────────────────────

class WOTSParams:
    """
    Standard WOTS+ parameter set for a given w.
    w must be a power of 2: 4, 16, or 256 (i.e., log2(w) in {2, 4, 8}).
    n = 32 bytes (256-bit SHA-256 output).
    """
    def __init__(self, w: int):
        assert w in (4, 16, 256), "w must be 4, 16, or 256"
        n = 32  # bytes
        self.w   = w
        self.n   = n
        self.log2w = int(math.log2(w))                  # bits per digit
        self.L1  = (8 * n) // self.log2w                # message chains
        self.L2  = math.floor(
            math.log2(self.L1 * (w - 1)) / self.log2w
        ) + 1                                            # checksum chains
        self.L   = self.L1 + self.L2                    # total chains
        self.sig_bytes = self.L * n

    def base_w_encode(self, msg: bytes) -> list:
        """Extract L1 w-ary digits from msg, then append L2 checksum digits."""
        # Extract message digits (log2(w) bits per digit)
        digits = []
        if self.log2w == 8:
            # w=256: each byte is one digit
            for b in msg:
                digits.append(b)
        elif self.log2w == 4:
            # w=16: each byte gives two 4-bit digits
            for b in msg:
                digits.append((b >> 4) & 0x0F)
                digits.append(b & 0x0F)
        elif self.log2w == 2:
            # w=4: each byte gives four 2-bit digits
            for b in msg:
                digits.append((b >> 6) & 0x03)
                digits.append((b >> 4) & 0x03)
                digits.append((b >> 2) & 0x03)
                digits.append(b & 0x03)

        assert len(digits) == self.L1, f"Expected {self.L1} digits, got {len(digits)}"

        # Checksum: sum of (w-1 - d) for each message digit
        csum = sum(self.w - 1 - d for d in digits)

        # Encode checksum in base-w (big-endian, L2 digits)
        csum_digits = []
        for _ in range(self.L2):
            csum_digits.append(csum % self.w)
            csum //= self.w
        csum_digits.reverse()

        return digits + csum_digits

    def keygen(self, msk: bytes, anchor: bytes) -> bytes:
        """Generate wotsPK commitment: SHA256(tip[0] || ... || tip[L-1])"""
        combined = b""
        for i in range(self.L):
            sk  = derive_sk(msk, anchor, i)
            tip = chain_up(sk, 0, self.w - 1, anchor, i)
            combined += tip
        return sha256(combined)

    def sign(self, msk: bytes, anchor: bytes, msg_hash: bytes) -> bytes:
        """Sign msg_hash, return L*32 byte signature."""
        digits = self.base_w_encode(msg_hash)
        sig = b""
        for i in range(self.L):
            sk   = derive_sk(msk, anchor, i)
            elem = chain_up(sk, 0, digits[i], anchor, i)
            sig += elem
        return sig

    def verify(self, wots_pk: bytes, anchor: bytes, msg_hash: bytes, sig: bytes) -> bool:
        """Verify signature, return True iff valid."""
        if len(sig) != self.sig_bytes:
            return False
        digits   = self.base_w_encode(msg_hash)
        combined = b""
        for i in range(self.L):
            elem = sig[i * 32:(i + 1) * 32]
            tip  = chain_up(elem, digits[i], self.w - 1, anchor, i)
            combined += tip
        return sha256(combined) == wots_pk

    def sha256_ops_keygen(self) -> int:
        """Exact SHA-256 op count for keygen."""
        # L chains x (1 SK derivation + (w-1) steps x 3 SHA256/step) + 1 final
        return self.L * (1 + (self.w - 1) * 3) + 1

    def sha256_ops_verify_avg(self) -> float:
        """Average SHA-256 op count for verify over a uniformly random message."""
        # Average digit for message elements: (w-1)/2
        # Verification steps per chain: (w-1) - avg_digit = (w-1)/2
        avg_steps = (self.w - 1) / 2
        return self.L * avg_steps * 3 + 1

    def sha256_ops_sign_avg(self) -> float:
        """Average SHA-256 op count for signing over a uniformly random message."""
        avg_digit = (self.w - 1) / 2
        return self.L * (1 + avg_digit * 3)

    def witness_bytes(self, script_bytes: int = 69, ctrl_bytes: int = 33) -> int:
        """
        Total witness serialization size for a WOTS spend.
        Items: anchor(32) + L sig elements(32 each) + schnorr_sig(64) + script + ctrl
        Each item: 1-byte length prefix + data (all items <= 252 bytes so 1-byte prefix).
        Plus 1 byte for item count.
        """
        return (
            1                              # item count
            + (1 + 32)                     # anchor
            + self.L * (1 + 32)            # sig elements
            + (1 + 64)                     # schnorr sig
            + (1 + script_bytes)           # script leaf
            + (1 + ctrl_bytes)             # control block
        )

    def transaction_weight(self) -> int:
        """
        Total weight of a minimal 1-input 1-output WOTS spending transaction.
        Non-witness = 94 bytes (version+input+output+locktime), each 4 wu.
        Segwit overhead = 2 bytes (marker+flag), 1 wu each.
        Witness = witness_bytes(), 1 wu each.
        """
        base = 94  # non-witness bytes
        return base * 4 + 2 + self.witness_bytes()

    def vbytes(self) -> int:
        return math.ceil(self.transaction_weight() / 4)

    def txs_per_block(self, max_weight: int = 4_000_000) -> int:
        return max_weight // self.transaction_weight()

# ── Benchmark runner ──────────────────────────────────────────────────────────

MSK      = bytes([0x07] * 32)
TXID     = bytes([0xAB] * 32)
MSG_HASH = bytes([0xCD] * 32)
ANCHOR   = compute_anchor(TXID, 0)

def bench_fn(fn, iters=50):
    for _ in range(5):
        fn()  # warmup
    t0 = time.perf_counter()
    for _ in range(iters):
        fn()
    return (time.perf_counter() - t0) / iters * 1000  # ms

# ── SHA-256 baseline ──────────────────────────────────────────────────────────

sha_times = []
for _ in range(5):
    n = 10_000
    t0 = time.perf_counter()
    for _ in range(n): sha256(bytes(64))
    sha_times.append((time.perf_counter() - t0) / n * 1_000_000)
sha256_us = statistics.median(sha_times)
sha256_per_sec = 1_000_000 / sha256_us

print("=" * 72)
print("WOTS+ w-parameter sweep benchmark  (n=32, SHA-256)")
print("=" * 72)
print(f"SHA-256 baseline: {sha256_us:.2f} us/hash  ({sha256_per_sec/1e6:.2f} M/sec, Python)")
print()

# ── Run for each w ────────────────────────────────────────────────────────────

W_VALUES = [4, 16, 256]
results = []

for w in W_VALUES:
    p = WOTSParams(w)

    # Pre-compute material
    pk  = p.keygen(MSK, ANCHOR)
    sig = p.sign(MSK, ANCHOR, MSG_HASH)
    assert p.verify(pk, ANCHOR, MSG_HASH, sig), f"Self-check failed for w={w}"

    t_key = bench_fn(lambda: p.keygen(MSK, ANCHOR),          iters=20)
    t_sgn = bench_fn(lambda: p.sign(MSK, ANCHOR, MSG_HASH),  iters=50)
    t_vfy = bench_fn(lambda: p.verify(pk, ANCHOR, MSG_HASH, sig), iters=100)

    results.append({
        "w"         : w,
        "L1"        : p.L1,
        "L2"        : p.L2,
        "L"         : p.L,
        "sig_bytes" : p.sig_bytes,
        "wit_bytes" : p.witness_bytes(),
        "tx_weight" : p.transaction_weight(),
        "vbytes"    : p.vbytes(),
        "txs_block" : p.txs_per_block(),
        "sha_key"   : p.sha256_ops_keygen(),
        "sha_sgn"   : p.sha256_ops_sign_avg(),
        "sha_vfy"   : p.sha256_ops_verify_avg(),
        "t_key_ms"  : t_key,
        "t_sgn_ms"  : t_sgn,
        "t_vfy_ms"  : t_vfy,
    })

# ── Results table ─────────────────────────────────────────────────────────────

print("WOTS+ Parameter Comparison")
print("-" * 72)
print(f"{'Metric':<36} {'w=4':>10} {'w=16':>10} {'w=256':>10}")
print("-" * 72)

rows = [
    ("Chains (L1 msg + L2 csum = L)",
     lambda r: f"{r['L1']}+{r['L2']}={r['L']}"),
    ("Signature size (bytes)",
     lambda r: f"{r['sig_bytes']:,}"),
    ("Witness total (bytes)",
     lambda r: f"{r['wit_bytes']:,}"),
    ("Tx weight (wu)",
     lambda r: f"{r['tx_weight']:,}"),
    ("Tx size (vbytes)",
     lambda r: f"{r['vbytes']}"),
    ("Max txs per block (4M wu)",
     lambda r: f"{r['txs_block']:,}"),
    ("SHA-256 ops: keygen (exact)",
     lambda r: f"{r['sha_key']:,}"),
    ("SHA-256 ops: sign (avg random)",
     lambda r: f"{r['sha_sgn']:.0f}"),
    ("SHA-256 ops: verify (avg random)",
     lambda r: f"{r['sha_vfy']:.0f}"),
    ("Keygen  [Python ms]",
     lambda r: f"{r['t_key_ms']:.2f}"),
    ("Sign    [Python ms]",
     lambda r: f"{r['t_sgn_ms']:.2f}"),
    ("Verify  [Python ms]",
     lambda r: f"{r['t_vfy_ms']:.2f}"),
    ("Verify  [C++ est. 10x, ms]",
     lambda r: f"{r['t_vfy_ms']/10:.3f}"),
    ("Verify  [C++ SHA-NI est. 40x, ms]",
     lambda r: f"{r['t_vfy_ms']/40:.3f}"),
]

for label, fn in rows:
    vals = [fn(r) for r in results]
    print(f"  {label:<34} {vals[0]:>10} {vals[1]:>10} {vals[2]:>10}")

print("-" * 72)

# ── Tradeoff analysis ─────────────────────────────────────────────────────────

print()
print("Tradeoff analysis (relative to current w=16):")
print("-" * 72)
base = results[1]  # w=16
for r in results:
    w = r['w']
    sig_ratio = r['sig_bytes'] / base['sig_bytes']
    vfy_ratio = r['t_vfy_ms'] / base['t_vfy_ms']
    blk_ratio = r['txs_block'] / base['txs_block']
    print(f"  w={w:<4}  sig size {sig_ratio:.2f}x  |  "
          f"verify time {vfy_ratio:.2f}x  |  "
          f"block capacity {blk_ratio:.2f}x")

print()

# ── Block validation cost ─────────────────────────────────────────────────────

print("Block validation cost (full node, C++ SHA-NI estimate):")
print("-" * 72)
cpp_speedup = 40  # conservative SHA-NI estimate
print(f"  (assuming {cpp_speedup}x Python speedup for C++ with SHA-NI)")
print()
for r in results:
    w         = r['w']
    vfy_ms    = r['t_vfy_ms'] / cpp_speedup
    txs       = r['txs_block']
    block_ms  = vfy_ms * txs
    print(f"  w={w:<4}  {txs:4d} txs/block x {vfy_ms:.3f} ms/verify = "
          f"{block_ms:6.1f} ms to verify all WOTS sigs in a full block")

print()

# ── Fee comparison ────────────────────────────────────────────────────────────

print("Fee comparison at 10 sat/vbyte, BTC = $100,000:")
print("-" * 72)
p2tr_vbytes = 111
p2tr_fee    = p2tr_vbytes * 10
print(f"  P2TR key-path:  {p2tr_vbytes} vbytes = {p2tr_fee:,} sat  (${p2tr_fee/1e8*100_000:.2f})")
for r in results:
    fee   = r['vbytes'] * 10
    delta = fee - p2tr_fee
    print(f"  w={r['w']:<4}        {r['vbytes']:3d} vbytes = {fee:,} sat  "
          f"(${fee/1e8*100_000:.2f})  +{delta:,} sat vs P2TR")

print()

# ── Recommendation ────────────────────────────────────────────────────────────

print("Recommendation:")
print("-" * 72)
# Find the sweet spot: minimize (verify_time_ratio * sig_size_ratio)
for r in results:
    sig_r = r['sig_bytes'] / base['sig_bytes']
    vfy_r = r['t_vfy_ms'] / base['t_vfy_ms']
    score = sig_r * vfy_r  # lower is better
    print(f"  w={r['w']:<4}  combined score (sig_ratio x verify_ratio) = {score:.3f}")

print()
print("  Lower combined score = better overall. Breakeven analysis:")
for r in results:
    if r['w'] == 16:
        continue
    sig_saving_pct = (1 - r['sig_bytes']/base['sig_bytes']) * 100
    vfy_cost_pct   = (r['t_vfy_ms']/base['t_vfy_ms'] - 1) * 100
    print(f"  w={r['w']:<4}  saves {sig_saving_pct:.0f}% on sig size  "
          f"costs {vfy_cost_pct:.0f}% more verify time")

print()
print("=" * 72)

# ── Canonical test vectors for each w ────────────────────────────────────────

print("Canonical test vectors:")
print("-" * 72)
msk_v  = bytes([0x01] * 32)
txid_v = bytes([0xDE] * 32)
anc_v  = compute_anchor(txid_v, 0)
msg_v  = bytes([0xAB] * 32)

print(f"  mskWOTS = {'01'*4}... (32 bytes)")
print(f"  txid    = {'DE'*4}... (32 bytes)")
print(f"  vout    = 0")
print(f"  anchor  = {anc_v.hex()}")
print()

for w in W_VALUES:
    p   = WOTSParams(w)
    pk  = p.keygen(msk_v, anc_v)
    sig = p.sign(msk_v, anc_v, msg_v)
    ok  = p.verify(pk, anc_v, msg_v, sig)
    print(f"  w={w}:")
    print(f"    L={p.L}  sig={p.sig_bytes}B")
    print(f"    wotsPK  = {pk.hex()}")
    print(f"    verify  = {ok}")
    print()

print("=" * 72)
