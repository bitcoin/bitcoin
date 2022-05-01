Updated RPCs
------------

* The `gettxoutsetinfo` RPC call now accept one optional argument (`hash_type`) that defines the algorithm used for calcluating the UTXO set hash, it will default to `hash_serialized_2` unless explicitly specified otherwise. `hash_type` will influence the key that is used to refer to refer to the UTXO set hash.
