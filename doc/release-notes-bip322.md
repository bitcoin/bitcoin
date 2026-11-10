Updated RPCs
------------

- `signmessage` and `verifymessage` now support BIP322 simple message proofs
  for native P2WPKH, P2WSH and P2TR addresses. Signing returns `smp` followed
  by a base64-encoded serialized witness stack. Verification also accepts
  older unprefixed simple proofs and requires no wallet or chain data.
- Signing requires a complete satisfaction with the wallet's available
  descriptor keys, scripts and other data; arbitrary scripts and partial
  proofs are not supported. Full signatures, proofs of funds and P2SH
  (including wrapped SegWit) remain unsupported. P2PKH compact signatures
  and the legacy-only `signmessagewithprivkey` RPC are unchanged.
- BIP322 simple proofs fix transaction version, locktime and input sequence
  to zero. Timelock requirements incompatible with these fields cannot be
  satisfied by waiting or importing more keys. Signing may still succeed
  through another satisfiable script branch or a Taproot key path.
- `verifymessage` returns `false` for invalid, unsupported or inconclusive
  proofs. Malformed base64 or witness serialization raises an error (-3).
  A valid message proof does not establish a current balance or a past payment.
