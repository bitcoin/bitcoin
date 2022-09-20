New RPCs
--------

- `analyzepsbt` examines a PSBT and provides information about what
  the PSBT contains and the next steps that need to be taken in order
  to complete the transaction. For each input of a PSBT, `analyzepsbt`
  provides information about what information is missing for that
  input, including whether a UTXO needs to be provided, what pubkeys
  still need to be provided, which scripts need to be provided, and
  what signatures are still needed. Every input will also list which
  role is needed to complete that input, and `analyzepsbt` will also
  list the next role in general needed to complete the PSBT.
  `analyzepsbt` will also provide the estimated fee rate and estimated
  virtual size of the completed transaction if it has enough
  information to do so.
