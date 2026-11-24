Mempool
-------

- Transactions retained in the mempool use less memory, especially when their
  witnesses contain many elements. The mempool can therefore retain more
  transactions within the same `-maxmempool` memory limit. (#36895)
