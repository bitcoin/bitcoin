Database errors
---------------

- LevelDB read errors and deserialization errors when looking up an entry in
  the chainstate, block index or optional index databases now abort the node
  after logging the database path and the error. Previously, such errors could
  surface as an initialization error during startup, as an RPC or REST error
  while the node kept running, or be mistaken for a missing entry. The GUI no
  longer offers to rebuild the databases when such an error occurs during
  startup. Missing entries are still reported as not found, and failures to
  open a database are handled as before.
- Transaction index lookups now also abort when an entry refers to an unknown
  block sequence number or to a block in the active chain whose data is
  missing, when a block file cannot be read, or when a legacy entry points to a
  different transaction. Block reads that bypass the transaction index are
  handled as before.
