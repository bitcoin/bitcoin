# Multisig Setup Wizard

Guides an orchestrator/participant through setting up a k-of-n multisig descriptor wallet.

This utility implements the multisig setup flow tracked in #35645 using available
RPCs and implements workarounds for missing RPC or wallet feature.

*configurations*
`rpc` is a wallet rpc handle.
All steps require an unlocked wallet with private keys enabled.
A coordinator that does not contribute keys needs a watch-only wallet
