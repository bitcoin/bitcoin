# Standardness Rules

A transaction is considered "standard" if it passes a set of relay policy checks
in addition to consensus rules. Nodes will not relay or mine non-standard
transactions by default. These rules are local to the node and can be partially
configured via command-line options.

Most standardness checks are applied by `IsStandardTx()`, `AreInputsStandard()`,
and `IsWitnessStandard()` in `src/policy/policy.cpp`. Additional checks (such as
the minimum transaction size) are applied during mempool acceptance in
`src/validation.cpp`.

## Transaction-level checks

- **Version**: Must be between 1 and 3 (inclusive).
- **Weight**: Must not exceed 400,000 weight units (`MAX_STANDARD_TX_WEIGHT`).
- **Non-witness size**: Must be at least 65 bytes (`MIN_STANDARD_TX_NONWITNESS_SIZE`),
  to prevent confusion with 64-byte Merkle tree inner nodes.
- **ScriptSig**: Each input's scriptSig must be at most 1,650 bytes and contain
  only push operations.

## Output-level checks

- **Script type**: Each output's scriptPubKey must be a recognized standard type
  (P2PK, P2PKH, P2SH, P2WPKH, P2WSH, P2TR, multisig, `NULL_DATA`, or
  `ANCHOR`). Unknown or malformed scripts are rejected as `NONSTANDARD`.
- **Data carrier (OP_RETURN)**: Outputs with `OP_RETURN` scripts
  (`NULL_DATA` type) are subject to a configurable size limit (see below).
  Multiple data carrier outputs are allowed; the limit applies to their
  aggregate scriptPubKey size.
- **Bare multisig**: Bare multisig outputs (not wrapped in P2SH) can be
  disabled with `-permitbaremultisig=0`.
- **Dust**: At most one output may be below the dust threshold. The dust
  threshold is derived from `-dustrelayfee` (default: 3,000 sat/kvB).

## Input-level checks

- **P2SH sigops**: A P2SH redeem script must not contain more than 15 sigops.
- **BIP 54 sigops**: The total number of potentially executed legacy signature
  operations across the transaction must not exceed 2,500.
- **Unknown witness programs**: Spending an output with an unknown witness
  version or unknown script type is non-standard.
- **P2WSH limits**: Witness scripts are limited to 3,600 bytes, 100 stack
  items, and 80 bytes per stack item.
- **Tapscript limits**: Stack items are limited to 80 bytes. Annexes are
  non-standard.

## Configurable relay options

| Option | Default | Description |
|--------|---------|-------------|
| `-datacarrier` | `1` | Relay and mine transactions with data carrier (`OP_RETURN`) outputs. Set to `0` to reject all data carrier transactions. |
| `-datacarriersize` | `100000` | Maximum aggregate size (in bytes) of data-carrying `OP_RETURN` scriptPubKeys per transaction. Prior to v30.0, the default was `83`, which limited each `OP_RETURN` output to 80 bytes of payload plus the 3-byte script overhead. To restore the previous behavior, set `-datacarriersize=83`. |
| `-permitbaremultisig` | `1` | Relay and mine transactions with bare (non-P2SH) multisig outputs. Set to `0` to reject them. |
| `-dustrelayfee` | `3000` (sat/kvB) | Fee rate used to calculate the dust threshold. Outputs worth less than the cost to spend them at this fee rate are considered dust. |
| `-bytespersigop` | `20` | Equivalent bytes per sigop in transactions for relay and mining. Affects the virtual size calculation. |

These options affect only the local node's relay and mining policy. They do not
affect consensus validation, and transactions rejected by these rules may still
be included in blocks by miners with different policies.
