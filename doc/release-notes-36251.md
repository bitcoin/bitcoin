Updated REST APIs
-----------------

- The `/rest/spenttxouts/BLOCKHASH.json` endpoint now includes the `generated`
  (whether the previous output is a coinbase output) and `height` fields for
  each previous output, matching the `prevout` objects of the `getblock` RPC
  with verbosity 3. The binary and hex formats are unchanged.
