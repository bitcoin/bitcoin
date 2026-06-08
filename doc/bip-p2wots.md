BIP:
Title: P2WOTS: 64 Slot Winternitz Signature UTXO's (Witness Version Three)
Authors: Opus Lux <opusluxofficial@proton.me>
Status: Draft
Type: Specification
Created 2026-06-06
Licesnse: BSD-2-Clause

## **---Abstract---**
This document specifies a new native Bitcoin output type using witness version 3.
P2WOTS provides unconditional post quantum security using only SHA-256 hashes. 
Unlike all existing Bitcoin output types, P2WOTS contains no elliptic curve key material anywhere. 
The 34 byte scriptPubKey encodes a 32 byte commitment: (OP_3 (0x53) || PUSH32 (0x20) || commitment)
For single signer outputs, the commitment is a Merkle Key Tree root over 64 independent WOTS+ key pairs. 
For k-of-n multisig outputs, the commitment is 
SHA256("wots39-multisig-v1" || k || n || root[0] || ... || root[n-1]), 
this is a binding over all n participant's Merkle roots.
The Single-sig spending supplies a 42 item witness.
The k-of-n multi-sig supplies 1+n+k*43 items with each signer contributing sig elements, a slot nonce, 
a key index and a Merkle authentication path proving membership in thier individual root.
The scriptPubKey is exactly 34 bytes: OP_3 (0x53) || PUSH32 (0x20) || commitment[32] 
The 32-byte `commitment` encodes one of two policy types:

**Single-signer**: 
`commitment`  is the root of a depth-6 Merkle Key Tree over 64 independent WOTS+ 
              key pairs derived from the owner's master secret.

**k-of-n multisig**: 
`commitment`  is `SHA256("wots39-multisig-v1" || k || n || root[0] || ... || root[n-1])`,
              a single binding commitment over all n participants' individual Merkle roots.

### WOTS+ Parameters

----------------------------------
| Parameter       | Value        |
|-----------------|--------------|
| Winternitz w    | 256          |
| Chain elements  | L = 34       |
| Hash function   | SHA-256      |
| Slots per addr  | 64           |
| Merkle depth    | 6            |
| Leaves          | 64           |
----------------------------------

At w=256, each WOTS+ signature consists of 34 chain elements, each 32 bytes. 
The Winternitz checksum is encoded within those 34 elements via the standard 
WOTS+ base-w encoding with checksum appended.

### Witness Format

**Single-signer** (exactly 42 items, each 32 bytes)
-------------------------------------------------------------
| Items     | Content                                       |
|-----------|-----------------------------------------------|
| [0..33]   | `sig[i]` — 34 WOTS+ chain elements            |
| [34]      | `slot_nonce` — per-slot domain separator      |
| [35]      | `key_index` — which of 64 slots (uint8, 0–63) |
| [36..41]  | `auth_path[0..5]` — Merkle sibling nodes      |
-------------------------------------------------------------

**k-of-n multisig** (exactly 1 + n + k×43 items)
------------------------------------------------------------------------
| Items              | Content                                         |
|--------------------|-------------------------------------------------|
| [0]                | `[k, n]` — 2-byte policy header                 |
| [1..n]             | `root[0..n-1]` — each signer's Merkle root      |
| Per signer (×k):   |                                                 |
| [+0]               | `signer_index` — position in root array (0–n-1) |
| [+1..+34]          | `sig[0..33]` — 34 WOTS+ chain elements          |
| [+35]              | `slot_nonce`                                    |
| [+36]              | `key_index`                                     |
| [+37..+42]         | `auth_path[0..5]`                               |
------------------------------------------------------------------------

### Sighash
The P2WOTS sighash is a BIP-341-style tagged hash:

sighash = TaggedHash("P2WOTS/sighash",
    epoch(0x00) ||
    sighash_type(0x01) ||
    nVersion (int32 LE) ||
    nLockTime (uint32 LE)||
    sha_prevouts ||          // SHA256 of all input outpoints
    sha_amounts ||          // SHA256 of all spent output amounts
    sha_scriptpubkeys ||   // SHA256 of all spent output scriptPubKeys (compact-size prefixed)
    sha_sequences ||      // SHA256 of all input nSequence values
    sha_outputs ||       // SHA256 of all outputs (amount + compact-size scriptPubKey)
    spend_type(0x00) || // no annex
    input_index (uint32 LE)
)

This construction is identical to BIP-341 (Taproot) sighash commitment, 
reusing the same `PrecomputedTransactionData` caches, with a distinct domain tag.

### Verification Algorithm
A validating node performs the following steps when a P2WOTS input is encountered:

**Step 1 — Detect P2WOTS output**
Confirm `scriptPubKey.size() == 34`, `scriptPubKey[0] == 0x53`, `scriptPubKey[1] == 0x20`.

**Step 2 — Classify by witness item count**
- 42 items → single-sig path
- `1 + n + k×43` for valid k, n → multisig path
- Any other count → `SCRIPT_ERR_WOTS_VERIFY`

**Step 3a — Single-sig verification**
1. Validate all 42 items are exactly 32 bytes each.
2. Extract `sig[0..33]`, `slot_nonce`, `key_index` (must be in [0,63]), `auth_path[0..5]`.
3. Compute the P2WOTS sighash as specified above.
4. Base-256 encode sighash into 34 digits `d[0..33]` (including WOTS+ checksum).
5. For each `i` in `[0, 34)`: 
    reconstruct the public key chain element: 
        `pk[i] = ChainUp(sig[i], from=d[i], to=255, nonce=slot_nonce, chainIdx=i)`
6. Compute the leaf public key: `leaf_pk = SHA256(pk[0] || pk[1] || ... || pk[33])`
7. Compute the Merkle leaf hash: `leaf = SHA256("P2WOTS/leaf" || slot_nonce || key_index || leaf_pk)`
8. Walk the authentication path (6 levels). 
At each level `l`:
    if (key_index >> l) & 1 == 0:
        node = SHA256(node || auth_path[l]) // current node is left child
    else:
        node = SHA256(auth_path[l] || node) // current node is right child
9. Compare the computed `node` to `scriptPubKey[2..33]`.
    Equal → valid. 
    Not equal → `SCRIPT_ERR_WOTS_VERIFY`.

**Step 3b — Multisig verification**
1. Parse `k = witness[0][0]`, `n = witness[0][1]`. Validate `1 ≤ k ≤ n`.
2. Extract `root[0..n-1]` from `witness[1..n]` (each 32 bytes).
3. Compute policy commitment:
`SHA256("wots39-multisig-v1" || k || n || root[0] || ... || root[n-1])`
Compare to `scriptPubKey[2..33]`. Mismatch → `SCRIPT_ERR_WOTS_VERIFY`.
4. For each of the k signer blocks (at offset `1 + n + i×43`):
- Extract `signer_index`, `sig[0..33]`, `slot_nonce`, `key_index`, `auth_path[0..5]`.
- Run Steps 3a.3–3a.8 above.
- At Step 3a.9, compare the computed root to `root[signer_index]`.
5. All k signer verifications must pass.

### Transaction Sizes
---------------------------------------------------
| Output type        | Witness items     | vbytes |
|--------------------|-------------------|--------|
| P2TR (key path)    | 1                 | 111    |
| P2WOTS single-sig  | 42                | 434    |
| P2WOTS 2-of-2      | 1 + 2 + 2×43 = 89 | 730    |
| P2WOTS 2-of-3      | 1 + 3 + 2×43 = 90 | 762    |
---------------------------------------------------

Full-block verification overhead: 736ms per block, 0.122% of Bitcoin's 10-minuteblock interval.

### Security
Security reduces entirely to SHA-256 preimage resistance. 
Grover's algorithm provides at most a quadratic speedup, 
reducing effective security from 256 bits to 128 bits against a quantum adversary. 

The Merkle Key Tree directly solves Winternitz's one time signing problem, 
each incoming UTXO is assigned a distinct slot key, 
making address reuse safe of up to 64 times per address. 
Lets break down why this works so well and improves bitcoin.

1. Pre-Spend
The scriptPubKey is: 
OP_3 (0x53) || PUSH32 (0x20) || commitment[32], a 32 byte SHA-256 Merkle commitment. 

Before any spend an observer only sees: 
53 20 [32 bytes], the 32 bytes are computationally indistinguishable from random.

Nobody watching the UTXO set knows: 
How many slots the tree contains or what any of the 64 individual WOTS public keys are.
This means that in P2WOTS before you spend a UTXO sent to a fresh address nobody knows who controls it.

2. Post-Spend
When you spend slot K you reveal exactly:
The WOTS signature for slot k, The slot nonce for slot K, 
The key index K, 6 Merkle authentication path nodes proving leaf K is in the root.

What stays hiddens:
The other 63 slots' public keys, pre-images, and nonces.
This means that you can safely and effectively use each address 64 times.

3. What P2WOTS Does Not Fix
The address linkability is unchanged. 
If you send two UTXO's to the same address they will share the scriptPubKey.
They will be identified as belonging to the same address after being spent. 
This is identical to how regular Bitcoin address reuse works today. 
P2WOTS does not provide anonymity between UTXO's sent to the same address.

So the real upgrade is two fold and specific.

1. Pre spend quantum privacy
Unlike P2TR, no public key material is published until spend time. 
This means that an adversary doesn't even have starting material for an attack. 
Your funds are safe as long as SHA-256 preimage resistance holds, 
which no quantum algorithm meaningfully breaks at 256 bit security.

2. Post spend slot forward secrecy
Each spend makes exactly one of the 64 independently derived keys public. 
The remaining 63 slots are still hidden inside the same commitment.
P2WOTS does not claim to be a completely anonymous system. 
It's a post quantum secure output type with stronger pre spend confidentiality than P2TR 
and per slot forward secrecy.

## **---Motivation---**
Bitcoin's quantum vulnerability is well understood. Shor's algorithm computes the 
discrete logaritm of a secp256k1 public key in polynomial time on a cryptographically 
relevent quantum computer. Every existing Bitcoin output type exposes an elliptic curve key. 
The quantum narritive also creates fear about Bitcoin that prevents more value from 
flowing into the world's best safe haven asset. The demand for a viable light weight 
post quantum signing scheme on Bitcoin is high. A more private, quantum-secure, 
reasonably affordable output type is something that Bitcoin needs. 
Winternitz is the oldest hash based signature scheme but also at the center of XMSS and SPHINCS, 
this means that it's the lightest weight most well studied quantum-secure hash based signature. 
This is an ideal fit in situations where every satoshi counts and long studied security matters. 
By using Winternitz at w=256, the first signature of every address produces 434 vbyte signatures. 
This is an acceptable cost to pay for mathematically unbreakable security and true digital ownership. 
All other isues with using Winternitz are solved by the merkle 64 slot construction, 
meaning address reuse is safe you just pay slightly more in transaction overhead. 
The higher fees on subsequent sends are an acceptable tradeoff to gain address reuse safety. 
The native multi-sig and compatability with lightning network make this proposal even stronger. 


## **---Specification---**

### New Files

#### `src/crypto/wots_sha256.h`

The complete public API for the P2WOTS cryptographic library. Declares all constants and function signatures in `namespace WOTS39`. 
Included by `interpreter.cpp`, `solver.cpp`, and `validation.cpp` — the single source of truth for every protocol parameter.

---------------------------------------------------------------------------------------------
| Constant                         | Value | Meaning                                        |
|----------------------------------|-------|------------------------------------------------|
| `WOTS_W`                         | 256   | Winternitz parameter                           |
| `WOTS_L`                         | 34    | Total chain elements (32 message + 2 checksum) |
| `WOTS_SIG_SIZE`                  | 1088  | Signature bytes (34 × 32)                      |
| `WOTS_TREE_HEIGHT`               | 6     | Merkle tree depth                              |
| `WOTS_TREE_SLOTS`                | 64    | Slots per address (2^6)                        |
| `WOTS_WITNESS_ITEMS`             | 42    | Single-sig witness item count                  |
| `WOTS_MULTISIG_ITEMS_PER_SIGNER` | 43    | Per-signer items in k-of-n witness             |
| `WOTS_MULTISIG_MAX_N`            | 20    | Maximum signers in any policy                  |
---------------------------------------------------------------------------------------------

#### `src/crypto/wots_sha256.cpp`

The full implementation of every cryptographic primitive used by P2WOTS. All functions are pure no global state, no side effects.

**Address and slot nonce derivation**

- `ComputeAddressNonce(mskWOTS, address_index)` — `SHA256("wots39-addr-v1" || mskWOTS || uint64_be(address_index))`. Incrementing `address_index` generates a fresh independent address.
- `ComputeSlotNonce(mskWOTS, address_nonce, slot_index)` — `SHA256("wots39-slot-v1" || mskWOTS || address_nonce || uint64_be(slot_index))`. Appears in the spending witness as item 34.

**Merkle Key Tree construction**

- `WotsMerkleLeaf(wots_pk)` — `SHA256("wots39-leaf-v1" || wots_pk)`. Distinct domain prevents second-preimage attacks between leaf and internal nodes.
- `WotsMerkleNode(left, right)` — `SHA256("wots39-node-v1" || left || right)`.
- `ComputeMerkleRoot(mskWOTS, address_nonce)` — builds all 64 slot keys, hashes each into a leaf, folds the binary tree bottom-up, returns the 32-byte root that becomes the `scriptPubKey` commitment.
- `GenerateAuthPath(mskWOTS, address_nonce, slot_index)` — returns the 6 sibling nodes proving a slot is a member of the tree. These are witness items 36–41.
- `VerifyAuthPath(wots_pk, slot_index, auth_path, merkle_root)` — recomputes the root from a reconstructed leaf and the 6 auth path siblings. This is the final step of spending authorization.

**Multisig policy commitment**

- `ComputeMultiSigCommitment(k, n, merkle_roots)` — `SHA256("wots39-multisig-v1" || k || n || root[0] || ... || root[n-1])`. Binds all n participants' Merkle roots into the single 32-byte value placed in the k-of-n `scriptPubKey`.

**Core WOTS+ primitives**

- `DeriveSK(mskWOTS, nonce, i)` — `SHA256(mskWOTS || "wots-sk-v1" || nonce || uint64_be(i))`. Derives secret key element `i` for a given slot.
- `ChainStep(x, j, nonce, chainIdx)` — one WOTS+ chain step using the XOR-then-hash construction:
```
bitmask = SHA256("wots-bm-v1--" || nonce || chainIdx_be8 || j_be8)
key     = SHA256("wots-key-v1"  || nonce || chainIdx_be8 || j_be8)
result  = SHA256(key || (x XOR bitmask))
```
- `ChainUp(x, from, to, nonce, chainIdx)` — applies `ChainStep` iteratively from `from` to `to`. Signing applies `0 → digit[i]`, verification completes `digit[i] → 255`.
- `BaseWEncode(msgHash)` — encodes a 32-byte hash into 34 base-256 digits. `digits[0..31]` = raw bytes; checksum `= Σ(255 − digits[i])` packed into `digits[32..33]`. Maximum checksum value is 8160, fits in 2 bytes.
- `GenerateWOTSPK(mskWOTS, nonce)` — `SHA256(pk[0] || ... || pk[33])` where `pk[i] = ChainUp(sk[i], 0, 255, nonce, i)`.
- `Sign(mskWOTS, nonce, msgHash)` — produces 1088-byte signature: `sig[i] = ChainUp(sk[i], 0, digits[i], nonce, i)` for `i` in `[0, 34)`.
- `Verify` / `VerifyFromStackElements` — reconstructs chain tips and checks `SHA256(tip[0] || ... || tip[33]) == wotsPK`. The stack-elements variant accepts 34 separate 32-byte vectors matching the witness layout.

---

#### `src/test/wots_tests.cpp`

C++ unit tests compiled into `test_bitcoin` under the `wots_tests` suite. Covers
key generation determinism, sign/verify round-trip, wrong-message rejection,
wrong-nonce rejection, nonce isolation, Merkle tree construction, auth path
round-trips, multisig commitment construction, and parameter boundary conditions.

```
./build/src/test/test_bitcoin --run_test=wots_tests
```

---

### New Python Files

#### `test/functional/test_framework/wots39.py`

Python mirror of `wots_sha256.cpp`. Every function must produce **byte-identical
output** to its C++ counterpart for the same inputs. Any domain separator discrepancy,
including a single wrong byte count, causes `block-script-verify-flag-failed` when a
constructed transaction is submitted to a live regtest node.

#### `test/functional/feature_wots39.py`

End-to-end functional tests against a live regtest node. Covers: fund and spend a
P2WOTS output, invalid signature rejection, wrong nonce rejection, three independent
UTXOs spending from three independent slot keys, and cross-type spending
(P2WOTS input → P2PKH output).

#### `test/functional/bench_wots39.py`

Benchmarks measuring signing throughput, per-block verification overhead, and address
derivation cost. Reported result: full-block P2WOTS verification adds **736 ms per
block** — **0.122%** of Bitcoin's 10-minute block interval.

---

### Modified Files

#### `src/script/solver.h` and `src/script/solver.cpp`

**Why modified:** The script solver classifies every `scriptPubKey` into a known
`TxoutType`. P2WOTS requires a new type and two new script builder functions.

- Added `WITNESS_V2_WOTS` to the `TxoutType` enum, positioned before `WITNESS_UNKNOWN`. Required — `WITNESS_UNKNOWN` would otherwise silently swallow all P2WOTS outputs.
- Added P2WOTS detection in `Solver()`: `witnessversion == 3 && witnessprogram.size() == 32`
- Added `"witness_v2_wots"` to `GetTxnOutputType()`. This function has an `assert(false)` at the bottom — any missing case crashes the node.
- Added `GetScriptForP2WOTS(const uint256& wots_pk)` — builds the 34-byte single-sig `scriptPubKey`.
- Added `GetScriptForP2WOTSMultiSig(uint8_t k, uint8_t n, const std::vector<uint256>& merkle_roots)` — computes the multisig commitment and builds the same 34-byte format.


#### `src/script/interpreter.h` and `src/script/interpreter.cpp`

**Why modified:** The script interpreter is where all spending authorization runs.
This is the largest and most critical change in the implementation.

**`interpreter.h` changes:**

- Added `SCRIPT_VERIFY_P2WOTS` to `script_verify_flag_name` — gates all P2WOTS verification. Active unconditionally in this prototype; in production deployed via BIP9/BIP8 soft fork activation.
- Added `SigVersion::WITNESS_V2 = 4` identifying the P2WOTS signing context.
- Added `virtual bool ComputeP2WOTSSighash(uint256& hash_out) const` to `BaseSignatureChecker` (default returns `false`).
- Added `bool ComputeP2WOTSSighash(uint256& hash_out) const override` to `GenericTransactionSignatureChecker`.

**`interpreter.cpp` changes:**

*`PrecomputedTransactionData::Init` — critical cache fix:*
The BIP-341 cache fields were only populated for witness version 1 (`OP_1`) inputs.
P2WOTS uses witness version 3 (`OP_3`) and requires the same fields for its sighash.

```cpp
// Before:
if (scriptPubKey[0] == OP_1)
// After:
if (scriptPubKey[0] == OP_1 || scriptPubKey[0] == OP_3)
```

Without this fix, `SignatureHashP2WOTS` returns `false` because `cache.m_bip341_taproot_ready` is never set. 
The transaction is accepted to the mempool but rejected at block connection with `"Witness program hash mismatch"`.

*`SignatureHashP2WOTS` — new static template function:*
Computes `TaggedHash("P2WOTS/sighash", ...)` reusing the BIP-341
`PrecomputedTransactionData` cache. Fields committed in order:

```
epoch(0x00) | SIGHASH_ALL(0x01) | nVersion | nLockTime |
sha_prevouts | sha_amounts | sha_scriptpubkeys | sha_sequences | sha_outputs |
spend_type(0x00) | input_index
```

All `sha_*` fields cover the entire transaction, not just the P2WOTS input.

*`VerifyWitnessProgram` — P2WOTS verification branch:*
A new `else if (witversion == 3 && program.size() == 32 && !is_p2sh)` block handles
all P2WOTS spending. After checking `SCRIPT_VERIFY_P2WOTS` is active, it dispatches
on witness item count:

**Single-sig path (42 items):** 
Extracts `slot_nonce` (item 34), `key_index` (item 35, validated in [0, 63]), and `auth_path[0..5]` (items 36–41). 
Computes sighash via `checker.ComputeP2WOTSSighash()`, applies `ComputeMsgHash()`, then `BaseWEncode()`.
Reconstructs each of the 34 chain tips with `ChainUp(sig[i], digits[i], 255, slotNonce, i)`,
hashes all tips into `reconstructedPK`, and calls `VerifyAuthPath()` to check the
Merkle proof against `program`.

**Multisig path (1 + n + k×43 items):** 
Parses the `{k, n}` header from `stack[0]`, validates policy parameters, extracts the n Merkle roots, 
and recomputes `ComputeMultiSigCommitment(k, n, roots)` to verify it matches `program`. 
The sighash is computed once and reused for all k signers. 
A `std::vector<bool> signerUsed(n, false)` tracker ensures no `signer_index` appears twice. 
Each signer runs the same chain reconstruction and `VerifyAuthPath()` against their assigned `merkleRoots[signerIdx]`.

---

#### `src/script/script_error.h` and `src/script/script_error.cpp`

**Why modified:** Every distinct interpreter failure requires a named error code for
diagnostics and test assertions.

- Added `SCRIPT_ERR_WOTS_VERIFY` to the `ScriptError` enum (before `SCRIPT_ERR_ERROR_COUNT`)
- Added error string: `"OP_WOTS_VERIFY: WOTS+ post-quantum signature verification failed"`

---

#### `src/script/sign.cpp`

**Why modified:** 
`SignStep()` switches on `TxoutType` with an `assert(false)` at the bottom. 
Any unhandled type crashes the node during signing operations.

```cpp
case TxoutType::WITNESS_V2_WOTS:
    return false;
```

P2WOTS outputs are never signed by the standard `SigningProvider`. 
The wallet constructs the 42-item witness externally using `mskWOTS`. 
The standard provider has no knowledge of WOTS keys.

---

#### `src/addresstype.cpp`

**Why modified:** 
`ExtractDestination()` switches on `TxoutType` and crashes via `assert(false)` for any unhandled type. 
Called by RPCs including `decoderawtransaction` and `fundrawtransaction`.
```cpp
case TxoutType::WITNESS_V2_WOTS:
    addressRet = WitnessUnknown{3, vSolutions[0]};
    return true;
```

Encoding as `WitnessUnknown{3, <32 bytes>}` lets the existing `CScriptVisitor` emit `OP_3 || <32-byte push>` correctly, 
so P2WOTS addresses round-trip through bech32m without requiring a separate visitor case.

---

#### `src/validation.cpp`

**Why modified:** 
Block validation requires a P2WOTS output detector and the `SCRIPT_VERIFY_P2WOTS` flag must be activated.

- Added `#include <crypto/wots_sha256.h>` and `#include <unordered_set>`
- Added `IsP2WOTSOutput(const CTxOut& txout)`:

```cpp
bool IsP2WOTSOutput(const CTxOut& txout) {
    const CScript& spk = txout.scriptPubKey;
    return spk.size() == 34 &&
           static_cast<uint8_t>(spk[0]) == 0x53 &&
           static_cast<uint8_t>(spk[1]) == 0x20;
}
```

Defined as a plain function before its first use to avoid MSVC C2668. 
`validation.cpp` has an anonymous namespace that opens around line 444 and never closes 
a `static` forward declaration inside it combined with a later `static` definition produces two ambiguous symbols in MSVC.

- Modified `GetBlockScriptFlags()`:

```cpp
flags |= SCRIPT_VERIFY_P2WOTS;
```

In production this would be gated behind a BIP9/BIP8 deployment entry with a defined
start time, timeout, and minimum activation height.

---

#### `src/txdb.h` and `src/txdb.cpp`

**Why modified:** 
Minor include hygiene to resolve transitive dependency issues
introduced when `wots_sha256.h` was included by validation-layer files.

- Added `#include <utility>` to `txdb.h`
- Added `#include <compat/byteswap.h>` to `txdb.cpp`

---

#### `src/crypto/CMakeLists.txt` and `src/test/CMakeLists.txt`

**Why modified:** 
CMake must be explicitly told about every new `.cpp` file. Without
registration, new source files are silently ignored by the build.

- `wots_sha256.cpp` registered in the crypto library target
- `wots_tests.cpp` registered in the unit test target

---

#### `src/uint256.h` and `src/util/strencodings.h`

**Why modified:** Minor utility additions — constructor or helper signatures used
internally by `wots_sha256.cpp` that were absent in the base Bitcoin Core version.

---

### Domain Separators

Every hash in P2WOTS uses an explicit domain separator to ensure no two distinct
operations can produce colliding outputs from the same input. Byte counts are
**load-bearing** — hardcoded as raw lengths in both C++ and Python. A single wrong
byte breaks cross-language compatibility and produces invalid signatures.

----------------------------------------------------------------
| Separator              |Byte| Used in                        |
|------------------------|----|--------------------------------|
| `"wots-bm-v1--"`       | 12 | `ChainStep` bitmask derivation |
| `"wots-key-v1"`        | 11 | `ChainStep` key derivation     |
| `"wots-sk-v1"`         | 10 | `DeriveSK`                     |
| `"wots39-addr-v1"`     | 14 | `ComputeAddressNonce`          |
| `"wots39-slot-v1"`     | 14 | `ComputeSlotNonce`             |
| `"wots39-msg-v1"`      | 13 | `ComputeMsgHash`               |
| `"wots39-leaf-v1"`     | 14 | `WotsMerkleLeaf`               |
| `"wots39-node-v1"`     | 14 | `WotsMerkleNode`               |
| `"wots39-multisig-v1"` | 18 | `ComputeMultiSigCommitment`    |
----------------------------------------------------------------

`"P2WOTS/sighash"` is used as a tagged hash label in `SignatureHashP2WOTS` via Bitcoin Core's `TaggedHash()` API, 
which double-SHA256s the tag to produce the initialization state, following the BIP-340 tagged hash convention.


---

## **--Backwards Compatibility--**

P2WOTS is deployed as a **soft fork** using a previously unassigned witness version
(version 3). No existing output type is modified. The upgrade is fully backwards
compatible by the same mechanism that governed SegWit (BIP-141) and Taproot (BIP-342).

**Pre-SegWit nodes** (nodes that do not enforce BIP-141) see P2WOTS outputs as
anyone-can-spend. This is identical behavior to P2WPKH and P2TR on the same class of
old nodes. Upgraded nodes enforce `SCRIPT_VERIFY_P2WOTS` and prevent unauthorized
spends, making this threat non-exploitable so long as a majority of hash rate enforces
the upgrade.

**Post-SegWit, pre-P2WOTS nodes** classify the `OP_3 || PUSH32` `scriptPubKey` as
`WITNESS_UNKNOWN` under BIP-141's forward-compatibility rules. They relay and mine
transactions spending P2WOTS outputs without applying WOTS verification — the same
behavior non-taproot nodes exhibited toward P2TR outputs during the taproot
deployment window. This is safe: the minority of non-upgraded nodes cannot produce
valid blocks that steal P2WOTS coins because upgraded nodes reject such blocks.

**All existing output types are completely unaffected:**

------------------------
| Output type | Impact |
|-------------|--------|
| P2PKH       | None   |
| P2SH        | None   |
| P2WPKH      | None   |
| P2WSH       | None   |
| P2TR        | None   |
------------------------

**Sighash cache change** 
— `PrecomputedTransactionData::Init` 
is extended to populate the BIP-341 precomputed fields for `OP_3` inputs in addition to `OP_1` inputs. 
This change is strictly additive: existing taproot sighash computation for `OP_1` inputs is unaffected. 
The condition `scriptPubKey[0] == OP_1 || scriptPubKey[0] == OP_3` does not change the behavior for 
any existing code path.

**Script flag activation** 
— `GetBlockScriptFlags()` adds `SCRIPT_VERIFY_P2WOTS` to the block-level script flag set. 
This flag is additive and does not remove or modify enforcement of any existing flag 
(`SCRIPT_VERIFY_P2SH`, `SCRIPT_VERIFY_WITNESS`, `SCRIPT_VERIFY_TAPROOT`, etc.).

Wallets that do not implement P2WOTS continue to operate without modification.
Users migrate to post-quantum security at their own pace, P2WOTS addresses are pt-in, 
and classical output types remain fully valid indefinitely.

---

## **--Future Integrations--**

### End-to-End Quantum-Secure Lightning Network

The Lightning Network's security depends on the quantum security of its funding
outputs and commitment transactions. A Lightning channel is currently secured by a
P2WSH 2-of-2 multisig funding output — the weakest link in the quantum threat model
because the 2-of-2 EC keys are visible on-chain at channel open.

P2WOTS provides a direct upgrade path:

1. **Quantum-secure channel funding** — Replace the P2WSH 2-of-2 multisig funding
   output with a P2WOTS 2-of-2 output. The 89-item multisig witness (~680 vbytes)
   is the only on-chain cost increase. No BOLT protocol changes are required beyond
   switching the signing scheme used to construct the funding transaction.

2. **Quantum-secure commitment transactions** — Each commitment transaction output
   can be a P2WOTS address derived from the channel participants' WOTS master keys.
   The 64-slot MKT allows the same P2WOTS address to receive multiple commitment
   updates without violating the one-time property.

3. **Combined with BIP-360** — A channel that uses P2WOTS for funding and P2QRH for
   HTLC outputs achieves end-to-end post-quantum security across all transaction
   types in a payment path. This is the target architecture for a fully
   quantum-resistant Lightning Network.

### Hardware Wallet Support

The entire P2WOTS signing stack is built on SHA-256. Every hardware wallet that
supports Bitcoin already contains a SHA-256 accelerator. The signing operation
(34 independent chain computations of up to 255 SHA-256 evaluations each) is
parallelizable and does not require elliptic curve arithmetic. The 42-item witness
can be constructed and transmitted incrementally without holding the full 1088-byte
signature in RAM simultaneously, making P2WOTS well-suited to memory-constrained
signing devices.

### Script Covenants (CTV / CSFS)

P2WOTS is compatible with proposed covenant opcodes (OP_CHECKTEMPLATEVERIFY,
OP_CHECKSIGFROMSTACK). Since P2WOTS commits to a `scriptPubKey` commitment rather
than a spending script, covenant restrictions on the output side compose naturally
with P2WOTS inputs. A vault scheme built on CTV can accept P2WOTS-signed inputs and
enforce quantum-secure withdrawal paths without modification to either proposal.


## **--Signet Connect Guide--**

The WOTS-39 signet is a live custom Bitcoin signet running the full P2WOTS patch
set. Blocks are produced and P2WOTS transactions are accepted right now. There are
three ways to interact with it, ordered from zero setup to full node access.

---

### Option A — Web Wallet (Zero Setup)

The WOTS-39 wallet runs entirely in the browser, hosted permanently on the permaweb.

```
https://block_opuslux.ar.io
```

Open it, click the **Bitcoin Signet** tab, generate a random new seed and
the wallet derives your P2WOTS addresses, fetches balances, and lets you send
transactions in one click. The in-wallet faucet tab drops signet coins directly to
your first address. This is the fastest path to a real P2WOTS transaction on a live
chain.

---

### Option B — REST API (Scripting / Tool Development)

Everything the wallet does is available over a plain HTTPS REST API. No authentication
required. All endpoints are live.

**Base URL**
```
https://103-214-71-4.sslip.io/faucet
```

---

#### `GET /api/stats`

Returns the current state of the signet chain.

```bash
curl https://103-214-71-4.sslip.io/faucet/api/stats
```

```json
{
  "balance":         238790.00047424,
  "blocks":          4876,
  "last_block_time": 1780887951
}
```
---------------------------------------------------------------------
| Field             | Description                                   |
|-------------------|-----------------------------------------------|
| `balance`         | Total faucet reserve in tBTC                  |
| `blocks`          | Current block height                          |
| `last_block_time` | Unix timestamp of the most recent block       |
---------------------------------------------------------------------
---

#### `GET /api/utxos/{script_hex}`

Fetches the balance and UTXO set for a P2WOTS output. The path parameter is the
**34-byte scriptPubKey in hex** — not a bech32m address.

```
script_hex = "5320" + <32-byte commitment hex>
```

```bash
COMMITMENT="<your-32-byte-merkle-root-hex>"
curl "https://103-214-71-4.sslip.io/faucet/api/utxos/5320${COMMITMENT}"
```

```json
{
  "balance_sat": 100000000,
  "utxos": [
    {
      "txid":          "a1b2c3...",
      "vout":          0,
      "amount_sat":    100000000,
      "confirmations": 6,
      "script_hex":    "5320<commitment>"
    }
  ]
}
```

The endpoint rejects any `script_hex` that is not exactly 34 bytes or does not begin
with `5320` (`OP_3 PUSH32`).

---

#### `POST /api/broadcast`

Broadcasts a fully signed raw transaction to the signet mempool.

```bash
curl -X POST https://103-214-71-4.sslip.io/faucet/api/broadcast \
     -H "Content-Type: application/json" \
     -d '{"hex": "<signed_raw_tx_hex>"}'
```

```json
{ "txid": "a1b2c3d4..." }
```

On error the response is `{ "error": "<reason>" }`. The node enforces all standard
mempool policy rules plus full P2WOTS script validation. A broadcast failure means
the witness is malformed — not a transient network issue.

---

#### Faucet Web UI

```
https://103-214-71-4.sslip.io/faucet/
```

Paste a bech32m P2WOTS signet address. The faucet sends **1 tBTC** per request,
rate limited per IP. The in-wallet faucet tab pre-fills your address automatically
and is the easier path for quick testing.

---

#### Block Explorer

Every confirmed transaction can be browsed at:

```
https://103-214-71-4.sslip.io/tx/<txid>
```

P2WOTS inputs (witness version 3, 42 items) are shown alongside their decoded
commitment and auth path.

---

### Option C — Full Node (Protocol Development)

Connect a locally built patched `bitcoind` for direct RPC access, local
`sendrawtransaction`, and the ability to run the C++ unit and functional test suites.

#### 1. Build Bitcoin Core with the P2WOTS patches

All modified source files are included in this repository under `bitcoin/src/`. Copy
them onto a matching Bitcoin Core checkout:

```bash
git clone https://github.com/bitcoin/bitcoin.git
cd bitcoin
git checkout <base-commit>           # match the commit this fork was built from

# Overlay the P2WOTS patch files
cp -r /path/to/BitcoinProposal/bitcoin/src/* src/

# Build (Linux/macOS — use -j to match your CPU count)
cmake -B build -DBUILD_TESTS=ON
cmake --build build -j$(nproc)

# Confirm the P2WOTS cryptographic primitives pass all unit tests
./build/src/test/test_bitcoin --run_test=wots_tests
```

A clean `wots_tests` run with zero failures confirms the build is correct and all
hash domain separators match the reference implementation byte-for-byte.

---

#### 2. Retrieve the signet challenge

The `signetchallenge` is the serialized scriptPubKey of the signet block-signing key.
Run this once on the VPS (or ask the signet operator):

```bash
# On the VPS:
bitcoin-cli -signet getblockchaininfo | python3 -c \
  "import sys, json; print(json.load(sys.stdin).get('signetchallenge', 'not found'))"
```

Copy the printed hex — you will paste it into `bitcoin.conf` below.

---

#### 3. Configure bitcoin.conf

Create or edit `~/.bitcoin/bitcoin.conf`:

```ini
signet=1

[signet]
addnode=103.214.71.4:38333
signetchallenge=<paste-hex-from-step-2>
server=1
txindex=1
rpcuser=devuser
rpcpassword=devpass
rpcport=38332
```

The P2P port `38333` is open and the node accepts inbound connections. Syncing from
genesis takes under a minute — the chain is small.

---

#### 4. Start and verify

```bash
./build/src/bitcoind -signet -daemon

# Watch block height — should climb to match /api/stats "blocks" value
watch -n 5 "./build/src/bitcoin-cli -signet \
  -rpcport=38332 -rpcuser=devuser -rpcpassword=devpass \
  getblockcount"

# Confirm P2WOTS is active in the script flag set
./build/src/bitcoin-cli -signet \
  -rpcport=38332 -rpcuser=devuser -rpcpassword=devpass \
  getblockchaininfo | grep -A2 softforks
```

---

#### 5. Derive a P2WOTS address

```python
# requires: test/functional/test_framework/wots39.py
import sys, os
sys.path.insert(0, "test/functional/test_framework")
from wots39 import ComputeAddressNonce, ComputeMerkleRoot

# 32-byte master secret — controls all derived keys, store it securely
msk = os.urandom(32)

# Derive address index 0 (increment for each new address)
addr_nonce  = ComputeAddressNonce(msk, address_index=0)
merkle_root = ComputeMerkleRoot(msk, addr_nonce)

# 34-byte scriptPubKey: OP_3 (0x53) || PUSH32 (0x20) || commitment[32]
script_hex = "5320" + merkle_root.hex()
print("scriptPubKey:", script_hex)

# bech32m encode with witness version 3 and HRP "tbs" for signet
# (use any standard bech32m library — the witness program is merkle_root)
```

---

#### 6. Fund the address

```bash
# Paste the bech32m address into the faucet UI, or POST directly:
curl -X POST https://103-214-71-4.sslip.io/faucet/ \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "address=<your-bech32m-p2wots-address>"
```

---

#### 7. Sign and broadcast a P2WOTS transaction

Construct the full 42-item witness using `wots39.py`, serialize the raw transaction,
then broadcast either through your local node or the REST API:

```bash
# Via local bitcoind
./build/src/bitcoin-cli -signet \
  -rpcport=38332 -rpcuser=devuser -rpcpassword=devpass \
  sendrawtransaction "<signed_raw_tx_hex>"

# Or directly via REST (no local node needed)
curl -X POST https://103-214-71-4.sslip.io/faucet/api/broadcast \
     -H "Content-Type: application/json" \
     -d "{\"hex\": \"<signed_raw_tx_hex>\"}"
```

---

#### 8. Run the test suite

```bash
# C++ unit tests (fast, no live node required)
./build/src/test/test_bitcoin --run_test=wots_tests

# End-to-end functional tests against a local regtest node
python3 test/functional/feature_wots39.py

# Benchmark: signing throughput and per-block verification overhead
python3 test/functional/bench_wots39.py
```

---

### Network Reference
-----------------------------------------------------------------------
| Resource              | Value                                       |
|-----------------------|---------------------------------------------|
| P2P peer              | `103.214.71.4:38333`                        |
| Block explorer        | `https://103-214-71-4.sslip.io/`            |
| Faucet UI             | `https://103-214-71-4.sslip.io/faucet/`     |
| REST API base         | `https://103-214-71-4.sslip.io/faucet`      |
| Web wallet            | `https://block_opuslux.ar.io`               |
| Network               | Custom signet — P2WOTS patch set active     |
| Block interval        | 10 minutes                                  |
| Faucet amount         | 1 tBTC per request                          |
| HRP (bech32m signet)  | `tbs`                                       |
-----------------------------------------------------------------------