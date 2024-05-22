RPC
---

The `hash_serialized_2` value has been removed from `gettxoutsetinfo` since the value it calculated contained a bug and did not take all data into account. It is superseded by `hash_serialized_3` which provides the same functionality but serves the correctly calculated hash.
