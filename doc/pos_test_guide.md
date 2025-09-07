# Proof-of-Stake Testing Guide

This guide describes the proof-of-stake tests added to the tree and the
expected outcomes when they run successfully.

## Unit tests

The stake unit tests live in `src/test/stake_tests.cpp` and exercise three
core concepts:

- **Stake kernel hashing** – verifies the kernel hash calculation matches a
  deterministic expectation.
- **Stake modifier** – ensures different stake inputs yield distinct kernels.
- **Block validation** – confirms blocks fail when the stake input is below the
  minimum amount.

Run the tests with:

```bash
src/test/test_bitcoin --run_test=stake_tests
```

## Functional tests

Three functional tests cover higher level staking behaviour:

- `pos_block_staking.py` stakes a block using wallet funds.
- `pos_reorg.py` demonstrates a reorganisation when competing stake chains
  meet, with the longer chain winning.
- `pos_slashing.py` builds the slashing tracker and detects a double-sign
  attempt.

Execute an individual test via:

```bash
test/functional/pos_block_staking.py
```

Each script should exit without errors when the expected behaviour is
observed.
