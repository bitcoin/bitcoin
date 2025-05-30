- Logging and RPC

  - Bitcoin Core now provide the specific reason and index of the first
    non-standard input in a transaction, applicable to non-standard inputs
    received via P2P and also changes the responses of transaction sending
    RPC's (`submitpackage`, and `sendrawtransaction`) and `testmempoolaccept` RPC.

  - `bad-txns-nonstandard-inputs` will message will now be:
    - `bad-txns-input-script-unknown`: When the input’s `scriptPubKey` is non-standard or, if standard, has an incorrect witness program size.
    - `bad-txns-input-p2sh-scriptsig-malformed`: When the input’s `scriptPubKey` is P2SH, but the `scriptSig` is invalid.
    - `bad-txns-input-scriptcheck-missing`: When the input’s `scriptPubKey` is P2SH, but `scriptSig` is missing.
    - `bad-txns-input-p2sh-redeemscript-sigops`: When the input’s `scriptPubKey` is P2SH, but `scriptSig` has too many signature operations that exceeds  `MAX_P2SH_SIGOPS`.