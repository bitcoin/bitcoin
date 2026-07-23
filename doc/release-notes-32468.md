# RPC

- The RPC `generateblock` takes `outputs` as an argument. This argument allows to specify where to distribute the reward of the block. It takes an address, a descriptor or a list of multiple addresses and descriptors. If `outputs` is not set, the reward is burnt in an OP_RETURN output. The reward is split in equal parts through all addresses and descriptors provided.

- The argument `transactions` is optional in `generateblock`. If no set of transactions is provided, the ones from the mempool are used. If an empty set is provided, an empty block is mined. If a set of transactions is provided, that set of transactions is mined.
