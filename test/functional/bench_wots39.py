#!/usr/bin/env python3
"""
WOTS-39 Benchmark — timing and weight analysis for BIP documentation.
Run standalone: python bench_wots39.py
"""

import sys, os, time, hashlib, struct, statistics

# Allow running from the bitcoin repo root
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from test_framework.wots39 import (
    compute_anchor, generate_wots_pk, sign_wots, verify_wots,
    derive_sk, chain_step, chain_up, base_w_encode, WOTS_L,
)

# ── Deterministic test material ────────────────────────────────────────────────

MSK       = bytes([0x07] * 32)
TXID      = bytes([0xAB] * 32)
VOUT      = 0
MSG_HASH  = bytes([0xCD] * 32)

ANCHOR    = compute_anchor(TXID, VOUT)
WOTS_PK   = generate_wots_pk(MSK, ANCHOR)
SIG       = sign_wots(MSK, ANCHOR, MSG_HASH)

# Sanity
assert verify_wots(WOTS_PK, ANCHOR, MSG_HASH, SIG), "Self-check failed"

# ── Timing helper ──────────────────────────────────────────────────────────────

def bench(label, fn, iterations=200):
    # Warmup
    for _ in range(10):
        fn()
    t0 = time.perf_counter()
    for _ in range(iterations):
        fn()
    elapsed = time.perf_counter() - t0
    per_op_ms = elapsed / iterations * 1000
    ops_per_sec = iterations / elapsed
    print(f"  {label:<30s}  {per_op_ms:8.3f} ms/op   {ops_per_sec:8.1f} ops/sec")
    return per_op_ms

# ── SHA-256 throughput baseline ────────────────────────────────────────────────

def sha256(data): return hashlib.sha256(data).digest()

sha256_times = []
for _ in range(5):
    payload = bytes(64)
    n = 10_000
    t0 = time.perf_counter()
    for _ in range(n):
        sha256(payload)
    sha256_times.append((time.perf_counter() - t0) / n * 1_000_000)  # microsecondss

sha256_us = statistics.median(sha256_times)
sha256_per_sec = 1_000_000 / sha256_us

print("=" * 65)
print("WOTS-39 Benchmark")
print("=" * 65)
print(f"\n[SHA-256 baseline]")
print(f"  64-byte SHA-256:              {sha256_us:.3f} uss/hash   {sha256_per_sec/1e6:.2f} M hashes/sec")

# ── WOTS operation benchmarks ──────────────────────────────────────────────────

print(f"\n[WOTS-39 operations]")
t_keygen  = bench("KeyGen (generate_wots_pk)", lambda: generate_wots_pk(MSK, ANCHOR))
t_sign    = bench("Sign   (sign_wots)",        lambda: sign_wots(MSK, ANCHOR, MSG_HASH))
t_verify  = bench("Verify (verify_wots)",      lambda: verify_wots(WOTS_PK, ANCHOR, MSG_HASH, SIG))
t_anchor  = bench("ComputeAnchor",             lambda: compute_anchor(TXID, VOUT))

# ── SHA-256 operation count analysis ──────────────────────────────────────────

print(f"\n[SHA-256 operation count per WOTS call]")

digits = base_w_encode(MSG_HASH)

# Verification: for each chain i, ChainUp from digits[i] to 15
verify_steps = sum(15 - d for d in digits)  # total chain steps
verify_sha_per_step = 3  # bitmask + key + output
verify_sha = verify_steps * verify_sha_per_step + 1  # +1 for final PK hash
avg_steps_verify = verify_steps / WOTS_L

# Signing: for each chain i, DeriveSK (1) + ChainUp from 0 to digits[i]
sign_steps = sum(digits)
sign_sha = WOTS_L * 1 + sign_steps * verify_sha_per_step  # SK derivations + chain steps
avg_steps_sign = sign_steps / WOTS_L

# KeyGen: for each chain i, DeriveSK (1) + ChainUp from 0 to 15
keygen_sha = WOTS_L * (1 + 15 * verify_sha_per_step) + 1

print(f"  KeyGen:  {keygen_sha:5d} SHA-256 ops  ({WOTS_L} chains × (1 SK + 15 steps × 3 SHA256) + 1)")
print(f"  Sign:    {sign_sha:5d} SHA-256 ops  ({WOTS_L} SK derivations + {sign_steps} chain steps × 3, avg digit={sum(digits)/WOTS_L:.1f})")
print(f"  Verify:  {verify_sha:5d} SHA-256 ops  ({verify_steps} chain steps × 3 + 1, avg steps/chain={avg_steps_verify:.1f})")

# Expected time from SHA-256 baseline
print(f"\n[Expected vs measured (Python, single-threaded)]")
expected_verify_ms = (verify_sha * sha256_us) / 1000
expected_sign_ms   = (sign_sha   * sha256_us) / 1000
print(f"  Expected verify: {expected_verify_ms:.3f} ms  (from SHA-256 baseline × {verify_sha} ops)")
print(f"  Measured verify: {t_verify:.3f} ms  (ratio: {t_verify/expected_verify_ms:.2f}x)")

# C++ estimate: CSHA256 is typically 8-15x faster than Python hashlib in pure throughput
# With SHA-NI hardware acceleration it's even faster.
# Conservative estimate: 10x speedup for native C++ vs CPython hashlib
cpp_speedup = 10
print(f"\n[C++ estimated performance (conservative {cpp_speedup}x over Python)]")
print(f"  Verify:  {t_verify/cpp_speedup:.3f} ms   ({1000/t_verify*cpp_speedup:.0f} verifications/sec)")
print(f"  Sign:    {t_sign/cpp_speedup:.3f} ms   ({1000/t_sign*cpp_speedup:.0f} signs/sec)")
print(f"  KeyGen:  {t_keygen/cpp_speedup:.3f} ms   ({1000/t_keygen*cpp_speedup:.0f} keygens/sec)")

# ── Weight / vbyte analysis ────────────────────────────────────────────────────

print(f"\n[Precise weight and vbyte analysis — Enhanced design with Lamport Auth Chain]")
print()

# Enhanced tapscript (Lamport Auth Chain design):
#   <schnorrPK(32)>  OP_CHECKSIGVERIFY         = 33 + 1  = 34 B
#   <genesis_txid(32)>                          = 33 B
#   <root_authTip(32)>                          = 33 B
#   <chain_index>   (OP_0 for slot 0 = 1 byte) =  1 B
#   <wotsPK(32)>    OP_WOTS_VERIFY  OP_1        = 33 + 1 + 1 = 35 B
#   Total script bytes                          = 136 B  (chain_idx 0-16)
#                                               = 137 B  (chain_idx 17-75)
script_bytes = 34 + 33 + 33 + 1 + 35  # = 136  (using chain_index 0-16, OP_n encoding)

# Control block: version(1) + internal_pubkey(32) + no merkle siblings = 33B
ctrl_block_bytes = 33

# Witness items for enhanced WOTS spend:
#   anchor(32) + 67×sig_elem(32) + schnorr_sig(64) + script + ctrl_block
# Each item serialized as: 1-byte compact_size + data  (for data ≤ 252 bytes)
# script_bytes=136: compact_size = 1 byte (fits in 1-byte range)

def wit_item_size(data_len):
    """Serialized witness item: 1-byte length prefix + data (valid for data ≤ 252 bytes)."""
    return 1 + data_len

wots_wit_items = (
    1                                       # item count (compact size)
    + wit_item_size(32)                     # anchor (Lamport preimage)
    + WOTS_L * wit_item_size(32)            # 67 WOTS sig elements
    + wit_item_size(64)                     # Schnorr sig
    + wit_item_size(script_bytes)           # tapscript leaf
    + wit_item_size(ctrl_block_bytes)       # control block
)

p2tr_key_wit_items = (
    1                           # item count compact size (= 1 item)
    + wit_item_size(64)         # Schnorr sig only (key-path spend)
)

# Non-witness per-input: outpoint(36) + scriptSig_len(1) + scriptSig(0) + seq(4) = 41 B
non_wit_input = 41

# Minimal 1-in/1-out base transaction (non-witness, ×4 weight):
# version(4) + input_count(1) + input(41) + output_count(1) + P2TR_out(43) + locktime(4) = 94 B
base_tx_bytes = 4 + 1 + non_wit_input + 1 + 43 + 4  # = 94

# Weight = non_witness × 4  +  (segwit_marker_flag(2) + witness_bytes) × 1
wots_weight = base_tx_bytes * 4 + 2 + wots_wit_items
p2tr_weight  = base_tx_bytes * 4 + 2 + p2tr_key_wit_items

wots_vbytes = -(-wots_weight // 4)   # ceiling division → vbytes
p2tr_vbytes  = -(-p2tr_weight  // 4)

print(f"  Tapscript breakdown (enhanced design):")
print(f"    schnorrPK push + OP_CHECKSIGVERIFY:  34 bytes")
print(f"    genesis_txid push:                   33 bytes  (Lamport chain authorization)")
print(f"    root_authTip push:                   33 bytes  (Lamport chain tip)")
print(f"    chain_index (OP_n, slot 0-16):        1 byte   (spend position)")
print(f"    wotsPK push + OP_WOTS_VERIFY + OP_1: 35 bytes")
print(f"    -----------------------------------------------")
print(f"    total script:                       {script_bytes:4d} bytes")
print()
print(f"  Witness breakdown (enhanced WOTS spend):")
print(f"    anchor (1 item):            {wit_item_size(32):4d} bytes  (Lamport preimage)")
print(f"    sig elements (67 items):    {WOTS_L * wit_item_size(32):4d} bytes  ({WOTS_L} x 33)")
print(f"    schnorr sig (1 item):       {wit_item_size(64):4d} bytes")
print(f"    script leaf (1 item):       {wit_item_size(script_bytes):4d} bytes  (script = {script_bytes}B)")
print(f"    control block (1 item):     {wit_item_size(ctrl_block_bytes):4d} bytes  (ctrl = {ctrl_block_bytes}B)")
print(f"    item count prefix:          {1:4d} byte")
print(f"    -------------------------------------")
print(f"    total witness:              {wots_wit_items:4d} bytes")
print()
print(f"  Transaction weight (1-in 1-out):")
print(f"    Enhanced WOTS spend: {wots_weight:5d} wu  =  {wots_vbytes} vbytes")
print(f"    P2TR key path:       {p2tr_weight:5d} wu  =  {p2tr_vbytes} vbytes")
print(f"    Overhead ratio: {wots_weight/p2tr_weight:.2f}x  ({wots_vbytes/p2tr_vbytes:.2f}x vbytes)")
print()

# Block capacity
max_weight = 4_000_000
print(f"  Block capacity (4,000,000 wu limit):")
print(f"    Max enhanced WOTS txs/block: {max_weight // wots_weight:5d}")
print(f"    Max P2TR key-path txs/block: {max_weight // p2tr_weight:5d}")
print()

# C++ SHA-NI block validation cost
cpp_shani_speedup = 40  # conservative SHA-NI estimate from bench_wots_w_sweep
verify_ms_cpp_shani = t_verify / cpp_shani_speedup
block_validation_ms = (max_weight // wots_weight) * verify_ms_cpp_shani
print(f"  Full-block WOTS validation cost (C++ SHA-NI estimate):")
print(f"    {max_weight // wots_weight} txs x {verify_ms_cpp_shani:.3f} ms/verify = {block_validation_ms:.1f} ms")
print(f"    (Bitcoin block interval = 600,000 ms — {block_validation_ms/600000*100:.4f}% of block time)")
print()

# Fee analysis
print(f"  Fee analysis at 10 sat/vbyte, 1 BTC = $100,000:")
wots_fee_sat = wots_vbytes * 10
p2tr_fee_sat  = p2tr_vbytes  * 10
fee_delta_sat = wots_fee_sat - p2tr_fee_sat
fee_delta_usd = fee_delta_sat / 1e8 * 100_000
print(f"    P2TR key path fee:     {p2tr_fee_sat:6d} sat  (${p2tr_fee_sat/1e8*100_000:.4f})")
print(f"    Enhanced WOTS fee:     {wots_fee_sat:6d} sat  (${wots_fee_sat/1e8*100_000:.4f})")
print(f"    Delta vs P2TR:         {fee_delta_sat:6d} sat  (${fee_delta_usd:.4f})")
print(f"    Delta as % of 1 BTC:   {fee_delta_sat/1e8*100:.4f}%")

# ── Test vectors ───────────────────────────────────────────────────────────────

print(f"\n[Canonical test vectors]")
anchor_v   = compute_anchor(bytes([0xDE]*32), 0)
msk_v      = bytes([0x01]*32)
anchor_v01 = compute_anchor(bytes([0xDE]*32), 0)
sk0_v      = derive_sk(msk_v, anchor_v01, 0)
step1_v    = chain_step(sk0_v, 0, anchor_v01, 0)
pk_v       = generate_wots_pk(msk_v, anchor_v01)

print(f"  Input:  mskWOTS = 0x{'01'*8}... (32 bytes of 0x01)")
print(f"          txid    = 0x{'DE'*8}... (32 bytes of 0xDE)")
print(f"          vout    = 0")
print(f"  anchor  = {anchor_v01.hex()}")
print(f"  sk[0]   = {sk0_v.hex()}")
print(f"  step1   = {step1_v.hex()}")
print(f"  wotsPK  = {pk_v.hex()}")

print()
print("=" * 65)
