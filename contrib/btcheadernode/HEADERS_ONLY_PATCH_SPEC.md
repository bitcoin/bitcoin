# Headers-only patch spec (Bitcoin v30.2 base)

This document defines the expected behavior of the optional Bitcoin patch used by
Syscoin BTCC signer policy checks.

## Base and scope

- Base source is pinned in `bitcoin.lock` (`BITCOIN_COMMIT`).
- Patch must apply cleanly with `git apply` against that commit.
- Patch goal is **header-chain policy support**, not full transaction validation.

## Required operational behavior

- Provide a mode (for example `-headersonly=1`) that:
  - syncs and validates PoW for headers from peers,
  - tracks canonical header tip and competing recent forks,
  - avoids block-body/transaction validation flow (header-only operation).
- Keep network behavior stable enough for long-running sentry use:
  - no aggressive reconnect loops,
  - deterministic startup behavior,
  - useful logs for degraded states.

## RPC contract required by Syscoin signer policy

Syscoin signer policy currently calls:

- `getblockchaininfo`
- `getblockheader <hash> true`
- `getchaintips`

Patch behavior must preserve useful semantics for these calls in headers-only mode:

- `getblockchaininfo`:
  - `bestblockhash` reflects canonical header tip hash.
  - `initialblockdownload` becomes `false` once header tip is considered healthy/synced.
  - tip timestamp and height information should remain consistent with the header view.
- `getblockheader <hash> true`:
  - returns info for headers in the tracked header tree.
  - `confirmations` for canonical headers is meaningful for policy checks.
  - includes correct `height`, `time`, `previousblockhash`, and chainwork-related fields.
- `getchaintips`:
  - includes active tip and recent competing tips in a way usable by fork-stability heuristics.

## Safety expectations

- If header state is unhealthy (peers unavailable, stale tip, corrupted view), RPC output should
  make that detectable so Syscoin signer policy can fail closed and skip signing.
- Do not silently report optimistic confirmation/finality values when header view is degraded.

## Validation checklist for patch authors

- Patch applies cleanly to pinned base commit.
- `bitcoind` and `bitcoin-cli` build with the Syscoin helper build flow.
- Header-only node can run continuously and answer the required RPCs.
- Manual checks:
  - canonical tip hash changes with incoming headers,
  - candidate header hash membership and confirmations are queryable,
  - recent fork tips are visible for policy gating.
