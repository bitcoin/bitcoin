Deprecated or removed RPCs
--------------------------

- The `getaddressinfo` RPC `labels` field now returns an array of label name
  strings. Previously, it returned an array of JSON objects containing `name` and
  `purpose` key/value pairs, which is now deprecated and will be removed in
  0.21. To re-enable the previous behavior, launch bitcoind with
  `-deprecatedrpc=labelspurpose`.
