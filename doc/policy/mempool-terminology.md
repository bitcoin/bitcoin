## Fee and Size Terminology in Mempool Policy

 * Each transaction has a **weight** and virtual size as defined in BIP 141 (different from serialized size for witness transactions, as witness data is discounted and the value is rounded up to the nearest integer).

   * In the RPCs, "weight", refers to the weight as defined in BIP 141.

 * A transaction has a **sigops size**, defined as its sigop cost multiplied by the node's `-bytespersigop`, an adjustable policy.

 * A transaction's **virtual size (vsize)** refers to its **sigops-adjusted virtual size**: the maximum of its BIP 141 size and sigop size. This virtual size is used to simplify the process of building blocks that satisfy both the maximum weight limit and sigop limit.

   * In the RPCs, "vsize" refers to this sigops-adjusted virtual size.

   * Mempool entry data with the suffix "-size" (eg "ancestorsize") refer to the cumulative sigops-adjusted virtual size of the transactions in the associated set.

 * A transaction can also have a **sigops-adjusted weight**, defined similarly as the maximum of its BIP 141 weight and 4 times the sigops size. This value is used internally by the mempool to avoid losing precision, and mempool entry data with the suffix "-weight" (eg "chunkweight", "clusterweight") refer to this sigops-adjusted weight.

 * A transaction's **base fee** is the difference between its input and output values.

 * A transaction's **modified fee** is its base fee added to any **fee delta** introduced by using the `prioritisetransaction` RPC. Modified fee is used internally for all fee-related mempool policies and block building.
