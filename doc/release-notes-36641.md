Wallet
------

- A wallet now keeps the secret nonces of at most 32768 unfinished MuSig2
  signing sessions, and drops the oldest when a new session starts beyond
  that. The wallet cannot make its partial signature for a dropped session.
  If no participant has added a partial signature for that MuSig2 key yet,
  remove the wallet's public nonce from the PSBT and process it again to make
  a new one. If a partial signature is already present, all participants must
  start the round again with new nonces. (#36641)
