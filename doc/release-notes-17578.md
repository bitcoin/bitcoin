Deprecated or removed RPCs
--------------------------

- RPC `getaddressinfo` changes:

  - the `label` field has been deprecated in favor of the `labels` field and
    will be removed in 0.21. It can be re-enabled in the interim by launching
    with `-deprecatedrpc=label`.

  - the `labels` behavior of returning an array of JSON objects containing name
    and purpose key/value pairs has been deprecated in favor of an array of
    label names and will be removed in 0.21. The previous behavior can be
    re-enabled in the interim by launching with `-deprecatedrpc=labelspurpose`.
