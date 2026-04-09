# Elektron Net – Protocol Workflows
**Version 2.2 | Technical Implementation Guide**

This document describes every workflow in the Elektron Net protocol as state machines and sequence diagrams.

---

## Table of Contents

1. [System Architecture](#1-system-architecture)
2. [Node Lifecycle](#2-node-lifecycle)
3. [Full Node Registry](#3-full-node-registry)
4. [Consensus Workflows](#4-consensus-workflows)
5. [Block Production](#5-block-production)
6. [Wallet Workflows](#6-wallet-workflows)
7. [Synchronization](#7-synchronization)
8. [Payment Channels](#8-payment-channels)
9. [Pruning](#9-pruning)
10. [Reward Distribution](#10-reward-distribution)
11. [Chain Security](#11-chain-security)
12. [Error Recovery](#12-error-recovery)

---

## 1. System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    E LE K T R O N   N E T                   │
├─────────────────────────────────────────────────────────────┤
│  Layer 3: Application (eGUI, CLI, RPC)                     │
├─────────────────────────────────────────────────────────────┤
│  Layer 2: Economy (Wallet, Channels, Privacy)              │
│           - BIP-32/39, Stealth, PoW                        │
│           - ElektronChannels, LeptonStreams                 │
├─────────────────────────────────────────────────────────────┤
│  Layer 1: Consensus (pBFT, VRF)                            │
│           - Full Nodes (137+ days online)                   │
│           - Genesis Mode (FN < 137)                        │
│           - Phase 1 (FN ≥ 137): VRF + pBFT, 137 committee  │
├─────────────────────────────────────────────────────────────┤
│  Layer 0: Network (P2P, Storage)                           │
│           - libp2p, sled/RocksDB                            │
│           - Bootstrap: PHP Registry + DNS Seeds             │
└─────────────────────────────────────────────────────────────┘
```

### Bootstrap Architecture

```
┌─────────────────────────────────────────────────────────────┐
│              BOOTSTRAP HIERARCHY (Priority)                 │
├─────────────────────────────────────────────────────────────┤
│  1. DNS Seeds (minimum 5, geographic distributed)           │
│  2. Bootstrap Server (PHP-based: www.elektron-net.org)      │
│  3. addr relay (peers share known addresses)                │
│  4. peer.db (saved peer list from previous sessions)        │
│  5. Hardcoded fallbacks (localhost for testing)             │
│  6. Genesis Mode (single node fully operational)            │
└─────────────────────────────────────────────────────────────┘
```

**Bootstrap Server (PHP Registry):**

| Endpoint | Methode | Beschreibung |
|----------|---------|--------------|
| `/bootstrap.php?action=list` | GET | Liste aller aktiven Nodes |
| `/bootstrap.php?action=register` | POST | Als Full Node registrieren |
| `/bootstrap.php?action=heartbeat` | POST | Heartbeat senden |
| `/bootstrap.php?action=stats` | GET | Registry Statistik |

**Node ID:** BLAKE3 hash of `pubkey:ip:port` (64 hex characters)
**IP Support:** IPv6 preferred, IPv4 fallback
**Heartbeat:** Every 15 minutes (timeout: 1 hour)
**Chain filter:** Every request includes `genesis_hash` — peers on a different chain are never returned.

See `DEV-README.md` for full Bootstrap Server API documentation.

---

### Genesis Node Hash Guard (Chain Authentication)

**Trigger:** All 6 bootstrap steps return zero reachable peers on `--network mainnet`.

When no peers are found, the daemon does **not** silently start as Genesis Node.
Instead it shows an interactive confirmation menu:

```
================================================================================
  No bootstrap peers or other peers found.
================================================================================

  Options:
    [1]  Retry peer discovery
    [2]  Start as Genesis Node  (requires mainnet genesis hash)
    [3]  Exit

  Your choice [1/2/3]:
```

#### Option [1] — Retry

Reruns the full 6-step bootstrap sequence from Step 1 (DNS seeds).
Use this if a network outage may have caused temporary peer unavailability.

#### Option [2] — Genesis Node Confirmation

```
  ┌──────────────────────────────────────────────────────────────────────────┐
  │  CAUTION: Starting as Genesis Node creates a new, separate chain.         │
  │  Only the authorized Elektron Net Genesis Node operator should do this.   │
  └──────────────────────────────────────────────────────────────────────────┘

  Enter the Elektron Net mainnet genesis hash (64 hex characters) to confirm:
  > ________________________________________________________________
```

The operator must type the exact Elektron Net mainnet genesis hash:

```
13703598e26d1536b62146c5ad1a64264d7f72167737a0d7662c5d1937b3e136
```

**Validation rules:**
- Exactly 64 hexadecimal characters required
- Compared byte-for-byte against the value hardcoded in `GenesisConfig::mainnet()`
  (`src/chain/chain.h`)
- Wrong input → menu shown again (no lockout, no limit — wrong input is always wrong)
- Correct input → `DiscoveryResult::GenesisConfirmed(Mainnet)` → Genesis Mode starts

#### Option [3] — Exit

Terminates the daemon. No chain state is created.

#### Network-mode behaviour summary

| `--network` | No peers found | Confirmation |
|-------------|----------------|--------------|
| `mainnet`   | Interactive menu | Hash entry required |
| `testnet`   | Auto-start with warning | None |
| `devnet`    | Auto-start silently | None |

#### Skipping the prompt (automated / scripted deployment)

Provide the genesis hash as a file via `--genesis`:

```bash
echo "13703598e26d1536b62146c5ad1a64264d7f72167737a0d7662c5d1937b3e136" \
  > /etc/elektron/genesis.hex

elektrond --genesis /etc/elektron/genesis.hex --network mainnet
```

This bypasses peer discovery entirely and loads genesis directly from the file.

#### Why this is not just about accidents

The Genesis Hash Guard also serves as **chain fork prevention**:

| Threat | Without Guard | With Guard |
|--------|--------------|------------|
| Misconfigured node | Silently starts new chain | Stopped at prompt |
| Network partition | All isolated nodes fork | All nodes retry or exit |
| Malicious operator | Easily starts "mainnet" fork | Must type exact hash |
| Automated attack | Script starts fork instantly | Script stops at interactive prompt |

The hash is public (in the source code). The protection is not secrecy —
it is **mandatory deliberate action**. No automation, no misconfiguration,
and no carelessness can pass this guard.

---

### Two-Tier Node Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    TWO-TIER ARCHITECTURE                     │
├─────────────────────────────────────────────────────────────┤
│  USER NODE (Tier 1)                                        │
│    - Runs wallet software                                  │
│    - May go offline at any time                            │
│    - Cannot participate in VRF committee                    │
│    - Cannot produce blocks (no 25% producer share)          │
│    - Receives epoch pool rewards (75% share) if online      │
│    - After 137 days online → eligible for Full Node        │
├─────────────────────────────────────────────────────────────┤
│  FULL NODE (Tier 2)                                        │
│    - 137+ days continuous online (verified via heartbeats) │
│    - Genesis Phase: ALL nodes are Full Nodes               │
│    - Forms VRF committee (137 randomly selected per slot)  │
│    - Signs blocks and checkpoints (BLS aggregate)           │
│    - Receives 25% producer share + epoch pool rewards     │
│    - Automatic promotion when FN < 137 (oldest candidates) │
└─────────────────────────────────────────────────────────────┘
```

### Technology Stack

```
CORE:
  Language:      C++20 (performant, zero-overhead abstractions, no GC)
  Async Runtime: Boost.Asio (async I/O, multi-threaded executor)
  Networking:    cpp-libp2p / Boost.Asio (P2P, encryption, NAT traversal)
  Database:      RocksDB (embedded, ACID-compliant, native C++ API)
  Build:         CMake

CRYPTOGRAPHY:
  Signatures:    Ed25519 (libsodium)
  Hashing:       BLAKE3 C library (address generation, PoW, stealth)
  VRF:           ristretto255 via libsodium (Curve25519 wrapper)
  BLS:           blst C library (aggregate signatures, finality)

WALLET:
  Mnemonics:     BIP-39 (24-word, 256-bit entropy, trezor-crypto)
  Derivation:    BIP-32 (hierarchical deterministic, libwally-core)
  Scanning:      Bloom-Filter (2^20 items, 0.1% FPR)

INTERFACES:
  RPC Port:      9337 (JSON-RPC 2.0)
  P2P Port:      9336 (multiaddr)
  GUI:           eGUI (Qt, wallet operations)
  CLI:           Subcommands via CLI11
```

---

## 2. Node Lifecycle

### State Machine

```
                    ┌──────────────┐
                    │    START     │
                    └──────┬───────┘
                           │
                           ▼
                    ┌──────────────┐
              ┌─────│  LOAD_CONFIG │
              │     └──────┬───────┘
              │            │
              ▼            ▼
       ┌──────────┐ ┌──────────┐
       │ NEW_NODE │ │ EXISTING │
       └────┬─────┘ └────┬─────┘
            │            │
            ▼            ▼
       ┌──────────┐ ┌──────────┐
       │  CREATE  │ │  LOAD    │
       │  WALLET  │ │  WALLET  │
       └────┬─────┘ └────┬─────┘
            │            │
            └─────┬──────┘
                  ▼
           ┌────────────┐
           │    BOOT    │
           └─────┬──────┘
                 │
                 ▼
           ┌────────────┐     No Peers
           │   DISCOVER │────────────────▶ GENESIS MODE SOLO
           │   PEERS    │
           └─────┬──────┘
                 │ Has Peers
                 ▼
           ┌────────────┐
           │    SYNC    │
           └─────┬──────┘
                 │
                 ▼
    ┌────────────────────────────┐
    │ CHECK_FULL_NODE_STATUS     │
    │                            │
    │ IF genesis_phase:          │
    │   → FULL_NODE (everyone)  │
    │                            │
    │ IF online_days ≥ 137:      │
    │   → FULL_NODE (promoted)   │
    │                            │
    │ IF candidate exists:        │
    │   → Apply to oldest FN     │
    │                            │
    │ ELSE:                      │
    │   → USER_NODE             │
    └─────┬────────────────────┘
          │
          ▼
    ┌───────────┐
    │  ACTIVE   │◀────────────────┐
    └───────────┘                  │
          │                         │
    ┌─────┴─────┐                   │
    ▼           ▼                   │
┌─────────┐ ┌─────────┐             │
│PRODUCE  │ │VALIDATE │             │ (if USER_NODE)
│BLOCKS   │ │BLOCKS   │             │ Shutdown
└─────────┘ └─────────┘             │
         (FULL_NODE only)           │
                                    │
    Shutdown ◄───────────────────────┘
```

### Entry Points

| Trigger | Action | Next State |
|---------|--------|------------|
| `elektrond start` | Load config | BOOT |
| Config missing | Create default | BOOT |
| Wallet missing | Start wallet creation | CREATE_WALLET |
| No peers | Enter Genesis Mode | GENESIS_MODE |
| Peers found | Sync chain | SYNC |
| FN ≥ 137 | Full Node eligible | FULL_NODE |
| FN < 137 + candidate | Apply for promotion | PENDING |
| Online < 137 days | User Node | USER_NODE |

### Full Node vs User Node

```
┌──────────────────────────────────────────────────────────────┐
│                    NODE TYPE COMPARISON                       │
├──────────────────────────────────────────────────────────────┤
│  PROPERTY         │ USER NODE    │ FULL NODE                │
├───────────────────┼───────────────┼──────────────────────────┤
│  VRF Committee    │ No           │ Yes (137 per slot)      │
│  Block Production │ No           │ Yes (P1/P2/P3)          │
│  BLS Signing      │ No           │ Yes                     │
│  Heartbeats       │ Optional     │ Required when online     │
│  Rewards          │ None         │ 25% + pool share        │
│  Can go offline   │ Yes          │ Yes (status stays FN)   │
│  Checkpoint Sig   │ No           │ Yes                     │
│  Channel Watchtower│ No          │ Yes                     │
│  FN Enable Button │ No           │ Yes (voluntary)         │
└───────────────────┴───────────────┴──────────────────────────┘
```

---

## 3. Full Node Registry

### 3.1 On-Chain Registry

Every Full Node is registered on-chain. The registry is a Merkle tree root stored in each block header.

```
┌─────────────────────────────────────────────────────────────┐
│  FULL NODE REGISTRY                                         │
├─────────────────────────────────────────────────────────────┤
│  pubkey_1 ──────┐                                          │
│  pubkey_2 ──────┼──► Merkle Root ───► Block Header         │
│  ...            │                                          │
│  pubkey_N ──────┘                                          │
│                                                             │
│  Each FN entry contains:                                    │
│    - pubkey (32 bytes)                                      │
│    - first_heartbeat_slot (u64)                             │
│    - uptime_confirmed_blocks (u32)                          │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 Full Node Qualification

```
┌─────────────────────────────────────────────────────────────┐
│  QUALIFICATION REQUIREMENTS                                 │
├─────────────────────────────────────────────────────────────┤
│  REQUIREMENT             │ DESCRIPTION                     │
│  ─────────────────────────────────────────────────────────  │
│  Heartbeats              │ Must send heartbeats every slot │
│  Uptime                  │ ≥137 consecutive days online    │
│  Registration            │ On-chain registry entry         │
│  Genesis Phase           │ All nodes are Full Nodes        │
└─────────────────────────────────────────────────────────────┘
```

**Heartbeat Verification:**
```
For each FN in registry:
  1. Count heartbeats in last 137 days (197,280 slots)
  2. This shows online time accumulated (for qualification)
  3. Full Node status is NEVER revoked for low uptime
```

### 3.3 Full Node Promotion (N < 137)

When Full Node count falls below 137, the network promotes candidates.

```
┌─────────────────────────────────────────────────────────────┐
│  PROMOTION WORKFLOW                                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  STEP 1: DETECT SHORTAGE                                    │
│    IF fn_count < 137:                                       │
│      → Generate candidate list (users with >0 heartbeats)    │
│      → Sort by oldest first_heartbeat_slot                  │
│                                                              │
│  STEP 2: CANDIDATE APPLICATION                              │
│    Candidate Node:                                          │
│      → Builds application: {pubkey, first_heartbeat_slot}   │
│      → Signs with spend_privkey                             │
│      → Sends to oldest FN(s)                               │
│                                                              │
│  STEP 3: FULL NODE APPROVAL                                 │
│    Oldest FN receives application:                         │
│      → Verifies candidate has sent heartbeats               │
│      → Signs approval: {pubkey, fn_pubkey, timestamp, sig} │
│      → Broadcasts approval transaction                      │
│                                                              │
│  STEP 4: REGISTRY UPDATE                                    │
│    When ≥50% of current FNs have approved:                 │
│      → Candidate is promoted to FULL_NODE                  │
│      → Added to on-chain registry                          │
│      → Eligible for VRF committee                          │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

### 3.4 Promotion Ties (Same Age)

```
When multiple candidates have same first_heartbeat_slot:
  1. Sort candidates by pubkey (lexicographic, deterministic)
  2. Oldest pubkey wins (lower = older in sort)
  3. Only ONE candidate promoted per round
  4. Others wait for next promotion round
```

### 3.5 Full Node Status (Voluntary & Persistent with Dormant State)

Full Node Status is VOLUNTARY and PERSISTENT. A "Dormant" state handles long offline periods without punishment.

```
┌─────────────────────────────────────────────────────────────┐
│  FULL NODE STATUS RULES                                    │
├─────────────────────────────────────────────────────────────┤
│  ACTIVE (Full Node enabled):                               │
│    - Counts toward committee quorum                         │
│    - Receives rewards when selected                        │
│    - User can manually switch to User Node                 │
│                                                              │
│  DORMANT (after 137 days without heartbeat):               │
│    - Does NOT count toward committee quorum                 │
│    - Does NOT receive rewards                              │
│    - Registry entry remains (never auto-removed)           │
│    - Reactivation: 1 heartbeat → immediately ACTIVE        │
│    - NO penalty, NO new 137-day wait                      │
│                                                              │
│  USER NODE (never was FN or manually switched):            │
│    - Full Node privileges disabled                         │
│    - Can accumulate heartbeats to qualify                  │
│    - Must meet 137-day minimum to re-enable FN            │
└─────────────────────────────────────────────────────────────┘
```

**Why This Design:**
- Short offline periods (hardware defect, vacation): NO penalty, instant recovery
- Network stability maintained: Dormant FNs don't count toward quorum
- Registry stays clean: Dead nodes removed after 1 year inactivity
- Voluntary + Persistent + Dormant = best of all worlds
```

**Genesis Phase Exception:**
- During Genesis Phase (FN < 137), ALL nodes are Full Nodes
- When FN count drops below 137: oldest User Nodes are promoted (not demoted)
- This ensures the committee always has 137 members
```

### 3.6 Full Node Enable Button (Voluntary Activation)

User clicks "Enable Full Node Mode" in GUI. FN status is VOLUNTARY and PERSISTENT.

```
┌─────────────────────────────────────────────────────────────────┐
│  FULL NODE ENABLE WORKFLOW                                      │
│                                                                 │
│  User clicks "Enable Full Node Mode"                            │
└─────────────────────────────────────────────────────────────────┘

STEP 1: UI CONFIRMATION
    Display: "Full Node Mode requires 137+ days of online time"
    Display: "Your status will be ACTIVE immediately when enabled"
    Display: "You can switch back to User Node anytime"
    User confirms → proceed

STEP 2: REGISTRY ENTRY CREATION
    Compute: FnRecord fn_record {
        .pubkey             = wallet_pubkey,
        .status             = FnStatus::ACTIVE,
        .first_seen         = current_slot,
        .heartbeat_cnt      = 0,
        .registration_block = current_height
    };
    Broadcast: REGISTRATION_MESSAGE (signed)
    Producer includes in next block
    On-chain registry updated

STEP 3: ACTIVE IMMEDIATELY
    After inclusion in block:
    - Status = ACTIVE (counts toward quorum)
    - Eligible for VRF committee selection
    - Eligible for block production
    - Receives 25% producer share if selected
    - Receives 75% pool rewards if in online window
    Heartbeat counter begins accumulating

STEP 4: 137-DAY QUALIFICATION TIMER
    Heartbeats tracked per slot:
        heartbeat_cnt++
    After 137 days (197,280 blocks):
        UPTIME_THRESHOLD_MET = true
        Full qualification complete
        Status remains ACTIVE (no change in behavior)

STEP 5: DISABLE FN (User Node)
    User clicks "Switch to User Node"
    Status: → DORMANT (after 137 days without heartbeat)
    Registry entry: REMAINS (never deleted)
    Re-enable: 1 heartbeat → ACTIVE (no new 137-day wait)
```

```
FN ENABLE BUTTON – UI FLOW:

    ┌──────────────────┐
    │ Wallet Dashboard │
    └────────┬─────────┘
             │ "Enable Full Node"
             ▼
    ┌──────────────────────────┐
    │ Confirmation Dialog      │
    │ - 137 days online req.   │
    │ - Voluntary + persistent │
    │ - Can switch back anytime│
    └──────────┬───────────────┘
               │ User confirms
               ▼
    ┌──────────────────────────┐
    │ Broadcast Registration   │
    │ (signed, on-chain)       │
    └──────────┬───────────────┘
               │ Included in block
               ▼
    ┌──────────────────────────┐
    │ FN Status: ACTIVE        │
    │ - Counts for quorum       │
    │ - Earns all FN rewards   │
    │ - Heartbeat timer starts  │
    └──────────────────────────┘
```

```
SWITCHING BACK TO USER NODE:

    User clicks "Switch to User Node"
    → Heartbeats stop counting for FN registry
    → After 137 days offline: DORMANT
    → DORMANT: does NOT count toward quorum
    → DORMANT: does NOT receive rewards
    → Registry entry: REMAINS

    REACTIVATION:
    → 1 heartbeat → immediately ACTIVE
    → NO new 137-day wait
    → Full privileges restored instantly
```

### 3.7 Genesis Phase

During Genesis Phase (fn_count < 137 initially), ALL nodes are Full Nodes.

```
┌─────────────────────────────────────────────────────────────┐
│  GENESIS PHASE RULES                                        │
├─────────────────────────────────────────────────────────────┤
│  1. Every wallet is automatically a Full Node              │
│  2. All participate in VRF committee                        │
│  3. All can produce blocks                                  │
│  4. Checkpoint creation:                                    │
│     - BLS signatures from all current FNs                  │
│     - Threshold: 2/3 of present FNs (not 92/137)            │
│  5. Phase ends when fn_count ≥ 137                          │
│     → Automatic transition to Phase 1                       │
│  6. If fn_count later drops below 137:                    │
│     → Oldest User Nodes promoted (voluntary)               │
└─────────────────────────────────────────────────────────────┘
```

### 3.8 Viewing Full Node Registry

```
CLI Command:
  elektrond fn-list

Output:
  Full Nodes: 247 (210 active, 37 dormant)
  Oldest:     2 years, 3 months
  Newest:     0 days (recently enabled)

  Top 5 oldest FNs:
    [1] fn1...abc  - 2y 3m - ACTIVE
    [2] fn2...def  - 1y 11m - ACTIVE
    [3] fn3...ghi  - 1y 8m - DORMANT (offline 200 days)
    [4] fn4...jkl  - 1y 5m - ACTIVE
    [5] fn5...mno  - 1y 2m - ACTIVE
```

---

## 4. Consensus Workflows

### 4.1 Genesis Mode (FN < 137)

**Entry condition:** Either a peer was found and synced, or the Genesis Hash Guard
was passed (operator typed the mainnet genesis hash — see §1 Bootstrap Architecture).

**Trigger:** `full_nodes.len() < 137`

```
┌─────────────┐
│ GENESIS    │
│   MODE     │
└──────┬─────┘
       │
       ▼
┌─────────────────────────────┐
│ SELECT_PRODUCER             │
│                             │
│ IF N == 1:                 │
│   producer = self           │
│                             │
│ IF N == 2:                  │
│   producer = alternate(a,b) │
│                             │
│ IF N > 2:                   │
│   last = last_producer      │
│   candidates = all - {last} │
│   producer = random(candidates) │
└──────┬──────────────────────┘
       │
       ▼
┌─────────────┐
│  PRODUCE    │
│  (Ed25519)  │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  BROADCAST  │
└──────┬──────┘
       │
       ▼
┌─────────────┐    N ≥ 137?
│  AWAIT_NEXT ├──────────┐
└──────┬──────┘          │
       │                  ▼
       │           ┌─────────────┐
       │           │  TRANSITION │
       │           │  TO_PHASE1  │
       │           └─────────────┘
       └──────▶ LOOP
```

**Parameters:**
- No VRF required
- No BLS signatures
- All nodes are Full Nodes (Genesis Phase)
- 25% producer bonus + 75% to all online (Genesis shares the Phase 1 model)

### 4.2 Phase 1 (FN ≥ 137)

**Trigger:** `full_nodes.len() >= 137`

```
SLOT_START (every 60s)
    │
    ▼
┌──────────────────────────┐
│ VRF_COMMITTEE_SELECTION  │
│                          │
│ For each FULL NODE:      │
│   vrf_input = hash(      │
│     slot_number ||        │
│     last_checkpoint_hash   │
│   )                      │
│   vrf_proof = prove(     │
│     private_key,          │
│     vrf_input            │
│   )                      │
│                          │
│ Sort by vrf_output       │
│ committee = top 137 nodes (fixed size) │
└──────┬───────────────────┘
       │
       ▼
┌──────────────────────────┐
│ ASSIGN_PRODUCERS         │
│                          │
│ P1 = committee[0]        │
│ P2 = committee[1]        │
│ P3 = committee[2]        │
│                          │
│ T = ceil(137 × 2/3) = 92 │
└──────┬───────────────────┘
       │
       ▼
┌──────────────────────────────────────────────────────┐
│               PRODUCTION_TIMELINE                     │
│                                                       │
│ t=0s:  P1 produces and broadcasts                    │
│ t=15s: If P1 failed → P2 prepares                  │
│ t=30s: If P2 ready → P2 broadcasts                 │
│ t=40s: If P1,P2 failed → P3 broadcasts              │
│ t=60s: If T votes collected → finalise               │
│        Else → skip slot, trigger pacemaker           │
└──────┬───────────────────────────────────────────────┘
       │
       ▼
┌─────────────┐
│ DISTRIBUTE  │
│ REWARDS     │
└──────┬──────┘
       │
       ▼
┌─────────────┐
│ NEXT_SLOT   │
└─────────────┘
```

### 4.3 Mode Transitions

**Genesis Phase (FN < 137):**
- ALL nodes are Full Nodes (automatic)
- No BLS checkpoint signatures required yet
- All participate in block production

**Phase 1 (FN ≥ 137):**
- Only Full Nodes in VRF committee
- 2/3 threshold (92/137) for finality
- Full checkpoint system active

**Promotion (FN falls below 137):**
- Oldest User Nodes promoted automatically
- No disruption to existing committee
- No transition back to Genesis Phase

### 4.4 VRF Input Randomness

To prevent long-term predictability of committee selection:

```
vrf_input = hash(
  slot_number ||
  last_checkpoint_hash ||
  utxo_root_at_checkpoint
)
```

**Why this matters:**
- `last_checkpoint_hash` only changes every 137 days
- `utxo_root_at_checkpoint` changes with every transaction
- Combining both: VRF output is unpredictable even over 137-day windows

### 4.4b VRF Proof Computation (Per-Node)

Each node computes its VRF proof independently using ristretto255 (Curve25519 wrapper).

```
┌─────────────────────────────────────────────────────────────────┐
│  VRF PROOF COMPUTATION (Every Slot)                            │
│                                                                 │
│  Each full node runs this locally to determine committee rank   │
└─────────────────────────────────────────────────────────────────┘

STEP 1: CONSTRUCT VRF INPUT
    vrf_input = hash(
        slot_number,         // u64: current_slot
        last_checkpoint_hash, // [u8; 32]: from latest checkpoint
        utxo_root           // [u8; 32]: current UTXO merkle root
    )
    // All three values are publicly known to all nodes
    // Input is IDENTICAL for all nodes in the same slot

STEP 2: COMPUTE VRF PROOF
    // Using ristretto255 (Curve25519 VRF implementation)
    proof = vrf_prove(node_private_key, vrf_input)

    // Internally:
    //   alpha = hash(vrf_input || private_key)  // hashed to scalar
    //   beta = alpha × G                         // point on curve
    //   gamma = hash(alpha || beta || vrf_input) // Fiat-Shamir
    //   r = alpha - gamma × private_key          // response scalar
    //   proof = (beta, r)                        // 32 + 32 = 64 bytes

    // Output: proof = vrf_prove(sk, input)
    //   Returns: (vrf_hash, vrf_proof)
    //   vrf_hash = BLAKE3(beta)  // 32 bytes output
    //   vrf_proof = (beta, r)    // 64 bytes, for verification

STEP 3: VERIFY OWN PROOF (local)
    is_valid = vrf_verify(node_public_key, vrf_input, vrf_proof)
    // Any node can verify any other node's proof

STEP 4: DETERMINE COMMITTEE RANK
    // Collect ALL VRF outputs from ALL full nodes
    // Sort all full nodes by vrf_hash (lowest = best)
    // Top 137 nodes = committee for this slot

    // Example:
    //   Node_A vrf_hash = 0x00a1... → rank 1 (in committee)
    //   Node_B vrf_hash = 0x00f2... → rank 5 (in committee)
    //   Node_C vrf_hash = 0x0123... → rank 200 (not in committee)
    //   ...
```

```
VRF IMPLEMENTATION (using libsodium ristretto255):

    // C++20 code (using libsodium + BLAKE3 C library)
    #include <sodium.h>
    #include <blake3.h>
    #include <array>
    #include <cstring>

    // Returns {vrf_hash[32], vrf_proof[64]}
    std::pair<std::array<uint8_t,32>, std::array<uint8_t,64>>
    vrf_prove(const uint8_t private_key[32],
              const uint8_t* input, size_t input_len)
    {
        // Step 1: Hash input + privkey to scalar via BLAKE3
        uint8_t alpha_buf[64];
        blake3_hasher h;
        blake3_hasher_init(&h);
        blake3_hasher_update(&h, input, input_len);
        blake3_hasher_update(&h, private_key, 32);
        blake3_hasher_finalize(&h, alpha_buf, 64);
        uint8_t alpha[32];
        crypto_core_ristretto255_scalar_reduce(alpha, alpha_buf);

        // Step 2: beta = alpha × G  (ristretto255 base-point multiply)
        uint8_t beta[crypto_core_ristretto255_BYTES];
        crypto_scalarmult_ristretto255_base(beta, alpha);

        // Step 3: Fiat-Shamir challenge gamma = BLAKE3(alpha || beta || input)
        uint8_t gamma_buf[64];
        blake3_hasher_init(&h);
        blake3_hasher_update(&h, alpha, 32);
        blake3_hasher_update(&h, beta, sizeof beta);
        blake3_hasher_update(&h, input, input_len);
        blake3_hasher_finalize(&h, gamma_buf, 64);
        uint8_t gamma[32];
        crypto_core_ristretto255_scalar_reduce(gamma, gamma_buf);

        // Step 4: r = alpha - gamma × private_key  (scalar arithmetic)
        uint8_t gamma_sk[32];
        crypto_core_ristretto255_scalar_mul(gamma_sk, gamma, private_key);
        uint8_t r[32];
        crypto_core_ristretto255_scalar_sub(r, alpha, gamma_sk);

        // Output vrf_hash = BLAKE3(beta)
        std::array<uint8_t,32> vrf_hash;
        blake3_hasher_init(&h);
        blake3_hasher_update(&h, beta, sizeof beta);
        blake3_hasher_finalize(&h, vrf_hash.data(), 32);

        // proof = beta[32] || r[32]  (64 bytes total)
        std::array<uint8_t,64> proof;
        std::memcpy(proof.data(),      beta, 32);
        std::memcpy(proof.data() + 32, r,    32);

        return {vrf_hash, proof};
    }
```

```
VRF VERIFICATION (by any node):

    bool vrf_verify(const uint8_t public_key[32],
                    const uint8_t* input, size_t input_len,
                    const uint8_t proof[64])
    {
        const uint8_t* beta = proof;       // bytes [0..32]
        const uint8_t* r    = proof + 32;  // bytes [32..64]

        // Re-compute challenge gamma = BLAKE3(r || beta || input)
        uint8_t gamma_buf[64];
        blake3_hasher h;
        blake3_hasher_init(&h);
        blake3_hasher_update(&h, r, 32);
        blake3_hasher_update(&h, beta, 32);
        blake3_hasher_update(&h, input, input_len);
        blake3_hasher_finalize(&h, gamma_buf, 64);
        uint8_t gamma[32];
        crypto_core_ristretto255_scalar_reduce(gamma, gamma_buf);

        // Check: r × G + gamma × public_key == beta
        uint8_t rG[crypto_core_ristretto255_BYTES];
        uint8_t gP[crypto_core_ristretto255_BYTES];
        uint8_t check[crypto_core_ristretto255_BYTES];
        crypto_scalarmult_ristretto255_base(rG, r);
        crypto_scalarmult_ristretto255(gP, gamma, public_key);
        crypto_core_ristretto255_add(check, rG, gP);

        return crypto_verify_32(check, beta) == 0;
    }
```

### 4.5 Pacemaker (Skip Slot Recovery)

When slots are skipped (no block finalized within 60s), the pacemaker handles recovery.

```
┌─────────────────────────────────────────────────────────────────┐
│  PACEMAKER STATE MACHINE                                        │
│                                                                 │
│  Normal operation: slot advances every 60 seconds               │
│  Skipped slot: triggers pacemaker to re-draw committee         │
└─────────────────────────────────────────────────────────────────┘
```

```
PACEMAKER LOGIC:

    Every 60 seconds:
        IF block_finalized_for_slot(current_slot):
            → Normal: advance to next slot
        ELSE:
            → PACEMAKER_ACTIVATED
            k = consecutive_skipped_slots
            k += 1
            timeout = 60s × 2^min(k, 10)  // exponential backoff

            Re-draw VRF committee for skipped slot
            Committee members: recompute vrf_output
            New P1/P2/P3 assigned

    COMMITTEE RE-DRAW:
        Same vrf_input (slot_number, checkpoint_hash, utxo_root)
        BUT: Offline nodes unlikely to be re-selected
        VRF is independent per slot (random each time)

    BACKOFF TABLE:
        k=1:  timeout = 120s  (2 × 60s)
        k=2:  timeout = 240s  (4 × 60s)
        k=3:  timeout = 480s  (8 × 60s)
        k=10: timeout = 61440s (1024 × 60s) ← MAX

    EXIT CONDITION:
        When block finalized: k = 0, timeout = 60s (normal)
```

```
PACEMAKER INTEGRATION WITH pBFT:

    Slot window = [slot_start, slot_start + 45s]  (valid production)
    t=0s:   P1 produces
    t=15s:  If P1 failed → P2 prepares
    t=30s:  If P2 ready → P2 broadcasts
    t=40s:  If P1,P2 failed → P3 broadcasts
    t=60s:  If no finality (T < 92 votes):
        → Slot SKIPPED
        → PACEMAKER increments k
        → New committee drawn for next slot
        → No punishment for offline producers

    SAFETY:
        - pBFT guarantees: no two conflicting blocks finalize
        - Pacemaker ensures: network eventually produces blocks
        - Skipped slots do NOT cause forks
```

```
IMPLEMENTATION NOTES:

    struct PacemakerState {
        size_t   k;                       // consecutive skipped slots
        uint64_t last_finalized_height;
        uint64_t current_slot;
        std::chrono::steady_clock::time_point timeout_until;
    };

    // Every SLOT_TICK (every 60s):
    void on_slot_tick(PacemakerState& state) {
        if (block_exists_for_slot(state.current_slot)) {
            state.k = 0;  // reset
            advance_to_next_slot();
        } else {
            state.k += 1;
            auto secs = 60 * (1u << std::min(state.k, size_t(10)));
            re_draw_committee(state.current_slot);
        }
    }

    VRF COMMITTEE IS ALWAYS FRESH:
        - Each slot: new independent VRF draw
        - Cannot predict next committee from current
        - Even if k=10, each re-draw is truly random
```

## 5. Block Production

### 5.1 Block Construction

```
┌──────────────┐
│ BUILD_BLOCK  │
└──────┬──────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ 1. SELECT_TRANSACTIONS                   │
│    - From mempool                        │
│    - Priority by fee                     │
│    - Max block size                      │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ 2. CREATE_HEADER                         │
│    version:     u32 = 1                 │
│    prev_hash:   [u8; 32]                │
│    merkle_root: calculate_merkle(txs)    │
│    timestamp:   unix_time()             │
│    slot:        current_slot             │
│    producer:    self.pubkey              │
│    vrf_proof:   Some(vrf) [Phase 1]     │
│                  None [Genesis Mode]      │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ 3. SIGN_BLOCK                            │
│    sig = Ed25519_Sign(                  │
│      producer_privkey,                   │
│      header.hash()                      │
│    )                                    │
└──────┬──────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────┐
│ 4. BROADCAST                             │
│    - Send to committee [Phase 1]         │
│    - Send to connected peers             │
└─────────────────────────────────────────┘
```

### 5.2 Block Validation

```
┌──────────────┐
│RECEIVE_BLOCK │
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ VALIDATION_CHECKS                        │
│                                          │
│ □ Header size == 80 bytes                │
│ □ Version == 1                           │
│ □ Prev_hash exists                       │
│ □ Timestamp in [slot_start, slot_start+45] │
│ □ Slot == current_slot                   │
│ □ Producer in committee [Phase 1]        │
│ □ VRF proof valid [Phase 1]             │
│ □ Merkle root matches transactions       │
│ □ All transactions valid                 │
│ □ Producer signature valid               │
└──────┬──────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ Phase 1: Vote with BLS signature         │
│ Genesis: Accept if all checks pass       │
└──────────────────────────────────────────┘
```

---

## 6. Wallet Workflows

### 6.1 Wallet Creation

```
USER: "Create New Wallet"
    │
    ▼
┌──────────────────────────────────────────┐
│ 1. GENERATE_ENTROPY                      │
│    entropy = CSPRNG(32 bytes)            │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 2. BIP39_ENCODE                          │
│    mnemonic = 24-word representation      │
│    Display: "Write down these words"      │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 3. VERIFY_MNEMONIC                       │
│    Prompt 5 random positions             │
│    Retry if wrong                        │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 4. DERIVE_KEYS                           │
│    seed = bip39_to_seed(mnemonic)       │
│    spend_priv = hd_derive(seed, "m/137'/0'/0'") │
│    scan_priv  = hd_derive(seed, "m/137'/0'/1'") │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 5. PROOF_OF_WORK (~137 seconds)          │
│    target = calibrate_difficulty()        │
│    nonce = 0                             │
│    LOOP:                                  │
│      h = BLAKE3(pubkey || nonce)         │
│      if h < target: break                 │
│    Display progress bar                   │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 6. CREATE_ADDRESS                        │
│    hash160 = BLAKE3(spend_pub)[0..20]    │
│    address = Bech32m("elek1", hash160)   │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 7. SAVE_WALLET                           │
│    Encrypt with password                  │
│    Store: ~/.elektron/wallet/            │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 8. BROADCAST_REGISTRATION                │
│    { pubkey, pow_nonce, timestamp, sig } │
│    TTL: 1440 blocks (24 hours)           │
│    If not included: re-broadcast          │
└──────────────────────────────────────────┘
```

### 6.2 Stealth Address Generation & Detection

```
SENDER has: (scan_pub, spend_pub)
RECIPIENT has: (scan_priv, spend_priv)

SENDER:
    r = random_scalar()
    R = r × G
    shared = BLAKE3(r × scan_pub)
    one_time = shared × G + spend_pub

    Send to one_time address
    Include R in transaction

RECIPIENT (detection using Bloom-Filter):

    STEP 1 – Generate Bloom-Filter (at wallet creation or recovery):
        filter_items = []
        For N = 0 to 2^20 (1,048,576 iterations):
            r_N = derive_ephemeral_scalar(seed, N)
            R_N = r_N × G
            candidate = BLAKE3(scan_priv × R_N) × G + spend_pub
            filter_items.push(candidate)
        bloom_filter = bloom::new(filter_items, 0.001)

    STEP 2 – Scan UTXO set:
        For each UTXO output address:
            IF bloom_filter.might_contain(address):
                → Add to candidates list

    STEP 3 – Verify candidates:
        For each candidate:
            r = derive_ephemeral_scalar(tx_ephemeral_R)
            candidate = BLAKE3(scan_priv × R) × G + spend_pub
            IF candidate == output_address:
                → CONFIRMED: This payment is mine
                spend_key = BLAKE3(scan_priv × R) + spend_priv

    BLOOM-FILTER SCALABILITY:
        Filter contains THIS WALLET's addresses only (~2.4 MB for 2^20 items).
        At 100M UTXOs: ~100,000 false positives to verify (acceptable).
        Filter regeneratable from seed at any time.
```

### 6.3 Wallet Recovery (Without History)

Recovery requires only the 24-word seed. No transaction history, no server, no trusted party.

```
┌─────────────────────────────────────────────────────────────────┐
│  WALLET RECOVERY WORKFLOW                                       │
│                                                                 │
│  Goal: Derive all spendable UTXOs from seed alone              │
└─────────────────────────────────────────────────────────────────┘

STEP 1: ENTER SEED
    User enters 24-word BIP-39 mnemonic
    Derive: scan_privkey, spend_privkey, spend_pubkey
    Path: m/137'/0'/0 (spend) and m/137'/0'/1 (scan)

STEP 2: GENERATE BLOOM-FILTER (100% offline, no network)
    For N = 0 to 2^20:
        r_N = derive_ephemeral_scalar(seed, N)
        R_N = r_N × G
        candidate = BLAKE3(scan_privkey × R_N) × G + spend_pubkey
        bloom_filter.add(candidate)
    Result: ~2.4 MB Bloom-Filter (false positive rate = 0.1%)

STEP 3: DOWNLOAD CURRENT UTXO SET (read-only, no data sent)
    Connect to 3+ peers simultaneously
    Request: GET_UTXO_SET (full UTXO snapshot)
    Store locally for scanning
    No private information leaves the device

STEP 4: SCAN UTXO SET WITH BLOOM-FILTER
    For each UTXO output address:
        IF bloom_filter.might_contain(address):
            → Add to candidates list (possible match)
        ELSE:
            → Definitely NOT mine (no false negatives)

STEP 5: VERIFY CANDIDATES (full stealth derivation)
    For each candidate:
        r = derive_ephemeral_scalar(tx_ephemeral_R)
        derived = BLAKE3(scan_privkey × R) × G + spend_pubkey
        IF derived == output_address:
            → TRUE MATCH: This UTXO belongs to this wallet
            Derive spend_key = BLAKE3(scan_privkey × R) + spend_privkey
        ELSE:
            → FALSE POSITIVE: Discard

STEP 6: RECONSTRUCT WALLET BALANCE
    Sum all TRUE MATCH UTXOs
    Wallet is fully recovered
    All funds accessible
    No transaction history needed
```

```
PERFORMANCE:
    Bloom-Filter generation: ~30 seconds (one-time)
    UTXO Set download: ~1-5 minutes (from 3 peers, ~500 MB)
    Scan with Bloom-Filter: ~100ms (10M UTXOs, O(n))
    Candidate verification: ~10,000 false positives at 0.1% FPR
    Total recovery time: ~5-10 minutes on first run
```

```
SECURITY PROPERTIES:
    - Bloom-Filter reveals NOTHING to network (generated locally)
    - UTXO download is READ-ONLY (no writes, no authentication)
    - All verification is LOCAL (private keys never leave device)
    - No trusted server or oracle required
    - Works after 137+ days offline (history is gone from network)
    - Seed is the ONLY required secret
```

```
POST-RECOVERY:
    After recovery, wallet resumes normal operation:
    - Can spend funds immediately
    - Earns new rewards if node is online
    - Stealth address generation resumes as normal
    - No re-registration needed (wallet already on-chain)
```

### 6.4 Light/IoT Mode

Light mode is for devices with limited storage and RAM. Full nodes handle all consensus and storage.

```
┌─────────────────────────────────────────────────────────────────┐
│  LIGHT/IoT MODE: THIN CLIENT                                   │
│                                                                 │
│  Role: Wallet + Limited Validation (no full chain)             │
│  Storage: UTXO set + headers only (~1 GB)                      │
│  RAM: ~1 GB                                                    │
│  Consensus: None (relies on full nodes for block validation)    │
└─────────────────────────────────────────────────────────────────┘
```

```
WHAT LIGHT NODES STORE:
    - UTXO set (current balances, ~1 GB)
    - Block headers (80 bytes × all blocks, ~24 MB/year)
    - Own wallet keys and transactions
    - Peer addresses (minimal)

WHAT LIGHT NODES DO NOT STORE:
    - Full blocks (transactions deleted after 137 days)
    - Historical blocks
    - Full blockchain history
```

```
LIGHT NODE WORKFLOW:

    BOOT:
        1. Download block headers chain (all headers, ~24 MB/year)
        2. Verify header chain (PoW valid, difficulty correct)
        3. Download current UTXO set from 3+ full nodes
        4. Verify UTXO root against latest checkpoint header
        5. Wallet is operational

    TRANSACTION CREATION:
        1. Wallet creates transaction locally
        2. Sends to connected full nodes
        3. Full nodes broadcast and include in blocks
        4. Wallet monitors via Bloom-Filter for incoming payments

    RECEIVING PAYMENTS:
        1. Generate stealth address
        2. Scan UTXO set with Bloom-Filter (same as wallet recovery)
        3. Full nodes do NOT know which addresses belong to light node

    SENDING PAYMENTS:
        1. Build transaction
        2. Sign with Ed25519
        3. Broadcast to connected full nodes
        4. Transaction enters mempool
        5. Full nodes include in next block
```

```
IoT CONSTRAINTS (Raspberry Pi, embedded devices):
    - 1 GB RAM minimum
    - 1 GB storage minimum (UTXO set + headers)
    - No VRF committee participation
    - No block production
    - No checkpoint signing
    - Can earn 75% pool rewards if online
```

```
REWARD ELIGIBILITY (Light/IoT vs Full Node):
    ┌──────────────────────────────────────────────────────────────┐
    │                    REWARD COMPARISON                         │
    ├──────────────────────────────────────────────────────────────┤
    │                        │ FULL NODE   │ LIGHT/IoT NODE       │
    │ 25% Producer Share     │ YES         │ NO (not a producer)  │
    │ 75% Pool (if online)  │ YES         │ YES (if in window)   │
    │ VRF Committee          │ YES         │ NO                   │
    │ Checkpoint Signing      │ YES         │ NO                   │
    │ Heartbeats              │ YES         │ YES                  │
    └──────────────────────────────────────────────────────────────┘
```

```
SWITCHING BETWEEN MODES:
    Light → Full: User enables "Full Node Mode" in GUI
             → Begins recording heartbeats
             → After 137 days → promoted to Full Node

    Full → Light: User disables "Full Node Mode"
            → Status becomes DORMANT after 137 days offline
            → Can reactivate instantly with 1 heartbeat
            → Full Node status is PERSISTENT (registry entry remains)
```

### 6.5 Data Sovereignty Declaration (First Start Gate)

Every user must confirm the Data Sovereignty Declaration before wallet creation.

```
┌─────────────────────────────────────────────────────────────────┐
│  DATA SOVEREIGNTY DECLARATION                                   │
│                                                                 │
│  Gate: Must be confirmed before wallet creation                  │
│  Purpose: User's explicit assertion of their rights             │
└─────────────────────────────────────────────────────────────────┘
```

```
UI FLOW:

    ┌────────────────────────────────────────┐
    │  WELCOME TO ELEKTRON NET               │
    │                                        │
    │  Before you begin, you must confirm:   │
    │                                        │
    │  "I install this software with the     │
    │   explicit intent that my transaction  │
    │   data and network activity are never  │
    │   permanently stored – anywhere, by     │
    │   anyone, at any time.                │
    │                                        │
    │   I understand that Elektron Net        │
    │   enforces this through automatic      │
    │   deletion after exactly 137 days.     │
    │                                        │
    │   → automatic    (no action required)  │
    │   → unconditional (no exceptions)      │
    │   → irreversible  (math enforced)      │
    └────────────────┬───────────────────────┘
                     │ User confirms
                     ▼
    ┌────────────────────────────────────────┐
    │  CONFIRMATION RECORDED                 │
    │  (stored locally, signed with seed)    │
    │  Proceed to wallet creation            │
    └────────────────────────────────────────┘
```

```
IMPLEMENTATION:

    struct SovereigntyConfirmation {
        std::array<uint8_t,32> user_pubkey;      // derived from seed
        std::array<uint8_t,32> declaration_hash; // BLAKE3(DATA_SOVEREIGNTY_TEXT)
        std::array<uint8_t,64> signature;         // Ed25519_Sign(seed, declaration_hash)
        uint64_t               timestamp;         // current_slot
    };
    // Serialized as JSON → ~/.elektron/sovereignty.json

    This is NOT broadcast. It's a local cryptographic record.
    The declaration is architectural, not contractual.
```

```
WHY THIS MATTERS:

    1. INFORMED CONSENT:
        User explicitly acknowledges 137-day pruning
        No surprises later about "missing history"

    2. PHILOSOPHY IN CODE:
        The declaration is not legal boilerplate
        It's the user's assertion of their rights
        Embedded in the protocol itself

    3. CRYPTOGRAPHIC PROOF:
        User signs declaration with their seed
        Proof they were informed before creating wallet
        No later claims of ignorance possible
```

### 6.6 QR Code Workflows

QR codes enable offline payment requests and easy mobile scanning.

```
┌─────────────────────────────────────────────────────────────────┐
│  QR CODE PAYMENT SYSTEM                                          │
│                                                                 │
│  Receiving: Generate QR with address (+ optional amount)         │
│  Sending:    Scan QR → Parse URI → Confirm → Broadcast          │
│  Format:     ELEKTRON: URI scheme (BIP-21 compatible)           │
└─────────────────────────────────────────────────────────────────┘
```

```
ELEKTRON URI SCHEME:
    ELEKTRON:<address>?amount=<elek>&message=<text>

    Address:    Bech32m "elek1..." stealth address
    Amount:    Optional, in Elek (1.0 = 100,000,000 Lep)
    Message:   Optional, max 128 chars UTF-8

    Examples:
        ELEKTRON:elek1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq9x7d9
        ELEKTRON:elek1qqqqqq...x7d9?amount=0.5
        ELEKTRON:elek1qqqqqq...x7d9?amount=0.5&message=Coffee
```

### 6.6.1 Generate QR (Receiving)

```
RECEIVE PAYMENT FLOW:

    User clicks "Receive" in wallet
        │
        ▼
    ┌──────────────────────────────────────────┐
    │ GENERATE STEALTH ADDRESS                 │
    │ Derive new stealth address per payment    │
    │ (never reuse addresses for privacy)      │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ PARSE PARAMETERS                         │
    │ amount = user_input (optional)           │
    │ message = user_input (optional)          │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ BUILD URI                                │
    │ uri = "ELEKTRON:" + address              │
    │ if amount: uri += "?amount=" + amount    │
    │ if message: uri += "&message=" + message │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ ENCODE QR                               │
    │ Encode UTF-8 uri as QR matrix           │
    │ Version: auto (2-40 based on length)    │
    │ Error correction: Level M (15%)         │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ DISPLAY                                  │
    │ Render QR image                          │
    │ Show address text below (copy button)    │
    │ Show amount + message if set             │
    │ User shares or saves QR image            │
    └──────────────────────────────────────────┘
```

```
QR VERSION SELECTION:
    Length 1-25 chars:    Version 2  (25×25 modules)
    Length 26-49 chars:  Version 4  (29×29 modules)
    Length 50-84 chars:  Version 7  (37×37 modules)
    Length 85-134 chars: Version 10 (41×41 modules)
    Length 135-198 chars: Version 14 (45×45 modules)

    Address alone (~27 chars): Version 2
    Address + amount:        ~34 chars → Version 4
    Address + amount + msg:   ~60 chars → Version 7
```

### 6.6.2 Scan QR (Sending)

```
SEND VIA QR FLOW:

    User clicks "Send" → "Scan QR"
        │
        ▼
    ┌──────────────────────────────────────────┐
    │ ACTIVATE CAMERA                           │
    │ Show viewfinder with QR detection         │
    │ Detect QR code patterns                  │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ DECODE QR                                │
    │ Decode matrix → raw bytes                │
    │ Try UTF-8 decode                        │
    │ Detect ELEKTRON: prefix                  │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ PARSE URI                                │
    │ Parse address (Bech32m with elek1 prefix)│
    │ Parse amount (if present)                │
    │ Parse message (if present)               │
    │ Validate all fields                      │
    │ IF invalid → Show error, abort           │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ CONFIRM PAYMENT                          │
    │ Display: To, Amount, Message              │
    │ Show address first/last 6 chars          │
    │ User reviews and confirms                │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ BUILD TRANSACTION                         │
    │ Create tx to stealth address              │
    │ Sign with Ed25519                        │
    │ Add transaction fee                      │
    └──────────┬───────────────────────────────┘
               │
               ▼
    ┌──────────────────────────────────────────┐
    │ BROADCAST                                │
    │ Send to connected full nodes             │
    │ Monitor for confirmation                 │
    │ Show txid and status                    │
    └──────────────────────────────────────────┘
```

```
URI VALIDATION:
    1. Starts with "ELEKTRON:"
    2. Address after prefix:
       - Valid Bech32m charset
       - Starts with "elek1"
       - Valid checksum
    3. Amount (if present):
       - Positive number
       - Max 21,000,000 Elek
       - Max 8 decimal places
    4. Message (if present):
       - Valid UTF-8
       - Max 128 bytes
```

### 6.6.3 Invoice QR (Merchant)

For point-of-sale or invoices, servers generate unique addresses.

```
INVOICE QR WORKFLOW:

    MERCHANT SERVER:
        1. Generate unique invoice_id
        2. Derive address_i = derive(invoice_id)
        3. Create QR: ELEKTRON:address_i?amount=X
        4. Store invoice with address_i and amount

    CUSTOMER:
        1. Scan QR
        2. Confirm payment
        3. Transaction broadcast

    MERCHANT (on tx confirm):
        1. Detect incoming to address_i
        2. Mark invoice #invoice_id as PAID
        3. Release goods/service
```

### 6.6.4 QR Library Dependencies

```
QR ENCODING:
    C++: libqrencode (C library, C++ wrapper) or nayuki QR Code generator
    Encode: std::string → QR Matrix → Image
    Version: auto-select based on data length
    Error correction: Level M (15%)

QR DECODING:
    C++: ZBar (C/C++ library) or ZXing-C++ (port of ZXing)
    Camera frame → Detect patterns → Decode → std::string

QR IMAGE:
    Output formats: PNG (lodepng), SVG (string generation), JPEG
    Display size: 200×200px minimum for scanning
    High-contrast: Black on white (default)
```

---

## 7. Synchronization

### New Node Sync

```
┌──────────────┐
│  BOOT_NODE   │
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 1. VERIFY_GENESIS                        │
│    genesis_hash = hardcoded()             │
│    assert(chain.genesis == expected)      │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 2. DISCOVER_PEERS                        │
│    DNS seeds (config file, editable)     │
│    If all fail → Genesis Mode            │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 3. DOWNLOAD_GENESIS_BLOCK                │
│    From any peer (all have it)           │
│    Verify: block height = 0              │
│    Contains: initial UTXO Root Hash       │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 4. DOWNLOAD_CHECKPOINT_CHAIN              │
│    Request from 3+ peers                 │
│    Verify: 92/137 BLS signatures        │
│    (2/3 threshold from committee)        │
│    Cross-reference across peers           │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 5. VERIFY_UTXO_ROOT                      │
│    Compare UTXO Root Hash                │
│    From Genesis Block header              │
│    From latest Checkpoint                │
│    Must match                            │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 6. DOWNLOAD_UTXO_SET                     │
│    From 3+ peers in parallel             │
│    10 MB chunks                          │
│    Recalculate UTXO Root                 │
│    Verify == Checkpoint.utxo_root        │
│    If mismatch → block peers, retry      │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 7. SYNC_BLOCKS                          │
│    Download blocks since checkpoint       │
│    Validate each block                   │
│    Apply to UTXO set                     │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────┐
│ SYNC_COMPLETE│
└──────────────┘
```

---

## 8. Payment Channels

### 8.1 Channel Lifecycle

```
┌───────────┐
│ PROPOSED  │
└─────┬─────┘
      │ funded
      ▼
┌───────────┐     ┌───────────┐     ┌───────────┐
│   OPEN    │────▶│ UPDATING  │────▶│  CLOSED  │
└───────────┘     └───────────┘     └───────────┘
      │               │
      │               ▼
      │          ┌───────────┐
      └─────────▶│  DISPUTE  │
                 │ (36 blocks)│
                 └───────────┘
```

### 8.2 Channel Open

```
ALICE                          BOB
  │                              │
  │──── Channel Proposal ────────▶│
  │◀─── Accept ─────────────────│
  │                              │
  │──── Funding TX ──────────────▶│ (on-chain)
  │◀─── Funding TX ─────────────│ (on-chain)
  │                              │
  │──── Initial State ──────────▶│
  │◀─── Signature ──────────────│
  │                              │
  │ Channel OPEN                  │
```

### 8.3 Channel Update

```
ALICE pays BOB 100 cLep

ALICE                          BOB
  │                              │
  │──── New State (seq+1) ──────▶│
  │◀─── Signature ──────────────│
  │                              │
  │ State updated, old discarded │
```

### 8.4 Unilateral Close

```
ALICE suspects BOB offline

ALICE:
  1. Broadcast latest state on-chain
  2. Open 36-block dispute window

IF BOB has newer state:
  1. Bob broadcasts newer state within 36 blocks
  2. Higher sequence wins

AFTER 36 blocks:
  Settlement TX broadcast
  Funds distributed
```

### 8.5 Lepton Streams

```
STREAM TYPES:
  - Per-second (video, voice)
  - Per-KB (data transfer)

SENDER                          RECEIVER
  │                                │
  │──── Open Stream ──────────────▶│
  │                                │
  │──── Continuous cLep ──────────▶│ (per second/KB)
  │◀─── Service ──────────────────│
  │                                │
  │──── Close Stream ──────────────▶│
  │◀─── Final Settlement ─────────│
```

### 8.6 Channel Security: Producer as Watchtower

```
DISPUTE WINDOW: 36 blocks (36 minutes)

STANDARD CHANNEL CLOSE:
  ALICE or BOB broadcasts latest state
  → Other party has 36 blocks to contest
  → No contest → state is final

FRAUD DETECTION (Producer validates):

  PRODUCER (when producing a block):
    1. For each channel close transaction:
       - Check: Is this the latest known state?
       - If NO → This is a fraud attempt

  PRODUCER BROADCASTS FRAUD PROOF:
    1. Collects evidence: old state, newer state, signatures
    2. Broadcasts as special transaction type
    3. Network validates

  FRAUD PENALTY:
    - Fraudster loses security deposit (2x disputed amount)
    - Producer who caught fraud earns 50% of penalty
    - Honest party recovers full funds

WHY THIS WORKS:
  - Producer already validates ALL transactions
  - Watchtower is a natural side-effect of block production
  - No extra computation, storage, or cost
  - 25% producer reward covers this work
```

### 8.7 HTLC: Multi-Hop Routing

HTLC enables payments through untrusted intermediaries without requiring direct channels.

```
ATOMIC PAYMENT FLOW: Alice → Bob → Carol → Dave

┌─────────────────────────────────────────────────────────────────┐
│ HTLC TIMELINE (hash-timeline)                                  │
│                                                                 │
│  t = 0: Dave generates secret R, computes H = BLAKE3(R)       │
│         H is shared with Alice (out-of-band or invoice)        │
│                                                                 │
│  t = 1: Alice creates HTLC to Bob                               │
│         {amount, H, expiry = block_height + 36}                 │
│         Alice's HTLC is locked until:                           │
│           - Dave reveals R (and Alice learns R)                 │
│           - OR expiry reaches block_height + 36                 │
│                                                                 │
│  t = 2: Bob creates HTLC to Carol                               │
│         Same H, expiry = block_height + 24 (earlier)           │
│                                                                 │
│  t = 3: Carol creates HTLC to Dave                              │
│         Same H, expiry = block_height + 12 (earliest)          │
│                                                                 │
│  t = 4: Dave reveals R (has 12 blocks to do so)               │
│         Carol sees R → claims from Bob's HTLC                  │
│         Bob sees R → claims from Alice's HTLC                  │
│         Alice's payment is settled through the chain            │
└─────────────────────────────────────────────────────────────────┘
```

```
PAYMENT RULES:

  1. HASH-LOCK: Payment locks until R is revealed
     - R = BLAKE3(pre_image) — pre_image known only to receiver

  2. TIME-LOCK: Each hop has strictly decreasing expiry
     - Alice → Bob:    +36 blocks
     - Bob → Carol:    +24 blocks  (Bob can claim refund first)
     - Carol → Dave:   +12 blocks  (Dave must reveal before Carol)

  3. ATOMICITY: All-or-nothing
     - Either Dave reveals R and all hops succeed
     - OR too late → all HTLCs expire and refund

  4. REVELATION PROPAGATION:
     - When Carol claims, Bob learns R from the blockchain
     - Bob can now claim from Alice's HTLC
     - Chain continues until Alice's HTLC is resolved
```

```
IMPLEMENTATION:

  HTLC OUTPUT SCRIPT:
    OP_HASH256 <H> OP_EQUAL
    OP_IF
      <receiver_pubkey>
    OP_ELSE
      <expiry_height> OP_CHECKLOCKTIMEVERIFY OP_DROP
      <sender_pubkey>
    OP_ENDIF
    OP_CHECKSIG

  MAX PENDING HTLCs per channel: 137 (α⁻¹)

  Routing is source-based:
    - Alice knows the full path: Bob → Carol → Dave
    - Each hop only knows previous and next peer
    - No routing table or path discovery needed
```

```
SECURITY PROPERTIES:

  - Intermediaries cannot steal (they don't know R)
  - Sender can always reclaim after expiry (refund path)
  - Receiver must act before earliest expiry (Dave's 12-block window)
  - Network cannot lose money: atomic settlement or refund
```

---

## 9. Pruning

```
EVERY BLOCK (after finalization)

┌──────────────────────────────────────────┐
│ 1. CALCULATE_CUTOFF                      │
│    cutoff_height = current_height -      │
│      (137 days × 1440 blocks/day)        │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 2. PRUNE_BLOCKS                          │
│    For each block <= cutoff_height:      │
│      DELETE:                             │
│        - Transaction content              │
│        - Input/output details             │
│      KEEP:                              │
│        - Block header (80 bytes)         │
│        - Merkle root                     │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 3. UPDATE_UTXO_SET                       │
│    Remove spent outputs older than cutoff │
│    Keep unspent outputs                  │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 4. CREATE_CHECKPOINT (every 197,280 blocks = 137 days)│
│    If height % 197280 != 0: skip                         │
│    If yes:                                                  │
│      checkpoint = {                                         │
│        height,                                              │
│        hash,                                                │
│        utxo_root,                                          │
│        BLS signatures                                       │
│      }                                                      │
│      Save checkpoint                                       │
└──────────────────────────────────────────┘
```

### 9.1 Checkpoint Creation Workflow

Checkpoints are created by the VRF committee and signed with BLS aggregate signatures.

```
┌─────────────────────────────────────────────────────────────────┐
│  CHECKPOINT CREATION (Every 197,280 blocks = 137 days)         │
└─────────────────────────────────────────────────────────────────┘

TRIGGER: block_height % 197280 == 0

STEP 1: DETECT CHECKPOINT HEIGHT
    Every finalized block:
        IF block_height % 197280 == 0:
            → This is a checkpoint block
            → Proceed to checkpoint creation

STEP 2: COLLECT UTXO ROOT AT THIS HEIGHT
    utxo_root = Merkle_root(current_UTXO_set)
    // This root commits to ALL current unspent outputs
    // Only UTXOs exist at this point (historical txs pruned)

STEP 3: VRF COMMITTEE SIGNS
    VRF committee for this slot = top 137 nodes (by VRF output)
    For each committee member:
        Compute: sig_i = BLS_Sign(private_key_i, checkpoint_data)
        checkpoint_data = hash(height || utxo_root || block_hash)
    Aggregate: agg_sig = BLS_Aggregate(sig_1, sig_2, ..., sig_137)

STEP 4: BROADCAST CHECKPOINT
    checkpoint_message = {
        height:        current_height,
        block_hash:     hash_of_this_block,
        utxo_root:      merkle_root_of_UTXO_set,
        agg_signature:  BLS_aggregate_from_committee
    }
    Broadcast to all peers

STEP 5: NETWORK STORES CHECKPOINT
    All full nodes:
        Verify: BLS_Verify(agg_sig, checkpoint_data, committee_pubkeys)
        IF valid (92/137 signatures):
            Store checkpoint in checkpoint_chain.db
        ELSE:
            Reject checkpoint as invalid
```

```
CHECKPOINT STRUCTURE (on-disk):

    checkpoint_chain.db:
        key:   height (u64)
        value: Checkpoint {
            height:       u64,
            block_hash:   [u8; 32],
            utxo_root:    [u8; 32],    // Merkle root of UTXO set
            signature:    [u8; 96],     // BLS aggregate
            bitmask:      [u8; 137/8], // which members signed
            timestamp:    u64
        }

    Checkpoint interval: every 197,280 blocks (137 days)
    Total checkpoints after 10 years: ~26
```

```
TRUST MODEL FOR CHECKPOINTS:

    Genesis Block (height 0):
        - Hardcoded in source code
        - Contains initial UTXO root (from genesis distribution)
        - NEVER deleted, NEVER modified

    Regular Checkpoints (every 137 days):
        - Signed by VRF committee (randomly selected 137 nodes)
        - Requires 92/137 BLS signatures (2/3 Byzantine threshold)
        - An attacker needs 46+ colluding committee members to forge

    Committee Selection:
        - VRF ensures randomness: cannot predict or bias committee
        - Checkpoint hash input: slot || checkpoint_hash || utxo_root
        - This input is publicly known before the slot
        - No single entity controls committee selection
```

### 9.2 UTXO Set Download & Multi-Source Verification

New nodes and light nodes must download and verify the UTXO set.

```
┌─────────────────────────────────────────────────────────────────┐
│  UTXO SET DOWNLOAD WORKFLOW (3+ Source Verification)           │
└─────────────────────────────────────────────────────────────────┘

STEP 1: DOWNLOAD FROM MULTIPLE PEERS
    Connect to 3+ full nodes simultaneously
    For each peer:
        Request: GET_UTXO_SET
        Response: stream of (utxo_key, utxo_value) pairs
        Store in temporary database per peer

STEP 2: VERIFY UTXO ROOT MATCHES CHECKPOINT
    For each peer's UTXO set:
        Compute: local_utxo_root = Merkle_Root(local_utxo_set)
        Compare: local_utxo_root == checkpoint.utxo_root
        IF mismatch:
            → Flag peer as INVALID
            → Exclude from consensus
            → Try different peer

STEP 3: CROSS-REFERENCE ACROSS PEERS
    Compare UTXO sets between peers:
        IF utxo_set_A == utxo_set_B:
            → Consistent
        IF utxo_set_A != utxo_set_B:
            → At least one peer has corrupted data
            → Find intersection of all sets
            → Use majority consensus

STEP 4: FINAL VERIFICATION
    After finding consistent UTXO set:
        Compute merkle_root_consistent
        Final check: merkle_root_consistent == checkpoint.utxo_root
        IF valid:
            → UTXO set is verified and complete
            → Save to permanent storage
        ELSE:
            → Abort sync, retry with different peers
```

```
UTXO SET FORMAT (on-disk):

    utxo.db (sled or RocksDB):
        key:   OutPoint (txid[32] || vout[4])
        value: TxOutput {
            amount:        u64,
            script_pubkey: Vec<u8>,
            height:        u64,     // block when created
            is_spent:      bool
        }

    Total UTXOs after 10 years: ~3-6 million
    Total UTXO set size: ~3-6 GB
```

```
SYNC SOURCES SELECTION:

    peers = select_peers(min = 3):
        - Prefer peers with highest handshake_version
        - Prefer peers with recent block height
        - Exclude peers flagged for DoS violations
        - Ensure geographic diversity if possible

    If < 3 peers available:
        - Retry every 60 seconds
        - After 5 failures: enter EMERGENCY_GENESIS mode
          (see Section 12.3)
```

### 9.3 Checkpoint Chain Download & Verification

Full sync requires downloading the full checkpoint chain.

```
CHECKPOINT CHAIN DOWNLOAD:

    Request: GET_CHECKPOINT_CHAIN
    Response: List of all checkpoints from height 0 to latest

    For each checkpoint in chain:
        Verify BLS signature:
            threshold_signatures_verified = BLS_Verify(
                checkpoint.agg_signature,
                hash(height || utxo_root || block_hash),
                committee_pubkeys_at_that_height
            )
            IF !threshold_signatures_verified (less than 92 sigs):
                → Reject checkpoint as invalid

    Checkpoint chain is SHORT:
        - ~26 checkpoints after 10 years
        - Each checkpoint: ~200 bytes
        - Total: ~5 KB for entire chain
```

```
FULL NODE RECOVERY PROCEDURE:

    NEW NODE BOOT:
        1. Hardcoded: genesis_block_hash
        2. Download genesis block (verify hash matches hardcoded)
        3. Download checkpoint chain (26 entries after 10 years)
        4. Verify each checkpoint BLS signature (92/137 threshold)
        5. Download UTXO set from 3+ peers
        6. Verify UTXO root against latest checkpoint
        7. Cross-reference UTXO sets between peers
        8. Block sync (headers first, then bodies for recent 137 days)

    LIGHT NODE BOOT:
        1. Hardcoded: genesis_block_hash
        2. Download all block headers (not bodies)
        3. Download latest checkpoint only
        4. Verify checkpoint BLS signature
        5. Download UTXO set from 3+ peers
        6. Verify UTXO root against checkpoint
        7. Apply Bloom-Filter to find own UTXOs
```

---

## 10. Reward Distribution

### 10.1 Block Reward (Producer – Instant)

```
TRIGGER: Block finalised

┌──────────────────────────────────────────┐
│ 1. CALCULATE_PRODUCER_REWARD              │
│    producer_share = block_reward × 0.25   │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 2. PAY_PRODUCER_INSTANT                  │
│    producer.balance += producer_share     │
│    Create UTXO for producer               │
│    (Spendable immediately)               │
└──────────────────────────────────────────┘
```

### 10.2 Pool Reward (Epoch Batching – Every 10 Minutes)

```
REWARD CYCLE: Every 10 minutes (1 epoch = 10 blocks)
→ Up to 21,000 DIFFERENT nodes receive rewards EACH epoch
→ Next epoch: potentially different 21,000 nodes
→ NOT cumulative: every epoch is independent
```

```
TRIGGER: Every 10 blocks (1 epoch = 10 minutes)

┌──────────────────────────────────────────┐
│ 1. ACCUMULATE_POOL                       │
│    For each block in epoch:              │
│      pool_accumulated += block_reward × 0.75 │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 2. DETERMINE_ONLINE_NODES                │
│    Rolling window: last K=10 slots        │
│    Online = participated in any of K       │
│    Participation: committee, block,       │
│      heartbeat (signed, in block)        │
│    Remove duplicates → unique_pubkeys     │
│    Cap at 21,000 (deterministic VRF)     │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 2b. VRF SELECTION (if online_count > 21,000)│
│    When online_nodes > 21,000:            │
│    Deterministic VRF selects top 21,000   │
│    Potentially different nodes each epoch  │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 3. SPLIT_POOL                            │
│    per_node = pool_accumulated /          │
│                min(online_count, 21000)   │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ 4. DISTRIBUTE_EPOCH_REWARDS               │
│    Create 1 UTXO per eligible node        │
│    UTXOs spendable immediately            │
└──────────────────────────────────────────┘
```

### 10.2b Deterministic VRF Node Selection (>21,000 Online Nodes)

When more than 21,000 nodes are online, a deterministic VRF selects exactly 21,000 recipients.

```
┌─────────────────────────────────────────────────────────────────┐
│  VRF SELECTION ALGORITHM (when online_count > 21,000)          │
│                                                                 │
│  Goal: Fair, transparent, deterministic selection of 21,000     │
│  Input: All nodes that participated in rolling window K=10      │
└─────────────────────────────────────────────────────────────────┘

ALGORITHM:

    STEP 1: COLLECT ALL ONLINE NODES
        online_nodes = all nodes that participated in last K=10 slots
        // Each node appears at most once (unique_pubkeys)

    STEP 2: COMPUTE VRF OUTPUT FOR EACH NODE
        For each node in online_nodes:
            vrf_input = hash(epoch_number || node_pubkey || slot_at_epoch_start)
            vrf_output = VRF_prove(node_privkey, vrf_input)
            // vrf_output is a scalar in [0, G)

        // Sort all nodes by vrf_output (lowest = best)
        sorted_nodes = sort_by(online_nodes, vrf_output)

    STEP 3: SELECT TOP 21,000
        selected_nodes = sorted_nodes.take(21,000)

    STEP 4: DISTRIBUTE REWARDS
        per_node = pool_accumulated / 21,000
        For each node in selected_nodes:
            Create UTXO with per_node reward

    STEP 5: BROADCAST SELECTION PROOF
        Producer broadcasts:
            {
                epoch: epoch_number,
                selected_pubkeys: [pubkey_1, ..., pubkey_21000],
                vrf_proofs: [proof_1, ..., proof_21000],
                signature: aggregate_BLS_sig
            }
        Any node can verify the selection independently
```

```
TRANSPARENCY & VERIFIABILITY:

    VRF INPUT COMPONENTS:
        epoch_number:      publicly known (block_height / 10)
        slot_at_epoch_start: publicly known
        node_pubkey:      publicly known (registered on-chain)

    VRF OUTPUT:
        Computed by each node using their private key
        Cannot be predicted or manipulated by anyone else

    VERIFICATION BY ANY NODE:
        For each claimed selected node:
            vrf_verify(node_pubkey, vrf_input, vrf_proof)
            → Returns true/false
        Re-verify sorting and selection:
            sorted = sort_by(all_nodes, vrf_output)
            selected = sorted.take(21,000)
            → Verify the same 21,000 pubkeys
```

```
FAIRNESS PROPERTIES:

    1. EVERY online node has a chance:
        - All nodes that participated in K=10 window are included
        - No node is excluded by design

    2. Selection is unbiased:
        - VRF output is uniformly distributed
        - Cannot be predicted before computation
        - Cannot be manipulated by producer

    3. No Sybil advantage:
        - Running multiple nodes doesn't increase selection probability
        - Each node's VRF is independent
        - One node = one VRF output

    4. Producer cannot cheat:
        - Producer computes VRF for all nodes (not just self)
        - Producer cannot fake VRF proofs (requires private key)
        - Network can verify all proofs independently
```

```
EXAMPLE:

    online_count = 50,000 nodes
    pool_accumulated = 37.5 Elek (10 blocks × 5 × 0.75)

    Step 1: All 50,000 nodes compute VRF outputs
    Step 2: Sort by vrf_output (lowest first)
    Step 3: Take top 21,000
    Step 4: per_node = 37.5 / 21,000 = 0.0017857 Elek
    Step 5: 21,000 UTXOs created

    Remaining 28,999 nodes: 0 reward this epoch
    (But they can try again next epoch)
```

### 10.3 Heartbeat Flow (Quantum-Safe)

```
HEARTBEAT DESIGN:
  - Quantum-safe: BLAKE3-based HMAC, no Ed25519 public key transmitted
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

  Producer computes:
    expected = BLAKE3(stored_hmac_secret || slot_number)
    IF heartbeat_hash == expected:
      → Node is online, add to heartbeat_merkle_root
    ELSE:
      → Heartbeat invalid or node offline

PROPERTIES:
  - Heartbeats are FREE (no fee)
  - No Ed25519 signature needed
  - BLAKE3 is quantum-resistant (hash-based, not ECC)
  - Producer stores hmac_secret locally (not on-chain)
  - No public key ever transmitted in heartbeat
```

### 10.4 Example

```
Epoch (10 blocks), 1,000 nodes online (under 21,000 cap)

Pool accumulated: 10 × (5 × 0.75) = 37.5 Elek
per_node: 37.5 / 1000 = 0.0375 Elek

Producer (per block): 5 × 0.25 = 1.25 Elek (instant)
Producer (epoch): + 0.0375 (pool share)

UTXOs created at epoch end: 1,000 (one per node)

vs WITHOUT batching:
  1,000 × 10 = 10,000 UTXOs
  → 10x reduction with batching ✓
```

### 10.5 Scalability (21,000 Node Cap)

```
Even with 1,000,000 online nodes:
  Only 21,000 nodes receive epoch rewards
  Per-epoch UTXOs: max 21,000
  Per-day UTXOs: 21,000 × 144 = 3,024,000

vs WITHOUT cap:
  1,000,000 × 144 = 144,000,000 UTXOs/day
  → Cap prevents chain bloat ✓
```

---

## 11. Chain Security

### 11.1 Security Model

Elektron Net's security rests on two independent pillars:

```
PILLAR 1: Full Node Registry
  - Only verified Full Nodes can sign blocks
  - 137+ days online time required (on-chain verifiable)
  - Heartbeat tracking shows accumulated uptime
  - DORMANT state after 137 days offline (never auto-removed)

PILLAR 2: pBFT Consensus (2/3 Threshold)
  - 92/137 signatures required for finality
  - Tolerates 45 Byzantine Full Nodes
  - BLS aggregate signatures for efficiency
  - VRF randomization prevents long-term attacks
```

### 11.2 Threat Model

| Threat | Protection |
|--------|------------|
| Sybil Attack (fake nodes) | 137-day uptime barrier + PoW registration |
| Eclipse Attack (network isolation) | IPv6 subnet diversity (max 2 peers per /48) |
| 51% Attack | pBFT + VRF randomization (no PoW possible) |
| Long-Range Attack | Checkpoints every 137 days, VRF input mixing |
| Quantum Attack | Hash-commitment (UTXO stores BLAKE3 hash only) |
| Channel Fraud | Producer-as-Watchtower (25% reward covers validation) |

### 11.3 Peer Scoring & DoS Protection

Peer scoring prevents malicious nodes from disrupting the network.

```
┌─────────────────────────────────────────────────────────────────┐
│  PEER SCORING MECHANISM                                         │
│                                                                 │
│  Every peer has a ban_score (0-100)                            │
│  Behavior above threshold → peer disconnected                   │
└─────────────────────────────────────────────────────────────────┘
```

```
BAN SCORE TABLE:

    Behavior                          | Score | Decay     | Action
    ----------------------------------|-------|-----------|------------------
    Invalid block                     | +20   | -1/hour   | Warning at 50
    Invalid transaction               | +10   | -1/hour   | Warning at 50
    Invalid signature                 | +10   | -1/hour   | Warning at 50
    Invalid checkpoint                 | +30   | -1/hour   | Warning at 50
    Repeated invalid blocks           | +50   | No decay  | Immediate ban
    Sending invalid transactions       | +20   | No decay  | Immediate ban
    Misbehaving protocol messages      | +15   | -1/hour   | Warning at 50
    Timeout (no response)             | +5    | -2/hour   | None
    Exhausted mempool                  | +5    | -2/hour   | None
    ----------------------------------|-------|-----------|------------------
    WARNING threshold: 50              |       |           | Log warning
    BAN threshold: 80                  |       |           | Disconnect + Ban
    Auto-unban after: 24 hours         |       |           | Score reset to 0
```

```
BAN LIST MANAGEMENT:

    peer.db stores:
        banned_peers = {
            peer_id: { ban_score, ban_reason, banned_at, ban_until }
        }

    BAN DURATION:
        score >= 80: ban for 24 hours
        score >= 90: ban for 7 days
        score >= 100: permanent ban (manual unban only)

    UNBAN PROCESS:
        After ban_duration expires:
            ban_score = 0
            Remove from banned_peers
            Peer can reconnect
        Manual unban (admin): clear ban entry immediately
```

```
DOS PROTECTION INTEGRATION:

    P2P LAYER (libp2p):
        - Max connections per peer: 125
        - Max same-subnet peers: 2 (/48 for IPv6)
        - Inbound connection rate limiting: 10/min
        - Outbound connection rate limiting: 30/min

    MEMPOOL PROTECTION:
        - Max 10,000 transactions per peer in mempool
        - Max 100 HTLCs per channel per peer
        - Eviction: lowest-fee transactions first

    BANDWIDTH THROTTLING:
        - Per-peer upload limit: 1 MB/s
        - Per-peer download limit: 4 MB/s
        - Burst allowance: 2x for 5 seconds
```

```
PROTOCOL VIOLATION HANDLING:

    Type 1: Malformed messages
        - Decode fails → +10 score, disconnect
        - No ban for first offense

    Type 2: Invalid state
        - Block doesn't connect → +20 score
        - Transaction conflicts with mempool → +10 score

    Type 3: Timing violations
        - Slot message too early/late → +5 score
        - Heartbeat for future slot → +10 score

    Type 4: Eclipse attack indicators
        - All outgoing peers in same /48 → flag warning
        - No diversity in peer list → peer.ban_score += 15
        - See Section 11.2c for details
```

### 11.4 Eclipse Attack Countermeasure

Eclipse attacks isolate a node from the honest network. Subnet diversity limits this.

```
┌─────────────────────────────────────────────────────────────────┐
│  ECLIPSE ATTACK: Network Isolation                               │
│                                                                 │
│  Attacker floods victim's peer table with malicious peers       │
│  Victim only connects to attacker's nodes                       │
│  Victim sees fake/reordered blockchain                           │
└─────────────────────────────────────────────────────────────────┘
```

```
COUNTERMEASURE: SUBNET DIVERSITY ENFORCEMENT

    IPv4: max 1 peer per /24 subnet
    IPv6: max 2 peers per /48 subnet

    peer.db stores:
        peer_subnets = {
            peer_id: { ip_address, /48_prefix, last_seen }
        }

    ON NEW OUTBOUND CONNECTION:
        1. Compute peer /48 prefix (IPv6) or /24 (IPv4)
        2. Count existing outbound connections to this prefix
        3. IF count >= limit:
            → Reject connection (find different peer)
            → Log: "Eclipse protection: subnet quota full"
        4. ELSE:
            → Accept connection
            → Add to peer_subnets
```

```
ECLIPSE DETECTION:

    MONITORING:
        Every 5 minutes:
            Count distinct /48 prefixes in outbound peers
            IF distinct_prefixes < 5:
                → Log warning: "Low peer diversity"
                → Trigger peer discovery

    ATTACK INDICATORS:
        - All outbound peers from same /48 → HIGH RISK
        - No outbound peers (all inbound) → ISOLATED
        - All peers respond with same chain → SUSPICIOUS

    RESPONSE:
        IF all outbound peers from same /48:
            peer.ban_score += 30  // Eclipse attempt flagged
            Disconnect all to this prefix
            Find new peers from different subnets
```

```
DIVERSITY REQUIREMENTS:

    MINIMUM FOR SAFETY:
        - At least 4 distinct /48 prefixes (IPv6)
        - At least 3 distinct /24 prefixes (IPv4)
        - At least 1 outbound connection from each prefix

    RECOMMENDED:
        - 8+ outbound connections
        - 6+ distinct /48 prefixes
        - Peers from 3+ geographic regions
```

### 11.5 Checkpoint Security

Checkpoints are the chain's memory - they allow new nodes to verify state without downloading all history.

```
CHECKPOINT LIFECYCLE:

CREATION (every 137 days / 197,280 blocks):
  1. VRF Committee creates checkpoint
  2. Checkpoint = { height, hash, utxo_root, sigs }
  3. Threshold: 92/137 BLS signatures (2/3)

VERIFICATION (node sync):
  1. Download checkpoint
  2. Verify 92/137 BLS signatures
  3. Compare UTXO root across 3+ peers
  4. Recalculate UTXO root from downloaded set
  5. If mismatch: reject, try different peers

SECURITY PROPERTY:
  Cannot forge checkpoint without 92/137 Full Nodes
  Committee is randomly selected (VRF) per slot
  Attacker needs 46+ colluding Full Nodes to forge
```

### 11.6 Network Partition Resolution

When a network partition heals, automatic resolution prevents forever-forked chains.

```
ON RECONNECT:
  1. Exchange checkpoint heights with peer

  2. IF heights differ:
     Use checkpoint with HIGHER height
     Reject blocks after fork point

  3. IF heights equal (same checkpoint):
     Use checkpoint with LOWER hash (deterministic tiebreaker)

  4. Resume normal operation

PROPERTY:
  - No manual intervention required
  - Deterministic outcome (always same result)
  - Safety guaranteed (pBFT prevents conflicting finals)
```

### 11.7 Epoch Attack Prevention

Full Nodes ensure the epoch reward distribution is never blocked by a malicious minority.

```
ATTACK SCENARIO:
  Byzantine nodes (up to 45) try to block Slot 9
  to prevent epoch reward distribution

PREVENTION:
  - Full Nodes are VOLUNTARY (user chooses to enable)
  - ACTIVE status: counts toward quorum
  - DORMANT after 137 days offline: does NOT count toward quorum
  - Reactivation: 1 heartbeat → immediately ACTIVE
  - To become FN: must accumulate 137+ days online
  - Cost of 46 fake Full Nodes: 46 x 137 days minimum

RESULT:
  Economic incentive to stay honest
  Dormant state handles offline periods without punishment
```

### 11.8 Genesis Block Trust

The Genesis Block is the chain's root of trust - it is never deleted and hardcoded in every client.

```
GENESIS BLOCK CONTENTS:
  - Initial UTXO Root Hash (verifiable state)
  - Creation timestamp
  - Protocol version
  - No previous block (origin)

PROPERTIES:
  - Hardcoded in software (not downloadable)
  - 80 bytes, never pruned
  - Network is identified by genesis hash
  - All checkpoints chain back to genesis

RECOVERY FROM GENESIS:
  1. Verify genesis hash matches hardcoded value
  2. Download latest checkpoint (92/137 signatures)
  3. Download UTXO set from 3+ peers
  4. Verify UTXO root matches checkpoint
  5. Sync blocks from checkpoint to now
```

### 11.9 Producer Watchtower

Block Producers validate all transactions, including channel close attempts. This serves as a natural watchtower.

```
VALIDATION (every block):
  For each channel close transaction:
    1. Is this the latest known state?
    2. If NO -> Broadcast fraud proof immediately

FRAUD PROOF:
  - Contains: old state, newer state, signatures
  - Network validates automatically
  - Fraudster penalized: 2x disputed amount
  - Producer reward: 50% of penalty

NO EXTRA COST:
  - Producer already validates all transactions
  - 25% block reward covers this work
  - No additional storage or computation needed
```

---
## 12. Error Recovery

### 12.1 Network Partition

```
┌──────────────┐
│  PARTITION  │
│  DETECTED   │
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ PACEMAKER_ACTIVATES                      │
│   k += 1                                │
│   timeout = 60s × 2^min(k, 10)          │
│   VRF committee redrawn                  │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ ON_RECONNECT                             │
│   Exchange finalised checkpoints         │
│                                          │
│   IF checkpoints match:                  │
│     Resume normal operation              │
│     Reset k = 0                          │
│                                          │
│   IF checkpoints differ:                │
│     HALT                                 │
│     Manual resolution required           │
└──────────────────────────────────────────┘
```

### 12.2 Invalid Block

```
┌──────────────┐
│ INVALID     │
│ BLOCK       │
│ RECEIVED    │
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ DETERMINE_REASON                         │
│   Bad signature? Invalid tx?              │
│   Wrong producer? Timestamp?              │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ SCORE_PEER                               │
│   IF from peer:                          │
│     peer.ban_score += penalty            │
│     IF > threshold: ban peer             │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ FALLBACK                                 │
│   Wait for next producer (P2/P3)          │
│   OR skip and trigger pacemaker          │
└──────────────────────────────────────────┘
```

### 12.3 Sync Failure

```
┌──────────────┐
│ SYNC_FAILED │
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ RETRY_DIFFERENT_PEERS                   │
│   UTXO download failed: try new peers    │
│   Increase min_sources                   │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ FALLBACK_FULL_SYNC                       │
│   Download all blocks from Genesis        │
│   Validate every transaction             │
│   Takes longer, zero trust               │
└──────┬───────────────────────────────────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ EMERGENCY_GENESIS                        │
│   If everything fails: start solo        │
│   Warning: separate network              │
└──────────────────────────────────────────┘
```

---

## Appendix A: Parameters

### Time Constants

| Parameter | Value |
|-----------|-------|
| Block Time | 60s |
| Production Window | 45s |
| Epoch Length | 10 blocks (10 minutes) |
| Online Window (K) | 10 slots |
| Pruning Period | 137 days |
| Dispute Window | 36 blocks |
| PoW Duration | 137s |

### Consensus Constants

| Parameter | Value |
|-----------|-------|
| Committee Size | 137 (fixed in Phase 1) |
| Byzantine Threshold | 2/3 = 92 votes |
| Phase 1 Start | N ≥ 137 |
| Phase 1 End | N < 137 → Genesis Mode |

### Economic Constants

| Parameter | Value |
|-----------|-------|
| Max Supply | 21,000,000 Elek |
| Genesis Reward | 5 Elek |
| Halving | 4 years |
| Producer Share | 25% (instant, per block) |
| Online Pool | 75% (epoch end, 10 min) |
| Node Reward Cap | 21,000 |
| Min Transaction Fee | 100 cLep |

---

*End of Workflows Document*
