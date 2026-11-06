Updated RPCs
------------

- `finalizepsbt` now verifies the final scriptSig and scriptWitness of every
  input before reporting the PSBT as complete. Previously, existing final
  fields were trusted as-is when the input had a `non_witness_utxo`, so an
  invalid final witness could be returned as `hex` with `complete: true`. Such
  PSBTs now return `complete: false` and no `hex`. This also applies to final
  scripts that are valid by consensus but non-standard (for example a high-S
  signature), matching `walletprocesspsbt` and `descriptorprocesspsbt`. (#36343)
