Updated RPCs
------------

- `removeprunedfunds` now also accepts an array of transaction ids, which are
  removed from the wallet in a single atomic batch. Passing a single
  transaction id as a string remains supported. The argument has been renamed
  from `txid` to `txids`; clients using named arguments need to update the
  name.
