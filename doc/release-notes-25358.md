Updated settings
------

- The maximum allowed value for the `-dbcache` configuration option has been
  dropped due to recent UTXO set growth. Note that before this change, large `-dbcache`
  values were automatically reduced to 16 GiB (1 GiB on 32 bit systems). (#28358)
