Updated RPCs
------------

- Descriptors mixing multipath key expressions with single-path ones (e.g.
  `wsh(multi(2,A/<0;1>/*,B/0/*))`) now produce a warning, since the single-path
  key expressions are repeated in every expanded descriptor and the same keys
  end up in all of the resulting scripts. The warning is returned by
  `importdescriptors` and `getdescriptorinfo`. (#35985)

- `getdescriptorinfo` now returns a `warnings` field with the descriptor's
  warnings, including the existing warnings for unsafe `older()` values in
  miniscript descriptors.

- The `warnings` field of RPC results no longer contains duplicates, which
  `importdescriptors` previously returned once per multipath expansion.
