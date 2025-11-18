RPC changes
-----------

- Two new hidden RPC commands have been added for diagnosing and repairing corrupted evodb diff records:
  - `evodb verify` - Verifies the integrity of evodb diff records between snapshots (read-only operation). Returns verification errors if any corruption is detected.
  - `evodb repair` - Repairs corrupted evodb diff records by recalculating them from blockchain data. Automatically verifies repairs before committing to database. Reports critical errors that require full reindex.
