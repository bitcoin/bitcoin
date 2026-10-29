# Dash Core version v23.1.9

This is a new patch version release. It fixes a consensus-affecting
transaction-encoding bug, several remotely reachable crashes and
denial-of-service issues in the masternode, DKG, signing and governance paths,
and backports a number of hardening and robustness fixes from later releases.

Upgrading is **strongly recommended** for all nodes, and required for
masternodes.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/dashpay/dash/issues>

# Upgrading and downgrading

## How to Upgrade

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Dash-Qt (on Mac) or
dashd/dash-qt (on Linux).

## Downgrade warning

### Downgrade to a version < v23.0.0

Downgrading to a version older than v23.0.0 is not supported, and will
require a reindex.

# Release Notes

## Consensus

- Transactions whose version field is negative are now rejected once DIP3 is
  active. The version and type share one 32-bit field; a negative version was
  sign-extended when a transaction was re-serialized, so a block containing such
  a transaction could be accepted by a node that received the original bytes and
  rejected by every node that received the re-serialized form, leading to a
  persistent chain split. The new rule rejects these transactions uniformly. No
  valid or historical transaction is affected and transaction ids are unchanged.

## Crash fixes

These issues do not affect consensus rules or put funds at risk, but each one
can take a node offline.

- Fixed an abort during the DKG when a quorum member sent a contribution whose
  decrypted secret-key share was invalid: the share reached an aggregation
  assertion and terminated the process on every honest member of the quorum.
  Invalid shares are now rejected before aggregation.
- Fixed block-template failures caused by provider transactions: a
  `ProUpServTx` with an invalid type, a `ProRegTx` reusing collateral already
  referenced by an in-mempool update, and a pending owner key colliding with a
  Platform node id are now rejected at mempool acceptance.
- Bounded the masternode-list caches by size so a peer cannot drive unbounded
  memory growth between blocks.
- Bounded `DomainPort` deserialization so a small provider transaction can no
  longer make the node allocate and zero very large buffers while holding
  `cs_main`.
- Decoding an unknown-version provider transaction through the verbose decode
  RPCs no longer dereferences a missing network-info object.
- `masternode payments` no longer aborts at the genesis block, and the
  masternode-list RPCs no longer cache a list for a block outside the active
  chain.
- Evonodes no longer abort at credit-pool snapshot boundaries when the pool is
  first constructed outside a block connection (for example by
  `getassetunlockstatuses`).
- Flat-file (block and undo) positions are now stored and seeked as 64-bit
  values. Block-undo data can be several times the size of its block, so a busy
  chain could push a data file past the point a 32-bit offset can address; the
  write and seek paths no longer narrow the position to 32 bits, which avoided a
  silent wrap (and, on Windows, a failure past 2 GiB).

## InstantSend

- Provider (masternode) transactions are no longer given InstantSend locks.
  Locking them served no purpose and interacted badly with lock eviction on
  mempool removal. Software that waited for an InstantSend lock on a provider
  transaction should wait for a ChainLock or for confirmations instead.

## Security and denial-of-service hardening

These issues do not affect consensus and do not put funds at risk.

- DKG contribution verification: a single verification-vector coefficient
  shared between two members no longer fails the whole verification batch.
  Previously that marked unrelated honest members' contributions invalid and
  caused them to be complained about; contributions are now verified
  individually, so only the offending member is affected.
- Signature-share batch verification is now scoped per sender, so one peer can
  no longer get an honest peer penalised or have a forged share stored in place
  of an honest one. Malformed detached quorum share batches are rejected, and
  signing intake is gated against the active-quorum set before any quorum is
  retrieved.
- Governance: an orphan vote now requires a valid masternode signature before
  it is cached, the number of orphan parent objects re-requested per tick is
  bounded, governance vote signature checks are memoised to bound the cost of a
  sync request, a vote is no longer recorded before it is validated, and a
  failed BLS vote-signature check is logged under the governance category
  instead of unconditionally, so a peer cannot fill `debug.log` with it.
- Socket exception monitoring is now consistent between the `poll()` and
  `select()` backends, and a peer whose send buffer stalls is now subject to the
  send timeout, so it can no longer hold a connection slot indefinitely.
- Verified inbound connections are bounded per operator identity; `CHECKDATASIG`
  uses the signature cache; repeated authenticated Platform bans are skipped;
  `DYNBITSET` allocation is bounded against the remaining stream size; a bogus
  InstantSend lock no longer drops a batch of unrelated locks; and
  `-allowprivatenet` no longer influences masternode address validation.

## Wallet

- Invalid-mnemonic errors no longer echo the recovery phrase, an encrypted
  wallet is relocked if an HD upgrade fails after it was unlocked, and a wallet
  unlock deadline can no longer be extended without re-authenticating.

# v23.1.9 Change log

See detailed [set of changes][set-of-changes].

# Credits

Thanks to everyone who directly contributed to this release:

- Konstantin Akimov
- PastaClaw
- PastaPastaPasta
- UdjinM6

As well as everyone that submitted issues, reviewed pull requests and helped
debug the release candidates.

# Older releases

These releases are considered obsolete. Old release notes can be found here:

- [v23.1.8](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.8.md) released Jul/29/2026
- [v23.1.7](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.7.md) released Jun/30/2026
- [v23.1.5](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.5.md) released Jun/19/2026
- [v23.1.4](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.4.md) released Jun/18/2026
- [v23.1.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.3.md) released May/28/2026
- [v23.1.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.2.md) released Mar/12/2026
- [v23.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.1.0.md) released Feb/15/2026
- [v23.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.0.2.md) released Dec/4/2025
- [v23.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-23.0.0.md) released Nov/10/2025
- [v22.1.3](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.3.md) released Jul/15/2025
- [v22.1.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.2.md) released Apr/15/2025
- [v22.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.1.md) released Feb/17/2025
- [v22.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.1.0.md) released Feb/10/2025
- [v22.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-22.0.0.md) released Dec/12/2024
- [v21.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.1.md) released Oct/22/2024
- [v21.1.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.1.0.md) released Aug/8/2024
- [v21.0.2](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.2.md) released Aug/1/2024
- [v21.0.0](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-21.0.0.md) released Jul/25/2024
- [v20.1.1](https://github.com/dashpay/dash/blob/master/doc/release-notes/dash/release-notes-20.1.1.md) released April/3/2024

[set-of-changes]: https://github.com/dashpay/dash/compare/v23.1.8...dashpay:v23.1.9
