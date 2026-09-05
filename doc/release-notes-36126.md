RPC
---

- A new RPC, `setkeylabel` allows to set a label per master key fingerprint. This is useful for example to identify participants of a multi signature setup, tag hardware wallets, etc. If an empty label is provided, the previous label is removed.

- A new RPC, `getkeylabel` allows to get the label associated to a master key by its fingerprint.

- A new RPC, `listkeylabels` allows to list all labels from all master key fingerprints that have a label.

- The RPC `getaddressinfo` now reports per-key labels (set with `setkeylabel`) under a new `key_labels` object, keyed by master fingerprint. Addresses with multiple keys report one entry per participant key.

- The RPC `gethdkeys` now reports the master key `fingerprint` (8 hex characters) and the per-key `label` (set with `setkeylabel`) for each HD key, when a label is set.
