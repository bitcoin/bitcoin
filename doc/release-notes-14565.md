Low-level RPC changes
---------------------

The `importmulti` RPC will now contain a new per-request `warnings` field with strings
that explain when fields are being ignored or inconsistant, if any.
