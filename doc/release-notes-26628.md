JSON-RPC
---

The JSON-RPC server now rejects requests where a parameter is specified multiple times with the same name, instead of silently overwriting earlier parameter values with later ones. (#26628)
