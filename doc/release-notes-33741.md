JSON-RPC
--------

The `getmempoolinfo`, `getnetworkinfo`, `estimatesmartfee` and `estimaterawfee` now take a new options argument `sat_vb`.
If enabled feerates on the RPC result will be represented in `sat/vB` instead of `BTC/kvB`. (#33741)
