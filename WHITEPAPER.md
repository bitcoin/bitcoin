# Elektron Net – Whitepaper
**Version 2.2 | Author: Ali Kutlusoy | Graz, Austria, 2026**

*"From the Lydian Elektron to the Digital Elek – Value Through Mathematics"*

---

## Prologue: On Freedom and Responsibility

*A word from the author*

This protocol was not created by an institution, a corporation, or a committee. I write this as a single human being who holds, without reservation, that every person is born free – and that this freedom must be actively defended in a digital world that was not built with freedom in mind.

Marcus Aurelius wrote: *"You have power over your mind – not outside events. Realise this, and you will find strength."* The Stoics understood something that most modern systems forget: freedom is not granted from above. It is exercised from within. It is a discipline, not a gift.

Elektron Net is my contribution to that discipline. It is a tool built from mathematics and code, inspired by 2,600 years of human history – from the Lydian merchants who traded without a king's permission to the cryptographers who proved that two strangers could share a secret without ever meeting. It is a tool. Nothing more, and nothing less.

**On authorship and control:**
I lit the first spark, but I do not own the fire. By publishing this protocol under the MIT licence, I relinquish all control. There are no administrators, no backdoors, no central authority – including me.

**On the Stoic nature of this system:**
The Stoics distinguished between what is *up to us* and what is *not up to us*. Elektron Net is designed around this distinction. What is up to the mathematics: your balance belongs to you, your payments are anonymous, your history is forgotten after 137 days. What is not up to us: market prices, adoption rates, the decisions of others. We build for what we can control.

**On responsibility:**
Freedom without responsibility is noise. If this tool gives you the power to act without surveillance, use that power to build something you would be proud of in full light.

*"Waste no more time arguing about what a good man should be. Be one."*
— Marcus Aurelius

---

*Ali Kutlusoy*
*Graz, Austria, 2026*
*https://www.elektron-net.org*

---

# Part I – Why

# 1 – Vision, Philosophy & Historical Origin

## The Lydian Elektron

Around 600 BC, the Lydians in Asia Minor struck the first coins in the world. They were made of Elektron – a naturally occurring alloy of gold and silver. These coins fundamentally changed civilisation: for the first time, a person could trade without giving their name. Value became transferable, divisible, anonymous.

The Lydian Elektron coins were the first decentralised currency in history. No king had to be present. No scribe had to record the exchange. The value lay in the material itself – in the mathematics of the alloy.

2,600 years later, the world faces the same problem, solved with the same means: mathematics instead of trust.

## 1.1 – The Pocket Philosophy

Elektron Net is deliberately built around a simple, ancient human experience: When you put on your pants in the morning and reach into your pocket, you immediately know three things:

- **How much money you have right now** — not last week, not last month.
- **That this money is yours** — because it is your pocket.
- **You do not need to remember where each coin came from**, what you bought yesterday, or who paid you last year.

*The past is irrelevant. Only the present matters.*

This everyday reality is the guiding metaphor for the entire protocol.

**Your UTXO set is your pocket.** It contains exactly what you possess in this moment. Nothing more, nothing less. It is the only permanent data structure in the network.

**137 days of pruning is natural forgetting.** Just as you do not keep receipts from five months ago in your pocket, the network mathematically erases all transaction history after exactly 137 days (α⁻¹). Not because someone ordered it, but because the protocol makes it impossible to keep. No subpoenas, no archives, no eternal digital footprint.

**Stealth addresses ensure no one can look into your pocket.** Observers may see that coins are moving in the world, but they cannot see into your pocket — nor connect one payment to another. Your financial life remains private by default.

**Wallet recovery without history is finding your pocket again.** If you lose your phone or reinstall the software, you only need your 24-word seed. The protocol lets you scan the current UTXO set, recognize what belongs to you, and reclaim it — without needing any past transaction records. Your pocket is always recoverable from the present state alone.

**Rewards for simply being online is wearing the pants.** You do not need to actively trade, stake, or perform complex tasks. By keeping your node running — by simply being present — you earn a fair share. Like carrying your pocket with you through the day.

This is not a marketing slogan. It is the architectural soul of Elektron Net.

The network lives in the Here and Now, exactly like a Stoic who focuses only on what is within his power in the present moment. The mathematics secures your money. Time gently erases your traces. You remain free to act with what you truly own today.

> *"Mathematics secures your money.
> Time erases your traces.
> You own the moment."*

This pocket philosophy is not an afterthought. It is the reason the system was built the way it is — light, private, fair, and profoundly human.

## 1.2 – The Problem of the Digital Present

The internet was not built for people. It was built for institutions.

```
Today:
  Your bank account      →  can be frozen at any time, without reason
  Your payment           →  requires a bank's permission
  Your transaction       →  stored, analysed, reported
  Your identity          →  owned by a corporation
  You                    →  are the product
```

## 1.3 – The Vision

Elektron Net is a decentralised digital currency built on three principles:

**Light** – The network forgets everything after 137 days. Light in storage. Light in memory. Light in surveillance.

**Anonymous** – Every payment uses stealth addresses. No one can link your transactions. Not your government. Not your bank. Not anyone.

**Fair** – Every participant who stays online earns a fair share. The producer earns extra for doing the work. Everyone else earns together.

```
Elektron Net:
  Your wallet            →  yours forever, mathematically guaranteed
  Your payment           →  direct, fast, anonymous
  Your balance           →  cannot be frozen by anyone
  Your rewards           →  earned by simply being online
  The network            →  alive from block #1, with or without peers
  Your history           →  forgotten after 137 days
  You                    →  are the owner
```

## 1.4 – The Promise

```
"A network where your wallet belongs to you.
 Where you are anonymous when you choose to be.
 Where nobody can freeze your money.
 Where nobody can trace your payments.
 Where you are not the product.
 Where mathematics governs, not institutions.
 Where a single node is enough to begin.
 Where 137 days later, everything is forgotten.

 Like the Lydian Elektron 2,600 years ago –
 value through mathematics, not through decree."
```

## 1.5 – The Two Numbers

```
21,000,000
  Maximum Elek. Immutable forever.
  The same cap as Bitcoin. A commitment to scarcity and honesty.

α⁻¹ = 137
  The fine-structure constant – the most fundamental dimensionless
  constant in physics. Describes the interaction of the electron
  with the electromagnetic field.
  "A mystery ever since it was discovered." – Richard Feynman

  In Elektron Net, 137 is present throughout:
    137 seconds   → one-time PoW for wallet creation
    137 days      → pruning window (forgotten by mathematics)
    137           → maximum VRF committee size
```

---

# Part II – How It Works

# 3 – The Currency

## Units

```
UNIT          SYMBOL    RELATION           NAMED AFTER
──────────────────────────────────────────────────────────────
Elektron      Elek      base unit          Lydian Elektron coin
Lepton        Lep       1/100,000,000 Elek Smallest Greek coin
Lepton Cent   cLep      1/100 Lepton       Sub-unit precision
──────────────────────────────────────────────────────────────

1 Elek = 100,000,000 Lep = 10,000,000,000 cLep
```

## Maximum Supply

```
21,000,000 Elek.
Anchored in Block #0.
Enforced by every node on the network.
Unchangeable by any person, company, or government.

This is not a promise. It is a mathematical property.
```

## The Emission Schedule

Every 60 seconds, a new block is produced. Every block creates new Elek. The amount decreases by half every 4 years – identical to Bitcoin's rhythm, because it works.

```
Block time:            60 seconds
Blocks per day:        1,440
Genesis block reward:   5.00000 Elek
Halving interval:      4 years (2,102,400 blocks)

Year  1– 4:  5.00000 Elek     7,200/day      10,512,000 total
Year  5– 8:  2.50000 Elek     3,600/day      15,768,000 total
Year  9–12:  1.25000 Elek     1,800/day      18,396,000 total
Year 13–16:  0.62500 Elek       900/day      19,710,000 total
Year 17–20:  0.31250 Elek       225/day      20,367,000 total
Year 21–24:  0.15625 Elek       113/day      20,695,500 total
...
~Year 115:   ≈0.00000               0      21,000,000 max
```

**No airdrop. No pre-mine. No founder allocation. No venture capital.**

Every Elek in existence must be earned by running the software. This is not charity. This is the only honest way.

---

# 4 – Technical Foundation

## Mathematics as Constitution

Security rests not on trust in institutions but on the mathematical unsolvability of certain problems. This is not faith – it is 50 years of cryptographic research confirmed by numerous attacks.

## Curve25519

```
y² = x³ + 486662x² + x  (mod 2²⁵⁵ - 19)

128-bit security. Fastest secure curve for ECDH.
Designed by D.J. Bernstein, 2006.

private_key = random 256-bit number
public_key  = private_key × G

pubKey from privKey: milliseconds
privKey from pubKey: longer than the age of the universe
```

## Ed25519 – Signatures

```
Sign:    sig = Ed25519_Sign(private_key, BLAKE3(message))
Verify:  Ed25519_Verify(public_key, BLAKE3(message), sig)

128-bit security. Deterministic. ~100,000 signatures/second.
```

## BLAKE3 – Hashing

Fastest modern cryptographic hash function. Used for wallet addresses, block hashes, Merkle roots, PoW, stealth addresses, and checkpoint digests.

## Merkle Trees

```
Block transactions:
  H(A)  H(B)  H(C)  H(D)
     \  /       \  /
    H(AB)       H(CD)
        \       /
        H(ABCD) ← Merkle Root

To prove Tx_A exists: provide H(B) and H(CD).
No other transaction revealed. Tamper-proof.
```

## Technology Stack

```
IMPLEMENTATION:
  Language:      C++20 (performant, zero-overhead abstractions, no GC)
  Async:         Boost.Asio (async runtime for network I/O)
  Networking:    cpp-libp2p / Boost.Asio (peer-to-peer, NAT traversal, encryption)
  Storage:       RocksDB (embedded key-value database, native C++ API)
  Serialization: nlohmann/json + Protocol Buffers (JSON/binary encoding)
  Build:         CMake (cross-platform build system)

CRYPTOGRAPHY:
  Keys/Signatures: Ed25519 (libsodium – deterministic, fast)
  Hashing:         BLAKE3 C library (addresses, PoW, stealth, checkpoints)
  VRF:             ristretto255 via libsodium (committee selection)
  BLS:             blst C library (aggregate signatures for pBFT finality)

WALLET:
  Mnemonics:     BIP-39 (24-word seed phrases, trezor-crypto or custom)
  Derivation:    BIP-32 (HD wallet, m/137'/0'/N paths, libwally-core)
  Addresses:     Bech32m with "elek1" prefix
  Privacy:       Bloom-Filter (local, offline stealth scanning)

INTERFACES:
  Daemon:        elektrond (background service)
  CLI:           Command-line interface for node control (CLI11)
  RPC:           JSON-RPC (port 9337, for wallet/explorer integration)
  GUI:           eGUI (Qt-based desktop wallet, Bitcoin Core-like)
```

## Proof-of-Work: 137 Seconds

```
BLAKE3(wallet_pubkey || nonce) meets difficulty target
in exactly 137 seconds (α⁻¹) on reference hardware.
Calibrated automatically every 10,000 wallets.

Purpose: prevent mass wallet creation (Sybil resistance).
Cost:    one-time, paid once at wallet creation, never again.
No PoW involved in block production or rewards.
```

---

# 5 – Wallet System

## Philosophy: Identity-Free by Design

```
Private key → Public key → Address → Balance
No username. No account. No registration beyond PoW.

Address encoding: Bech32m with prefix "elek1"
HD derivation:    m/137'/0'/N  (coin type = α⁻¹)
```

## Wallet Creation

```
WALLET CREATION FLOW:

  Step 1: Generate master seed
          BIP-39: 24-word mnemonic (256-bit entropy, CSPRNG)
          User writes down seed phrase – the only backup.

  Step 2: Mandatory seed verification
          5 random positions selected from the 24 words.
          User must enter them correctly before PoW begins.

  Step 3: Derive keypair (BIP-32 HD, path m/137'/0'/0)
          master_seed → (private_key, public_key)

  Step 4: Proof-of-Work (~137 seconds, one-time only)
          BLAKE3(public_key || nonce) < difficulty target
          Prevents mass wallet creation (Sybil resistance).

  Step 5: Registration broadcast
          { public_key, pow_nonce, timestamp, sig }
          Included in next block by current producer.
          If not included within 1440 blocks: retry with new timestamp.

  Step 6: Wallet active
          Address = Bech32m("elek1", BLAKE3(public_key)[0:20])
          Rewards begin immediately if online.
```

## Stealth-Integrated Wallet

Every wallet includes a dual-key stealth structure at creation:

```
scan_privkey,  scan_pubkey  = derive(seed, m/137'/0'/1)
spend_privkey, spend_pubkey = derive(seed, m/137'/0'/0)

Published stealth address: (scan_pubkey, spend_pubkey)
All on-chain receipts use stealth by default.
```

---

# 6 – Network Architecture & Peer-to-Peer

## Bitcoin Core as Foundation

Elektron Net builds directly on the Bitcoin Core peer-to-peer network layer, implemented in C++:

```
Inherited from Bitcoin Core:
  → message serialisation (magic bytes + command + payload)
  → block propagation (inv → getdata → block)
  → transaction relay (mempool sync)
  → headers-first sync
  → DoS protection (banning, score-based)

Elektron Net modifications:
  → IPv6 as primary address family
  → 60-second block time
  → pBFT-style finality layer
  → VRF committee for block production
  → Genesis Mode: single-node operation
  → Stealth address relay
  → 137-day pruning enforced at protocol level
```

## Two-Tier Node Architecture

Elektron Net distinguishes between two types of nodes, each with different roles and responsibilities:

```
┌─────────────────────────────────────────────────────────────┐
│                    TWO-TIER ARCHITECTURE                    │
├─────────────────────────────────────────────────────────────┤
│  USER NODE (Tier 1)                                        │
│    - Runs wallet software                                   │
│    - May go offline at any time                             │
│    - Cannot participate in VRF committee                   │
│    - Cannot produce blocks (no 25% producer share)          │
│    - Receives epoch pool rewards (75% share) if online      │
│    - After 137 days online → eligible for Full Node        │
├─────────────────────────────────────────────────────────────┤
│  FULL NODE (Tier 2)                                        │
│    - 137+ days continuous online (verified via heartbeats)  │
│    - Genesis Phase: ALL nodes are Full Nodes               │
│    - Forms VRF committee (137 randomly selected per slot)   │
│    - Signs blocks and checkpoints (BLS aggregate)           │
│    - Receives 25% producer share + epoch pool rewards       │
│    - Voluntary: user enables FN mode, status persists       │
└─────────────────────────────────────────────────────────────┘
```

**Full Node Qualification:**
- Minimum: 137 days of online time recorded (heartbeats)
- Voluntary: user clicks "Enable Full Node Mode" in client
- Heartbeats continue while online to build qualification time
- Genesis Phase: all nodes are automatically Full Nodes

**Full Node Status (Three States):**

| State | Condition | Counts for Quorum | Epoch Pool (75%) | Reactivation |
|-------|-----------|-------------------|---------|--------------|
| ACTIVE | FN enabled + online | YES | YES (if in window) | - |
| DORMANT | 137+ days without heartbeat | NO | NO | 1 heartbeat → ACTIVE |
| USER | Never was FN or manually switched | NO | YES (if in window) | 137 days again |

- Hardware defects, network outages, vacations: NO penalty
- DORMANT after 137 days offline: does NOT count toward quorum
- Reactivation: instant (1 heartbeat) — no new 137-day wait
- User can manually switch back to User Node at any time

**Why This Design:**
- Short offline periods: instant recovery, NO penalty
- Network stability: Dormant FNs don't bloat the quorum
- Registry stays clean: truly dead nodes can be manually removed
- Voluntary + Persistent + Dormant = best of all worlds

## Bootstrap & Node Discovery

```
STEP 1: DNS seeds (config file – minimum 5, geographically distributed, user-editable)
STEP 2: Bootstrap Server (PHP-based registry at www.elektron-net.org/bootstrap.php)
STEP 3: addr relay (peers share known addresses)
STEP 4: peer.db (saved peer list from previous sessions)
STEP 5: Hardcoded fallbacks (localhost for testing)
STEP 6: Genesis Mode (if no peers found - single node fully operational)
```

### Bootstrap Server (PHP Registry)

Full Nodes register dynamically via HTTPS:

- **Endpoint:** `https://www.elektron-net.org/bootstrap.php`
- **Actions:** `list`, `register`, `heartbeat`, `stats`
- **Node ID:** BLAKE3 hash of `pubkey:ip:port`
- **IP Support:** IPv6 preferred, IPv4 fallback
- **Heartbeat:** Required every 15 minutes (timeout: 1 hour)
- **Storage:** `bootstrap_nodes.json` (auto-cleaned)

See `DEV-README.md` for full Bootstrap Server API documentation.

## Genesis Node Chain Authentication (Genesis Hash Guard)

The first node ever to start on a given chain is the **Genesis Node**. Because
starting as Genesis Node creates a new, independent chain, the protocol enforces
an explicit authentication step to prevent both accidents and malicious chain forks.

### The Problem

Without a guard, any operator who finds no peers — due to a network outage,
misconfiguration, or deliberate isolation — could inadvertently fork the chain
by silently starting as Genesis Node. Other nodes would reject that fork, but
real damage (confusion, wasted blocks, split communities) could still result.

### The Mechanism

When a node completes all 6 bootstrap steps and finds **zero reachable peers**,
and the selected network is **mainnet**, the daemon presents an interactive menu:

```
================================================================================
  No bootstrap peers or other peers found.
================================================================================

  Options:
    [1]  Retry peer discovery
    [2]  Start as Genesis Node  (requires mainnet genesis hash)
    [3]  Exit
```

Choosing option **[2]** requires typing the exact 64-character mainnet genesis hash:

```
13703598e26d1536b62146c5ad1a64264d7f72167737a0d7662c5d1937b3e136
```

This hash is **hardcoded** in the binary (`GenesisConfig::mainnet()`).
The operator input is compared **byte-for-byte** against the stored value.
Any deviation — wrong character, wrong case, extra space — is immediately rejected
and the menu is shown again. There is no way to bypass this without modifying
the source code.

### Chain Identification in Peer Communication

The genesis hash is not only checked locally. It is included in every outgoing
bootstrap request:

- **Bootstrap server registration:** `POST` body contains `genesis_hash`
- **Bootstrap peer list query:** `GET` query contains `genesis_hash=…`
- **P2P handshake** (Phase 1): nodes exchange genesis hashes; mismatches disconnect

This means the bootstrap server only returns peers that registered with the same
genesis hash. Nodes on a different chain are never returned, never connected to,
and never waste bandwidth.

### Network-Mode Behaviour

| Network | Behaviour when no peers found |
|---------|-------------------------------|
| `mainnet` | Interactive menu — genesis hash confirmation required |
| `testnet` | Automatic Genesis Node start with warning message |
| `devnet`  | Automatic Genesis Node start, no prompts |

### Relation to Sybil Resistance

The two mechanisms protect against different threats:

| Mechanism | Protects Against |
|-----------|------------------|
| Wallet PoW (~137 s, one-time) | **Sybil:** mass identity / wallet creation |
| Genesis Hash Guard (hash entry) | **Fork:** accidental or malicious parallel chains |

The PoW makes it expensive to create thousands of identities.
The Genesis Hash Guard ensures only one authorised mainnet chain can ever exist.
Neither mechanism can be circumvented without modifying and recompiling the binary.

### Why the Hash Is Not a Secret

The genesis hash is publicly visible in the source code. Its protective value
does not come from secrecy — it comes from requiring **deliberate, conscious
action**. An automated script, a misconfigured node, or a careless operator
will stop at the prompt. Only someone who explicitly knows and types the hash
can proceed.

> **The genesis hash of Elektron Net mainnet:**
> `13703598e26d1536b62146c5ad1a64264d7f72167737a0d7662c5d1937b3e136`

---

## Genesis Mode: Solo Operation

If no peers are found and the Genesis Hash Guard is passed, a single node
operates alone:
- Produces blocks
- Validates transactions
- Earns all rewards
- Full functionality, no minimum required

This is not a fallback. This is a feature. The network is alive from block #1.

---

# 7 – Consensus: VRF Committee & Block Production

## Two Modes

The network operates in two modes depending on the number of Full Nodes.

### Genesis Mode (FN < 137)

When fewer than 137 Full Nodes exist:
- ALL nodes are Full Nodes (Genesis Phase - automatic)
- Simple rotation selects the block producer
- No VRF committee
- No BLS checkpoint signatures
- Ed25519 only
- Full reward to producer (25% + 75% pool)
- Automatic promotion of candidates if needed

### Phase 1 (FN ≥ 137)

When 137 or more Full Nodes are active:
- VRF committee selection (from Full Node Registry only)
- Fixed committee size: C = 137
- pBFT finality (≥2/3 signatures required = 92 votes)
- BLS aggregate signatures
- Only Full Nodes can participate in VRF committee

## Committee & Byzantine Threshold

```
PHASE 1 PARAMETERS:

  Committee Size:    C = 137 (always maximum, when Phase 1 active)
  Byzantine Threshold: T = ceil(C x 2/3) = 92 votes required
  Tolerated Byzantine: 45 nodes (137 - 92)

  Why 137 + 2/3?
    - VRF randomization makes coordinated attacks exponentially harder
    - Even with 30% of network controlled by attacker
    - Probability of 92 attackers in random 137-committee: negligible
    - No fork needed: security is built-in from day one
```

## VRF Selection

Every slot, the committee is selected by VRF:

```
STAGE 1 — SELECTION:

  1. Each FULL NODE computes:
     vrf_input = hash(slot_number || last_checkpoint_hash || utxo_root)
     vrf_output = VRF_prove(node_privkey, vrf_input)

  2. Sort by vrf_output (lowest = best)

  3. Top C nodes = committee for this slot

  4. Within committee: P1 (committee[0]), P2 (committee[1]), P3 (committee[2])

NOTE: last_checkpoint_hash + utxo_root are used to prevent long-range
manipulation. The checkpoint hash changes every 137 days; the UTXO root
changes with every transaction, making manipulation impossible.

STAGE 2 — VOTING:

  5. Each committee member validates the block
     Every member has exactly 1 vote

  6. Threshold: T = ceil(C x 2/3) = 92 votes required for finality

  7. P2 collects signatures → BLS aggregate → final block
```

## Block Production Flow

```
Every 60 seconds:

  slot = unix_time / 60
  slot_start = slot × 60
  valid_production_window = [slot_start, slot_start + 45 seconds]

  t =  0s:   P1 produces and broadcasts
  t = 15s:   If P1 failed → P2 prepares
  t = 30s:   If P2 ready → P2 broadcasts
  t = 40s:   If P1,P2 failed → P3 broadcasts
  t = 60s:   If T signatures not collected → slot skipped

  Committee votes on the first valid block received.
  If ≥2/3 vote: block is final (pBFT safety guaranteed).
```

## Pacemaker

If slots are skipped:
```
k = consecutive skipped slots
timeout = 60s × 2^min(k, 10)
VRF committee is redrawn
Offline nodes are unlikely to be re-selected (VRF is independent per slot)
```

---

# 8 – The Reward System

*This is the heart of the protocol.*

## The Philosophy of Fairness

Elektron Net makes no future promises. Rewards are distributed fairly and instantly. A network lives in the *Now*. Anyone who is online today earns today. There are no complex calculations, no retroactive claims.

## The Rule

```
Every block reward is distributed as follows:

  25% → Block Producer (paid INSTANT, every block)
  75% → Pool (paid every 10 minutes, at epoch end)
```

## Why This Model

```
Simplicity:   Only two numbers (25% and 75%)
Fairness:     Everyone online shares the pool
Incentive:    Producer gets extra for doing the work
Scalability:  21,000 node cap prevents chain bloat
Efficiency:   10-minute batching = 10x less UTXOs
```

## Online Definition (Rolling Window)

```
A node is "online" if it participated in consensus during the current slot
OR in one of the last K slots (S-K to S-1).

K = 10 slots (10 minutes at 60s block time)

Participation = one of:
  1. Selected in the VRF committee, OR
  2. Produced or validated a block, OR
  3. Sent a heartbeat message (included in block)
```

## Heartbeat Mechanism (Quantum-Safe)

The heartbeat system uses BLAKE3-based HMAC — no Ed25519 signatures, no quantum computer vulnerability.

```
DESIGN PRINCIPLES:
  - Quantum-safe: BLAKE3 hash-based, no ECC public keys
  - Minimal storage: 0 bytes on-chain
  - Minimal compute: 1× BLAKE3 per heartbeat

REGISTRATION (once, off-chain):
  Node computes:
    hmac_secret = BLAKE3(spend_privkey || "heartbeat")
  Node sends secret to current Producer (encrypted P2P message)

EVERY SLOT:
  Node computes:
    heartbeat_hash = BLAKE3(hmac_secret || slot_number)
  Node sends: { heartbeat_hash, slot_number } to Producer
    (48 bytes total: 32 hash + 8 slot + 8 overhead)

  Producer validates:
    expected = BLAKE3(stored_hmac_secret || slot_number)
    If match → Node is online

WHY THIS IS QUANTUM-SAFE:
  - BLAKE3 is a hash function (not ECC)
  - No Ed25519 public key is ever transmitted in heartbeat
  - Even if Ed25519 is broken by quantum computer,
    heartbeats remain secure
```

## Node Cap & Epoch Batching

```
REWARD CYCLE: Every 10 minutes (1 epoch = 10 blocks)
→ Up to 21,000 DIFFERENT nodes receive rewards EACH epoch
→ Next epoch: potentially different 21,000 nodes
→ Every epoch is independent — your odds reset each time
```

```
To prevent chain bloat:

  NODE CAP: Maximum 21,000 nodes receive pool rewards per epoch
  If online_nodes > 21,000: selection by deterministic VRF

  EPOCH: 10 blocks (10 minutes)
  At epoch end: ALL pool rewards aggregated into ONE transaction
  Result: max 21,000 UTXOs per epoch (instead of 21,000 per block)

SELECTION TRANSPARENCY:
  VRF input = hash(slot_number || last_checkpoint_hash)
  All nodes can independently verify the selection result.
  The producer cannot manipulate which nodes are selected —
  VRF output depends only on publicly known inputs.

COMPARISON – Chain Bloat Prevention:
  Without batching: 21,000 × 1,440 blocks = 30,240,000 UTXOs/day
  With 10-min epoch: 21,000 × 144 epochs = 3,024,000 UTXOs/day
  Reduction: 10x smaller ✓
```

## Reward Distribution Timeline

```
BLOCK PRODUCTION (every 60 seconds):
  Producer receives: 25% of block_reward IMMEDIATELY
  Pool share (75%): accumulates for 10 blocks

EPOCH END (every 10 blocks = 10 minutes):
  1. Sum all accumulated pool shares (10 blocks × 75%)
  2. Count unique online nodes (max 21,000)
  3. per_node = pool_sum / min(online_count, 21000)
  4. Create 1 UTXO per eligible node
```

## Examples

```
Scenario 1: Solo node (N = 1)
  Block reward: 5 Elek
  Producer receives: 25% + 75% = 100% = 5 Elek
  Pool: 0 (only producer)

Scenario 2: 10 nodes online, epoch reward = 37.5 Elek
  Pool per node: 37.5 / 10 = 3.75 Elek
  Producer: 1.25 (instant) + 3.75 (epoch) = 5 Elek ✓

Scenario 3: 100 nodes online
  Pool per node: 37.5 / 100 = 0.375 Elek
  Producer: 1.25 + 0.375 = 1.625 Elek

Scenario 4: 1,000,000 nodes online (capped at 21,000)
  Pool per node: 37.5 / 21000 = 0.00178 Elek
  Producer: 1.25 + 0.00178 = 1.25178 Elek
```

## Scalability – 10-Year Projection

```
CHAIN SIZE AFTER 10 YEARS:

  Blocks: 5,256,000 (at 60s intervals)
  Block data (rolling 137-day window): ~40 GB
  Block headers: ~420 MB
  UTXO Set: ~5 GB
  Checkpoints: ~1 MB

  TOTAL PERMANENT STORAGE: ~5.5 GB

  vs Bitcoin (same timeframe): ~550 GB
  Elektron Net is ~100x smaller due to 137-day pruning

WHY THIS IS POSSIBLE:
  - 137-day pruning deletes all transaction history automatically
  - Only UTXO set, headers, and checkpoints are permanent
  - 21,000 node cap limits reward UTXOs to 3M/day (not billions)
```

## Transaction Fees

```
Market-based. Senders attach fees to incentivise block inclusion.
Block producer collects 100% of fees in their block.
No burning. No protocol treasury.
Minimum fee: 100 cLep (= 1 Lep) per transaction.
```

---

# 9 – Privacy & Pruning

## The Philosophy of "Pants & Pocket"

Elektron Net is modeled after real, physical life. When you put on pants in the morning and find coins in your pocket, only the present moment matters:

- **The Current State:** You know how many coins you have *right now*.
- **The Sovereignty:** Because it is your pants, you know the coins are yours.
- **The Forgetfulness:** You don't need to remember what you bought at the supermarket months ago.
- **The Focus:** What matters is only what you possess in the present moment to be capable of acting.

This human experience is translated directly into code.

## The Core Principle

Elektron Net does not store transaction history indefinitely. Transaction data is deleted after exactly 137 days – by mathematics, not by policy.

This is not GDPR compliance. It is a structural impossibility of data retention. No company can be subpoenaed. No server can be hacked. No government can demand records because those records simply no longer exist.

### UTXO as the "Pocket"

The network permanently stores the current status of all unspent amounts (UTXOs). This corresponds to reaching into your pocket:
- It shows your current balance in the "Here and Now."
- Mathematical security (Ed25519) replaces the physical feeling of the cloth pocket — only you have access.

### The 137-Day Pruning (The Forgetting)

Like in real life, the network deletes history after 137 days (α⁻¹).
- **No Archive:** There is no permanent record of your past spending for institutions.
- **Stoic Calm:** The system frees the user from the burden of an eternal digital file.

*"Mathematics secures your money. Time erases your traces. You own the moment."*

## Stealth Addresses

Every on-chain payment uses stealth addresses by default. Observers see only one-time addresses that cannot be linked to any recipient wallet or to each other.

```
SETUP (once per wallet):
  Recipient publishes scan_pubkey and spend_pubkey

PER PAYMENT:
  Sender generates ephemeral keypair (r, R = r×G)
  one_time_address = BLAKE3(r × scan_pubkey) × G + spend_pubkey
  Transaction sent to one_time_address; R included in tx data

RECIPIENT DETECTION (Bloom-Filter):
  Instead of scanning every transaction, the wallet uses a Bloom-Filter
  for efficient, privacy-preserving detection.

  STEP 1 – Generate filter at wallet creation:
    filter_items = []
    For N = 0 to 2^20 (approx 1 million possible outputs):
      r_N = derive_ephemeral_scalar(seed, N)
      R_N = r_N × G
      candidate = BLAKE3(scan_privkey × R_N) × G + spend_pubkey
      filter_items.push(candidate)
    bloom_filter = create_bloom(filter_items, false_positive_rate=0.001)

  STEP 2 – Scan incoming UTXO set:
    For each UTXO output address:
      If bloom_filter.might_contain(address):
        → POSSIBLE MATCH (check further)
      Else:
        → Definitely not mine (no false negatives)

  STEP 3 – Verify possible matches:
    For each POSSIBLE MATCH:
      r = derive_ephemeral_scalar(tx_ephemeral_R)
      candidate = BLAKE3(scan_privkey × R) × G + spend_pubkey
      If candidate == output_address:
        → CONFIRMED MATCH
        spend_key = BLAKE3(scan_privkey × R) + spend_privkey

  PERFORMANCE:
    UTXO Set = 10M outputs
    Bloom-Filter = ~2.4 MB (at 0.1% FPR, 1M items)
    Lookups = 10M × O(1) = 10M hash checks (~100ms)
    False positives to verify = ~10,000 (acceptable)

  SCALABILITY:
    The Bloom-Filter contains THIS WALLET's possible addresses only —
    it does NOT grow with the network UTXO set.
    At 100M UTXOs: still ~2.4 MB filter, ~100,000 verifications needed.
    Filter size can be increased to 2^24 items (~38 MB) if wallet needs more addresses.

  SECURITY NOTES:
    - Bloom-Filter generation is 100% local (no data leaves the device)
    - Filter reveals nothing about wallet contents to network observers
    - An observer seeing the filter cannot derive any addresses
    - Filter can be regenerated from seed at any time

OBSERVER:
  Sees:    R, one_time_address, hidden amount
  Cannot:  link address to recipient
  Cannot:  link two payments to same recipient
```

## What Each Node Stores

```
ALWAYS (permanent):
  → Genesis block header (80 bytes, never deleted)
  → All block headers (80 bytes each, chain integrity)
  → UTXO set (current balances with stealth metadata)
  → Checkpoint chain (BLS-signed by committee, one per pruning period)

137 DAYS (then deleted):
  → Full transaction content
  → Input/output details

NEVER:
  → User identity
  → IP-to-wallet mapping
  → Permanent transaction history
```

## Checkpoint Trust Model

```
WHY CHECKPOINTS?

  After 137 days, all transaction history is pruned.
  New nodes need a way to verify the UTXO set is correct.

HOW IT WORKS:

  Every 137 days (197,280 blocks):
    1. Current VRF Committee (137 members) creates a Checkpoint
    2. Checkpoint contains:
       - Block height
       - UTXO Root Hash (Merkle root of all current UTXOs)
       - BLS Signatures from committee members
    3. Threshold: 92 of 137 signatures required (2/3 Byzantine))

TRUST MODEL:

  - Genesis Block: First block created by the first node, never deleted
    Its header contains the initial UTXO Root Hash

  - Checkpoints: Signed by VRF Committee (92/137 required)
    - Committee is randomly selected from ≥137 nodes
    - 92 honest signatures = trustworthy
    - Attacker needs 28+ colluding nodes to forge

RECOVERY PROCESS FOR NEW NODES:

  1. Download Genesis Block header from any peer
     → Contains initial UTXO Root Hash

  2. Download latest Checkpoint
     → Verify: 92/137 BLS signatures valid
     → Verify: UTXO Root matches expected value

  3. Download UTXO set from multiple peers (3+ sources)
     → Compare UTXO Root Hash across peers
     → If mismatch: block peers, try different ones
     → If match: UTXO set is verified

  4. Verify UTXO set integrity:
     → Recalculate Merkle root of downloaded UTXO set
     → Must match Checkpoint's UTXO Root Hash

WHY THIS IS SECURE:

  - Cannot forge Checkpoint without 92/137 committee members
  - Committee is randomly selected per slot (VRF)
  - UTXO Root comparison across peers detects tampering
  - No single point of trust required
```

## Storage Requirements

```
Component       Year 1    Year 5    Year 10
─────────────────────────────────────────────
UTXO Set        ~50 MB   ~1-2 GB   ~3-6 GB
Block Headers   <1 MB    ~12 MB    ~24 MB
Checkpoint      <1 MB    <1 MB     <1 MB
─────────────────────────────────────────────
Total           ~100 MB  ~2-3 GB   ~5-8 GB

Light Mode:     ~1 GB RAM, ~1 GB storage
```

## Wallet Recovery Without History

```
STEP 1: Enter 24-word seed phrase
        → Derive: scan_privkey, spend_pubkey

STEP 2: Generate Bloom-Filter locally (100% offline, no network contact)
        → Creates filter of all possible stealth addresses
        → ~2.4 MB, computed from seed only

STEP 3: Download current UTXO set from peers
        → Everyone has it, no special trust required
        → Only UTXO data, no private information

STEP 4: Apply Bloom-Filter to UTXO set
        → O(n) with tiny constant factor (10M UTXOs in ~100ms)
        → Returns ~10,000 possible matches at 0.1% FPR

STEP 5: Verify each candidate (full stealth derivation)
        → Confirm true positives
        → Discard false positives

STEP 6: Derive spend keys for confirmed UTXOs
        → spend_key = BLAKE3(scan_privkey × R) + spend_privkey

STEP 7: Wallet recovered
        → All funds accessible
        → No transaction history needed
        → Works after 137 days offline
        → 100% self-verified, no trusted third party
```

**Why this is secure:**
- The Bloom-Filter reveals NO information about your addresses to the network
- UTXO set download is read-only (you send nothing)
- All verification is local
- No server or oracle required

This is the power of privacy by mathematics. The network forgets. But you can always remember.

## The "Here and Now" Principle

```
What is my balance?        Check the current UTXO set.
Can I spend?               Yes, if the UTXO exists.
How do I recover?          Enter seed → Generate Bloom-Filter locally →
                           Scan UTXO set → Verify candidates → Done.
Is my history gone?        From the network – yes. From you – no.
```

---

# 10 – Payments

## On-Chain Payments

Standard wallet-to-wallet payments:
- Sender creates transaction (stealth address by default)
- Producer includes in block
- 60-second finality
- Private by default

## QR Code Payments

Every wallet supports receiving and sending payments via QR codes, similar to Bitcoin Core.

```
QR CODE FORMAT (elektron uri scheme):

    ELEKTRON:<address>?amount=<elek>&message=<text>

    Components:
        address:  Bech32m "elek1..." stealth address
        amount:   Optional, in Elek (not Lep)
        message:  Optional, payment description (max 128 chars)

    Example:
        ELEKTRON:elek1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9x7d9?amount=0.5&message=Coffee
```

### Receiving via QR Code

```
STEP 1: Generate QR Content
    User taps "Receive" in wallet
    System generates:
        address = wallet's current stealth address
        amount  = (optional) user-entered amount
        message = (optional) user-entered description
        uri = "ELEKTRON:" + address + "?" + params

    STEP 2: Encode as QR
        Encode uri as UTF-8 bytes
        Generate QR code matrix (version 1-40)
        Error correction: Level M (~15%)
        Render as QR image

    STEP 3: Display
        Show QR code on screen
        Show address as text below (copy button)
        Show amount and message if entered
        User can share QR image or save
```

```
QR CODE SIZES:
    Address only:        ~Version 2  (25×25 modules)
    With amount:         ~Version 4  (29×29 modules)
    With amount+message: ~Version 7  (37×37 modules)

    All use Level M error correction (15% recovery)
    Scanning distance: 0.5-1m typical, 0.2-0.5m with small QR
```

### Sending via QR Code

```
STEP 1: Scan QR
    User taps "Send" → "Scan QR"
    Camera activates
    Detect and decode QR code
    Parse ELEKTRON: URI

    STEP 2: Parse URI
        Validate: address starts with "elek1"
        Validate: address is valid Bech32m (checksum)
        Validate: amount (if present) is positive number
        Validate: message (if present) is <= 128 chars
        IF invalid → Show error, abort

    STEP 3: Confirm Payment
        Display parsed payment:
            To:      [address (first/last 6 chars shown)]
            Amount:  [elek]
            Message: [text if any]
        User confirms → proceed

    STEP 4: Create Transaction
        Build transaction to stealth address
        Sign with Ed25519
        Broadcast to network

    STEP 5: Status
        Show "Payment sent" with txid
        Monitor for confirmation
```

### Offline QR Codes (Payment Requests)

For merchants or recurring payments, QR codes can be pre-generated offline.

```
STATIC QR (no amount):
    User generates QR with only address
    Payer enters amount manually
    Use case: Donation addresses, tip jars

DYNAMIC QR (with amount):
    User generates QR with address + specific amount
    Amount is locked in QR
    Use case: Point-of-sale, invoices

INVOICE QR:
    Merchant's server generates unique address per invoice
    QR contains: address + amount + description + expiry
    Payer scans → confirms → pays
    Invoice marked paid when tx confirms
```

### BIP-21 Compatibility

Elektron Net QR codes follow BIP-21 URI scheme (compatible with Bitcoin wallets).

```
Standard BIP-21 (also works):
    bitcoin:1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa?amount=0.5&label=Alice

Elektron Net equivalent:
    ELEKTRON:elek1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9x7d9?amount=0.5&label=Alice
```

## ElektronChannels: Payment Channels

For fast, low-cost payments between two parties without touching the blockchain for every transaction.

```
CHANNEL LIFECYCLE:

  Open:    Both parties fund a 2-of-2 multisig on-chain
  Use:     Exchange signed state updates off-chain (instant)
           Higher sequence number always wins on-chain
  Close:   Cooperative (both sign) or Unilateral (36-block dispute)

OFFLINE OPERATION:
  Payments queue locally
  On reconnect: queue replayed in sequence order
  Strict sequence numbers prevent replay attacks
```

## HTLC: Multi-Hop Routing

HTLCs enable payments through intermediary channels without trusting them.

```
Alice → Bob → Carol → Dave (no direct channel required)

  1. Dave generates secret R, shares H = BLAKE3(R) with Alice
  2. Alice creates HTLC with Bob
  3. Bob creates HTLC with Carol
  4. Carol creates HTLC with Dave
  5. Dave reveals R → Carol claims → Bob claims → Alice settles

Result: atomic. Either all hops succeed, or all refund.

max_pending_htlc_per_channel = 137 (α⁻¹)
```

## Lepton Streams: Continuous Micropayments

```
Stream types:
  → Per-second  (video streaming, voice calls)
  → Per-KB      (data transfer, file sync)
  → Per-request (API metering, compute)

Sender opens stream → transfers cLep at defined rate
Receiver monitors stream → delivers service
Either party closes stream at any time
Final balance settled in the channel
```

---

# 11 – Security

## pBFT: Honest Majority Assumption

Phase 1 uses pBFT with fixed committee size C = 137 and 2/3 Byzantine threshold:

```
Committee:  C = 137 (always, when Phase 1 active)
Threshold: T = ceil(137 × 2/3 = 92 votes required
Tolerated Byzantine nodes: 45 (137 - 92)

Security is reinforced by VRF randomization:
  - Even with 30% of network controlled by attacker
  - Probability of 92 attackers in random 137-committee: negligible
  - No fork needed: security is built-in from day one
```

## Eclipse Attack Resistance

```
IPv4: max 1 peer per /24 subnet
IPv6: max 2 peers per /48 subnet

An attacker must control addresses from many different subnets.
```

## Sybil Resistance

- 137-second PoW for wallet registration (one-time cost per identity)
- IPv6 subnet diversity enforcement (max 1 peer per /24 IPv4, 2 per /48 IPv6)
- No trusted setup, no staking requirement
- **Genesis Hash Guard:** prevents malicious or accidental parallel chains
  (see §6 — Genesis Node Chain Authentication)

The PoW targets identity multiplication; the Genesis Hash Guard targets chain
multiplication. Together they close both attack vectors at the protocol level.

## Quantum Resistance: The 60-Second Window

Quantum computers pose a theoretical threat to ECDSA/Ed25519 signatures. Here is why Elektron Net remains secure without complex post-quantum cryptography:

### The Hash-Commitment Protection

```
UTXO stores:  BLAKE3(pubkey)[0:20]  ← only a hash, never the full key
Address is:   Bech32m("elek", BLAKE3(pubkey)[0:20])
```

As long as coins remain unspent, a quantum computer sees only a hash — it cannot reverse BLAKE3 to recover the private key. This is analogous to coins resting deep in your pocket: invisible to outside observers.

### The 60-Second Attack Window

When you spend coins, your public key is temporarily exposed in the broadcast transaction:

```
t = 0s:      Transaction broadcast — public key is now visible
t ≤ 60s:     Block producer includes transaction
t > 60s:     Block finalized — UTXO spent — quantum is too late
```

The window of vulnerability is exactly 60 seconds. For a quantum computer to successfully steal:

```
1. Intercept the transaction from the mempool
2. Compute private key from public key (Shor's algorithm)
3. Craft a contradictory transaction with higher fee
4. Propagate faster than the honest network
All within: 60 seconds
```

### The Realistic Threat Level

| Factor | Reality |
|--------|---------|
| Quantum computer exists today? | No — would need ~4,000 logical qubits, current systems have ~100-1,000 |
| Time until real threat | Realistically: 10-20+ years |
| 60-second window | Far too short for even future QC (error correction overhead) |
| Race against honest network | QC starts at a disadvantage — TX already propagating |

### Why No Zero-Knowledge Proofs

Zero-knowledge proofs would add quantum resistance but at a high cost:

- **Complexity:** New cryptographic constructions, setup ceremonies
- **Storage:** +200 bytes to +50KB per transaction
- **Speed:** Expensive verification, slower consensus
- **Simplicity:** Violates Elektron Net's design philosophy

Elektron Net uses a simpler approach: **the hash-commitment + 60-second rule**. As long as coins rest in the pocket (UTXO), they are quantum-safe. When spent, the window is too short for practical attack.

### Long-Term Migration

If a credible quantum threat emerges within the next decade, the network can hard-fork to post-quantum signatures (CRYSTALS-Dilithium, Falcon, or SPHINCS+). The 60-second protection window gives the network time to respond.

**Summary:**
- **Coins at rest:** Quantum-safe (only hash stored)
- **Coins in transit:** 60-second vulnerability window
- **After finalization:** Permanently safe, even against quantum
- **Long-term:** Hard-fork capability if needed

No ZKP. No ceremonies. No bloat. Just mathematics and the present moment.

## VRF Sort-Order Security

```
VRF Selection is not a vulnerability:

  The VRF process is unbiasable:
    - Same input (slot, checkpoint_hash, utxo_root) → same output
    - An attacker cannot manipulate their VRF output
    - Cannot predict or influence other nodes' outputs

  Knowing your committee status is not an advantage:
    - Attacker knows if they are in the committee or not
    - This provides no additional attack capability
    - Disruption attempts are the same as in any PoS/PoW system

  The 2/3 Byzantine threshold protects against disruption:
    - Even with 30% of Full Nodes not participating
    - The remaining 70% still provides strong finality
    - Honest majority is mathematically guaranteed
```

## Channel Security: Block Producer as Watchtower

```
Channel dispute window: 36 blocks (36 minutes)

Problem: If a channel partner goes offline, they cannot contest
a fraudulent old state broadcast.

Solution: Block Producer IS the Watchtower

  Every Block Producer validates all transactions in the block.
  This validation INCLUDES checking channel close transactions:
    1. Is this a channel close?
    2. Is it the latest known state?
    3. If not → Broadcast fraud proof immediately

  The 25% producer reward already covers this work:
    - Producer already validates ALL transactions
    - Watchtower is a natural side-effect of block production
    - No extra computation or storage required

  Fraud penalty:
    - If a node broadcasts an old channel state and gets caught:
    - They lose their security deposit (2x the disputed amount)
    - The Producer who caught the fraud earns a share of the penalty
```

## Known Limitations

### Network Partition
pBFT chooses safety over liveness. During a partition, no finality is reached. On reconnect, the network heals automatically:
- Higher checkpoint height wins
- If equal height: lower checkpoint hash wins (deterministic)
- No manual intervention required.

---

# Part III – Getting Started

# 12 – Running Elektron Net

## Requirements

```
Minimum:          2 CPU, 4 GB RAM, 10 GB storage
Full Node:        4+ CPU, 8 GB RAM, 50 GB SSD
Light/IoT Mode:  1 GB RAM, 1 GB storage
```

## Getting Started

```
1. Download the software
2. Read and confirm the Data Sovereignty Declaration
3. Choose "Create New Wallet"
4. Write down your 24-word seed phrase
5. Verify 5 random words when prompted
6. Wait ~137 seconds for wallet PoW (one-time only)
7. Your wallet is active. Rewards begin immediately.
```

## The Data Sovereignty Declaration

Before any wallet is created, every user confirms this declaration. This is not a terms-of-service agreement. This is the user's explicit assertion of their fundamental rights – a statement of intent that is architectural, not contractual.

```
┌─────────────────────────────────────────────────────────────────┐
│                    DATA SOVEREIGNTY DECLARATION                   │
│                    Elektron Net – First Start                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  "I install this software with the explicit intent that my        │
│   transaction data and network activity are never permanently     │
│   stored – anywhere, by anyone, at any time.                    │
│                                                                  │
│   I understand that Elektron Net enforces this through automatic  │
│   deletion after exactly 137 days (α⁻¹). This deletion is:      │
│                                                                  │
│   → automatic    (no action required from me)                   │
│   → unconditional (applies to all nodes, no exceptions)           │
│   → irreversible  (mathematically enforced by protocol)          │
│                                                                  │
│   I exercise my fundamental right to privacy and my right to     │
│   be forgotten – not as a request to a company, but as an        │
│   architectural property of the software I am voluntarily        │
│   installing.                                                     │
│                                                                  │
│   I understand that the protocol enforces 137-day pruning        │
│   technically. No node can retain data beyond 137 days           │
│   without modifying the software, which would place them on       │
│   a separate network.                                            │
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│  LEGAL REFERENCES – UNIVERSAL PRIVACY RIGHTS                      │
│                                                                  │
│  GDPR           | EU General Data Protection Regulation           │
│  UDHR Art. 12   | Universal Declaration of Human Rights         │
│  CCPA           | California Consumer Privacy Act                 │
│  LGPD           | Brazil General Data Protection Law              │
│  UK GDPR        | United Kingdom Data Protection Act              │
│  PIPEDA         | Canada Personal Information Protection Act      │
│  APPI           | Japan Act on Protection of Personal Information│
│  DPDP Act       | India Digital Personal Data Protection Act      │
│  nDSG           | Switzerland Federal Act on Data Protection       │
│  Privacy Act    | Australia Privacy Act 1988                     │
│                                                                  │
│  This declaration asserts rights under the laws of my              │
│  jurisdiction, regardless of where the network operates.        │
│                                                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  [ ] I have read and understood this declaration                 │
│                                                                  │
│                              [Continue →]                        │
└─────────────────────────────────────────────────────────────────┘
```

The checkbox cannot be pre-ticked. The Continue button is disabled until confirmed. There is no skip option.

## Roadmap

```
PHASE 1 – Foundation (Months 1-6)
  - elektrond daemon + CLI
  - eGUI wallet (send, receive, balance)
  - Genesis Mode fully operational (FN < 137)
  - Stealth addresses + Bloom-Filter scanning
  - Phase 1 consensus when N ≥ 137 (VRF + pBFT)
  - ElektronChannels with built-in Watchtower (Producer validates)

PHASE 2 – Full Features (Months 7-12)
  - Lepton Streams (micropayments)
  - Mobile wallet
  - Block explorer
  - Developer API

PHASE 3 – Ecosystem (Year 2)
  - Exchange integrations
  - Hardware wallet support
  - Light client improvements
```

---

# Part IV – The Philosophy

# 13 – Closing Words

## What We Have Built

Elektron Net is a decentralised digital currency with anonymous payments by default, a fair reward system for every participant, and a network that is fully functional from the first second with a single node.

## Mathematical Properties

```
Your wallet belongs to you     → Ed25519: theft mathematically infeasible
Your payments are private       → Stealth addresses: linking infeasible
Your balance is pseudonymous    → Addresses are hashes, amounts visible but unlinked
The supply is limited           → 21,000,000 Elek, anchored in Block #0
Your rewards are fair           → 25% producer / 75% online
Your payment is fast            → 60-second deterministic finality
Your history is forgotten        → 137-day pruning, protocol-enforced
The network starts alone        → Genesis Mode, one node is enough
The transition is automatic    → Phase 1 at N = 137
```

These are not promises. They are mathematical and architectural properties. They do not depend on anyone keeping their word.

## From the Lydian Elektron to the Digital Elek

```
2,600 years. The same principle. New tools.
Value through mathematics, not through decree.
```

Elektron Net is built on Stoic principles:

- **Freedom** – What is up to us (our keys, our choices)
- **Acceptance** – What is not up to us (market prices, adoption rates)
- **Responsibility** – Freedom without responsibility is noise

The mathematics secures your money. You decide how to use it.

---

# Appendix A – Glossary

**α⁻¹ (Alpha Inverse):** The fine-structure constant ≈ 1/137. Used throughout: 137s PoW, 137-day pruning, C_max = 137.

**BLAKE3:** Fastest modern cryptographic hash function. Used for addresses, block hashes, Merkle roots, PoW, checkpoint digests.

**BLS:** Aggregate signatures. Multiple signatures combined into one 96-byte value. Used for finality and checkpoints.

**Bech32m:** Address encoding with prefix "elek1". Error-detecting checksum.

**Ed25519:** Digital signature algorithm. Fast, secure, deterministic.

**Genesis Block:** First block in the chain. Its hash uniquely identifies the network.

**pBFT:** Practical Byzantine Fault Tolerance. Finality after ≥2/3 signatures.

**Stealth Address:** One-time address per payment. Cannot be linked.

**UTXO:** Unspent Transaction Output. Your current balance.

**VRF:** Verifiable Random Function. Unpredictable but verifiable randomness.

---

# Appendix B – Technical Parameters

## Consensus

| Parameter | Value |
|-----------|-------|
| Block Time | 60 seconds |
| Production Window | 45 seconds |
| Committee Size | 137 (fixed in Phase 1) |
| Byzantine Threshold | 2/3 = 92 votes |
| Phase 1 Start | FN ≥ 137 |
| Phase 1 End | FN < 137 → Genesis Mode |
| Dispute Window | 36 blocks |

## Economy

| Parameter | Value |
|-----------|-------|
| Max Supply | 21,000,000 Elek |
| Genesis Reward | 5 Elek |
| Halving | 4 years |
| Producer Share | 25% (instant, every block) |
| Online Pool | 75% (epoch end, every 10 min) |
| Epoch Length | 10 blocks (10 minutes) |
| Node Cap | 21,000 |
| Online Window | K = 10 slots |
| Min Transaction Fee | 100 cLep |

## Privacy

| Parameter | Value |
|-----------|-------|
| PoW Duration | 137 seconds |
| Pruning | 137 days |
| Address Format | Bech32m (elek1) |
| Key Derivation | BIP-32 m/137'/0'/N |

---

*This is the final form of the protocol. No promises. Only mathematics.*

**License: MIT**
