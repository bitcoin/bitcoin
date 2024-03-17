## Wallet

- Backwards compatibility has been dropped for two `getaddressinfo` RPC
  deprecations, as notified in the 19.1.0 and 19.2.0 release notes.
  The deprecated `label` field has been removed as well as the deprecated `labels` behavior of
  returning a JSON object containing `name` and `purpose` key-value pairs. Since
  20.1, the `labels` field returns a JSON array of label names. (dash#5823)
