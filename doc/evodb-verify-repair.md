# EvoDb Verify and Repair RPC Commands

### `evodb verify`

Verifies the integrity of evodb diff records between snapshots stored every 576 blocks.

**Syntax:**
```
evodb verify [startBlock] [stopBlock]
```

**Parameters:**
- `startBlock` (optional): Starting block height. Defaults to DIP0003 activation height.
- `stopBlock` (optional): Ending block height. Defaults to current chain tip.

**Returns:**
```json
{
  "startHeight": n,           // Actual starting height (may be clamped to DIP0003)
  "stopHeight": n,            // Ending block height
  "diffsRecalculated": 0,     // Always 0 for verify mode
  "snapshotsVerified": n,     // Number of snapshot pairs that passed verification
  "verificationErrors": [     // Array of errors (empty if verification passed)
    "error message",
    ...
  ]
}
```

**Description:**

This is a **read-only** operation that checks whether applying the stored diffs between consecutive snapshots produces the expected results. It does not modify the database.

The command processes snapshot pairs (snapshots are stored every 576 blocks) and applies all diffs between them, verifying that the result matches the target snapshot. If all verification passes, `verificationErrors` will be empty.

**Use cases:**
- Diagnose suspected evodb corruption
- Verify database integrity after hardware issues
- Confirm successful repair operations

**Example:**
```bash
# Verify entire chain
dash-cli evodb verify

# Verify specific range
dash-cli evodb verify 1000 10000
```

---

### `evodb repair`

Repairs corrupted evodb diff records by recalculating them from blockchain data.

**Syntax:**
```
evodb repair [startBlock] [stopBlock]
```

**Parameters:**
- `startBlock` (optional): Starting block height. Defaults to DIP0003 activation height.
- `stopBlock` (optional): Ending block height. Defaults to current chain tip.

**Returns:**
```json
{
  "startHeight": n,              // Actual starting height (may be clamped to DIP0003)
  "stopHeight": n,               // Ending block height
  "diffsRecalculated": n,        // Number of diffs successfully recalculated
  "snapshotsVerified": n,        // Number of snapshot pairs that passed verification
  "verificationErrors": [        // Errors during verification phase
    "error message",
    ...
  ],
  "repairErrors": [              // Critical errors during repair phase
    "error message",             // Non-empty means full reindex required
    ...
  ]
}
```

**Description:**

This command first runs verification on all snapshot pairs in the specified range. For any pairs that fail verification, it recalculates the diffs from actual blockchain data and writes the corrected diffs to the database.

The repair process:
1. **Verification phase**: Checks all snapshot pairs for corruption
2. **Repair phase**: For failed pairs, recalculates diffs from blockchain blocks
3. **Database update**: Writes repaired diffs in efficient 16MB batches
4. **Cache clearing**: Clears both diff and list caches to prevent serving stale data

**Important notes:**
- Requires all blockchain data to be available (blocks must not be pruned in the repair range)
- If repair encounters critical errors (block read failures, missing snapshots), a full reindex is required
- Critical errors are prefixed with "CRITICAL:" in error messages
- Successfully repaired diffs are verified before being committed to the database

**Use cases:**
- Repair corrupted evodb after unclean shutdown
- Fix database inconsistencies after hardware failures
- Recover from disk corruption without full reindex (when possible)

**Example:**
```bash
# Repair entire chain
dash-cli evodb repair

# Repair specific range
dash-cli evodb repair 1000 10000
```

---

## When to Use These Commands

### Use `evodb verify` when:
- You suspect evodb corruption but want to diagnose before taking action
- Verifying integrity after hardware issues or crashes
- Confirming successful repairs

### Use `evodb repair` when:
- `evodb verify` reports verification errors
- You experience masternode list inconsistencies
- After unclean shutdown or disk errors
- As an alternative to full reindex when snapshots are intact

### Full reindex required when:
- Repair reports errors prefixed with "CRITICAL:"
- Snapshots are missing or corrupted (cannot be repaired by this tool)
- Block data is unavailable (pruned nodes in repair range)

---

## Technical Details

### Verification Process
- Processes snapshot pairs sequentially (snapshots stored every 576 blocks)
- Applies all stored diffs between snapshots
- Verifies result matches target snapshot using `CDeterministicMNList::IsEqual`
- Reports all errors without stopping early

### Repair Process
- Reads actual blockchain blocks from disk
- Processes special transactions to rebuild masternode lists
- Uses dummy coins view (avoids UTXO lookups for historical blocks)
- Calculates correct diffs and verifies before committing
- Fails fast on critical errors (missing blocks, missing snapshots)

### Performance Considerations
- Repair is I/O intensive (reads blockchain blocks, writes database)
- Progress logged every 100 snapshot pairs
- Database writes batched in 16MB chunks for efficiency
- Caches cleared after repair to ensure consistency

### Implementation Notes
- Both commands require `::cs_main` lock
- Special handling for initial DIP0003 snapshot (may not exist in older databases)
- Only diffs can be repaired; missing snapshots require full reindex
- Repair verification must pass before diffs are committed to database
