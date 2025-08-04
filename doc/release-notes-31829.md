P2P

- The transaction orphanage, which holds transactions with missing inputs temporarily while the node attempts to fetch
its parents, now has improved Denial of Service protections. Previously, it enforced a maximum number of unique
transactions (default 100, configurable using `-maxorphantx`). Now, its limits are as follows: the number of entries
(unique by wtxid and peer), plus each unique transaction's input count divided by 10, must not exceed 3,000. The total
weight of unique transactions must not exceed 404,000 Wu multiplied by the number of peers.

- The `-maxorphantx` option no longer has any effect, since the orphanage no longer limits the number of unique
transactions. Users should remove this configuration option if they were using it, as the setting will cause an
error in future versions when it is no longer recognized.
