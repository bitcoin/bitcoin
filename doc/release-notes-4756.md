Updated RPCs
------------

* The `generatedescriptor` RPC call has been introduced to allow mining a set number of blocks, defined by argument `num_blocks`, with the coinbase destination set to a descriptor, defined by the `descriptor` argument. The optional `maxtries` argument can be used to limit iterations.

* The `generateblock` RPC call will mine an array of ordered transactions, defined by hex array `transactions` that can contain either transaction IDs or a hex-encoded serialized raw transaction and set the coinbase destination defined by the `address/descriptor` argument.

* The `mockscheduler` is a debug RPC call that allows forwarding the scheduler by `delta_time`. This RPC call is hidden and will only be functional on mockable chains (i.e. primarily regtest). `delta_time` must be between 0 - 3600.
