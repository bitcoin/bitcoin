RPC changes
-----------
The `walletcreatefundedpsbt` RPC call will fail with
`Insufficient funds` when inputs are manually selected but are not enough to cover
the outputs and fee. Additional inputs can automatically be added through a
new `add_inputs` option.
