### Updated RPCs

- The `joinpsbts` RPC now sets the merged transaction's `nLockTime` to the
  maximum `nLockTime` among input PSBTs that constrain it. A PSBT constrains
  the locktime only if it has at least one input with `nSequence` not equal
  to `SEQUENCE_FINAL` (0xffffffff) and a non-zero `nLockTime`. PSBTs with all
  inputs using `SEQUENCE_FINAL` or with `nLockTime` 0 no longer affect the
  chosen locktime, since `nLockTime` is not enforced for those inputs. When no
  input PSBT constrains the locktime, the merged transaction uses `nLockTime` 0.
  Among constraining PSBTs, height-based and time-based locktimes cannot be
  mixed. Attempts to join them return an error. Previously, the RPC used
  the minimum `nLockTime` among all PSBTs, which could merge incompatible
  locktime semantics. (#35100)
