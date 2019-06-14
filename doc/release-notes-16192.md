Wallet: Transaction fees
------------------------

Currently the wallet caps fees at `-maxtxfee=<x>` (default: 
0.10) BTC. A user can set the minimum fee rate for all 
transactions with `-mintxfee=<i>`, which defaults to 
1000 satoshis per kB. Users can also decide to pay a 
predefined fee rate by setting `-paytxfee=<n>` (or 
`settxfee <n>` rpc during runtime). These settings could 
conflict with one another. If capping at `-maxtxfee` would 
render the fee lower than `-mintxfee` per kB or `-paytxfee`
per kB, the wallet will now return an error.