Updated RPCs
------------

- RPCs now fail to deserialize transactions with more inputs or outputs than
  can fit in a block (more than 24,390 inputs or 111,111 outputs), and return a
  decoding error for them. Previously, such transactions were decoded, and only
  rejected (as `bad-txns-oversize`) when validated. (#36906)
