Updated RPCs
--------

- Any RPC in which one of the parameters are descriptors will throw an error
if the provided descriptor contains a whitespace at the beginning or the end
of the public key within a fragment - e.g. `pk( KEY)` or `pk(KEY )`.
