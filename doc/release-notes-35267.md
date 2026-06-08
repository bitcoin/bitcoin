RPC
---

The `getprivatebroadcastinfo` and `abortprivatebroadcast` RPCs now return an error `-32601`, "Method not found", when `-privatebroadcast` is not enabled at startup.
