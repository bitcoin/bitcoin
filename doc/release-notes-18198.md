Low-level RPC Changes
---------------------

- To make RPC `sendtoaddress` more consisent with `sendmany` the following error
    `sendtoaddress` codes were changed from `-4` to `-6`:
  - Insufficient funds
  - Fee estimation failed
  - Transaction has too long of a mempool chain
