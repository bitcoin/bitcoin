Mining and Transaction Relay Policy
=========================

The minimum block feerate (`-blockmintxfee`) has been changed to 1 satoshi per kvB. It can still be changed using the
configuration option.

The default minimum relay feerate (`-minrelaytxfee`) and incremental relay feerate (`-incrementalrelayfee`) have been
changed to 100 satoshis per kvB. They can still be changed using their respective configuration options, but it is
recommended to change both together if you decide to do so.

Other minimum feerates (e.g. the dust feerate, the minimum returned by the fee estimator, and all feerates used by the
wallet) remain unchanged. The mempool minimum feerate still changes in response to high volume but more gradually, as a
result of the change to the incremental relay feerate.

Note that unless these lower defaults are widely adopted across the network, transactions created with lower fee rates
are not guaranteed to propagate or confirm. The wallet feerates remain unchanged; `-mintxfee` must be changed before
attempting to create transactions with lower feerates using the wallet.
