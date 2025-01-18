## New RPCs

- **`getislocks`**
    - Retrieves the InstantSend lock data for the given transaction IDs (txids).
    Returns the lock information in both human-friendly JSON format and binary hex-encoded zmq-compatible format.

## Updated RPCs

- **`getbestchainlock` Changes**
    - A new hex field has been added to the getbestchainlock RPC, which returns the ChainLock information in zmq-compatible, hex-encoded binary format.

