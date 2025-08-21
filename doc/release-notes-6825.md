# Breaking Change: Block Filter Index Format Update

## Summary
The compact block filter index format has been updated to include Dash special transaction data, providing feature parity with bloom filters for SPV client support. This change is **incompatible** with existing blockfilter indexes. Existing blockfilter indexes will automatically be re-created with the new version.

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
