Notes on the halfagg module API
===============================

The following sections contain additional notes on the API of the halfagg module (`include/secp256k1_schnorrsig_halfagg.h`).
A usage example can be found in `examples/halfagg.c`.

## Aggregation

Half-aggregation is non-interactive: anyone can aggregate a list of BIP 340 Schnorr signatures without involving the signers.
`secp256k1_schnorrsig_aggregate` aggregates n signatures of 64*n total bytes into an aggregate signature of 32*(n+1) bytes.
`secp256k1_schnorrsig_inc_aggregate` adds further signatures to an existing aggregate signature, which allows growing an aggregate signature over time without keeping the original signatures around.
The aggregate signature depends on the order of the lists of public keys, messages and signatures, so the aggregator and the verifier must use the same order.

Aggregation does not verify the input signatures, it only rejects signatures whose s value is out of range.
Aggregating an otherwise invalid signature succeeds, but the resulting aggregate signature is invalid.
The same applies to the input aggregate signature of `secp256k1_schnorrsig_inc_aggregate`.

## Verification

An aggregate signature is verified with `secp256k1_schnorrsig_aggverify` against the ordered lists of all public keys and messages.
Verification is all-or-nothing: if it fails, it is not possible to determine which of the constituent signatures is invalid.
Applications that need to identify invalid signatures must verify them individually with `secp256k1_schnorrsig_verify` before aggregation.
