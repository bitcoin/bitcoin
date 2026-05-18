# RPC

- The RPC `generateblock` takes `output` as an argument. It accepts an address, a descriptor or a list containing address and descriptor strings and `{"address/descriptor": amount}` objects. Each object specifies an exact payout in satoshis. The remaining block reward is split equally among the outputs without specified amounts, with any indivisible satoshis assigned one each to the first such outputs. If every output has a specified amount, any remaining reward is left unclaimed. If no outputs are provided, the reward is burnt in an OP_RETURN output.

- The argument `transactions` is optional in `generateblock`. If no set of transactions is provided, the ones from the mempool are used. If an empty set is provided, an empty block is mined. If a set of transactions is provided, that set of transactions is mined. (#32468)
