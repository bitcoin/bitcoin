Block file reobfuscation
-----------------------

- `-reobfuscate-blocks` rewrites existing block and undo files under a new XOR
  key without downloading the chain again. This can obfuscate files created
  before Bitcoin Core 28.0, rotate an existing key, or remove obfuscation.
  With no value or `=1`, it generates a random key. A 16-character hexadecimal
  value selects an exact key, including `0000000000000000` to remove
  obfuscation. `=0` disables a new migration. Keeping `=1` in `bitcoin.conf`
  starts a new migration on every restart. (#33324)

- The node is unavailable while files are rewritten. Reobfuscation processes
  one file at a time. Interrupted migrations resume automatically on startup,
  even without the option. If an I/O error interrupts migration, correct the
  reported filesystem problem and restart. Do not remove or modify the staged
  `.reobfuscated` files.

- Do not run an older binary or another block-data utility until the migration
  has finished. Files rewritten under a nonzero key cannot be read by Bitcoin
  Core versions before 28.0. `-blocksxor=0` can be combined with an explicitly
  selected zero target key to remove obfuscation.
