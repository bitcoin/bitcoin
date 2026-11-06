RPC
---

- Requests refused by `-rpcwhitelist` now get a JSON-RPC error instead of an
  empty HTTP 403 response. A refused request gets the new
  `RPC_METHOD_NOT_PERMITTED` (-38) error, with HTTP status 200 for JSON-RPC 2.0
  requests and 403 for legacy 1.x requests. A batch containing a refused request
  still runs none of its requests, and now gets HTTP 200 with an error for each
  request: -38 for the refused ones, and the new `RPC_REQUEST_NOT_EXECUTED`
  (-39) for the others. HTTP clients that check for a 403 status after sending
  JSON-RPC 2.0 requests or batches should check the error code instead.
  `bitcoin-cli` now prints the permission error. (#36753)
