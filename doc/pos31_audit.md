# PoS v3.1 Audit

This document summarizes the current state of proof‑of‑stake (PoS) related code in the BitGold repository and highlights areas still needing work in order to fully mirror Blackcoin's PoS v3.1 (BPoS) implementation.

## Existing PoS Code

The repository already contains a sizeable PoS implementation:

* **Consensus parameters** – `src/consensus/params.h` defines `fEnablePoS`, `posActivationHeight`, `nStakeTimestampMask`, `nStakeMinAge` and `nStakeModifierInterval`.
* **Stake checking** – `src/pos/stake.cpp` and `src/pos/stake.h` implement
  * `CheckStakeKernelHash` (stake kernel creation and hashing)
  * `ContextualCheckProofOfStake` (coinstake structure, coin age, timestamp/slot rules)
  * `CheckStakeTimestamp` helper enforcing the 16‑second mask and tight future drift
  * `IsProofOfStake` utility
  * constants such as `STAKE_TARGET_SPACING` (16s) and `MIN_STAKE_AGE` (1h)
* **Wallet staking** – `src/wallet/bitgoldstaker.cpp` contains a staking loop that constructs and signs coinstakes and submits PoS blocks.
* **Network protocol support** – new P2P message types (`COINSTAKE`, `STAKEMODIFIER`) are declared in `src/protocol.h` and handled throughout `net.cpp`, `protocol.cpp`, and `net_processing.cpp`.
* **Chain parameter wiring** – `src/kernel/chainparams.cpp` and `src/chainparams.cpp` expose `-posactivationheight` and set `nStakeTimestampMask` for various networks.
* **Tests and docs** – multiple functional tests (`test/functional/feature_posv3.py`, `test/functional/pos_block_staking.py`, `test/functional/pos_reorg.py`) and user documentation (`doc/staking.md`, `doc/bitgoldstaking.md`, `doc/pos3.1-overview.md`, etc.) already exist.

## Missing / To‑Do Items

Despite the extensive PoS code, the audit reveals gaps relative to a complete BPoS implementation:

* The code base lacks a clearly separated stake modifier manager or helper resembling Blackcoin's `CheckKernel`/`GetKernelStakeModifier` semantics.
* No `ThreadStakeMiner` or equivalent top‑level miner thread name is present; existing logic lives in `BitGoldStaker` but may require review against upstream behaviour.
* PoS difficulty/target retargeting integration with `GetNextWorkRequired` is not obvious; further investigation is required to confirm parity with Blackcoin's rules.
* A `-staking` CLI flag (alias of `-staker`) now gates staking, though additional optional feature gates beyond PoSV3.1 may still require implementation and documentation.

This audit serves as the baseline for upcoming commits that will add missing consensus parameters, activation logic, stake modifier handling, full PoS validation rules, wallet integration and functional tests to achieve an exact PoS v3.1 implementation.

