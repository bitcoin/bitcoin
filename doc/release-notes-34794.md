REST API
--------

- REST responses now include `Cache-Control` headers to guide intermediary
  caches. Immutable responses such as block binary and hex data, block parts,
  block filters, spent transaction outputs, and block-specific deployment info
  are marked cacheable for one day. Responses that can change when the active
  chain changes use `no-cache, must-revalidate`, so caches must check with
  `bitcoind` before reusing a cached copy. Mempool/current node-state responses
  and errors are marked `no-store`. (#34794)
