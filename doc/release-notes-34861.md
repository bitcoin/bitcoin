RPC
---

- The minimum timestamps for `importdescriptors` is set to `0` instead of `1`. Additionally negative timestamps are now invalid in `importdescriptors`, passing a timestamp bellow `0` will throw with error code `-8` (`RPC_INVALID_PARAMETER`).
