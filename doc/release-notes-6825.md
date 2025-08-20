# Breaking Change: Block Filter Index Format Update

## Summary
The compact block filter index format has been updated to include Dash special transaction data, providing feature parity with bloom filters for SPV client support. This change is **incompatible** with existing blockfilter indexes.

## Impact
- Nodes with existing `-blockfilterindex` enabled will **fail to start** with a clear error message
- SPV clients will now be able to detect Dash special transactions using compact block filters
- The index must be rebuilt, which may take 30-60 minutes depending on hardware

## Required Action for Node Operators

If you run a node with `-blockfilterindex` enabled, you **must** rebuild the index using one of these methods:

### Option 1: Full Reindex (Recommended)
```bash
dashd -reindex
```
This will rebuild all indexes including the blockfilter index. This is the safest option but will take the longest.

### Option 2: Manual Index Deletion
1. Stop your node
2. Delete the blockfilter index directory:
   ```bash
   rm -rf <datadir>/indexes/blockfilter
   ```
3. Restart your node normally - the blockfilter index will rebuild automatically

## Error Message
If you attempt to start a node with an old blockfilter index, you will see this error:
```
Error: The basic filter index is incompatible with this version of Dash Core. 
The index format has been updated to include special transaction data. 
Please restart with -reindex to rebuild the blockfilter index, 
or manually delete the indexes/blockfilter directory from your datadir.
```

## Technical Details
- The blockfilter index now includes fields from Dash special transactions:
  - ProRegTx (masternode registration)
  - ProUpServTx (masternode service updates)
  - ProUpRegTx (masternode operator updates)
  - ProUpRevTx (masternode revocation)
  - AssetLockTx (platform credit outputs)
- A versioning system has been added to detect incompatible indexes on startup
- The index version is now 2 (previously unversioned)

## Benefits
- SPV clients can now detect and track Dash-specific transactions
- Feature parity between bloom filters and compact block filters
- Protection against serving incorrect filter data to light clients