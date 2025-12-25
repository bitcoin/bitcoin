New REST API
------------

- A new REST API endpoint (`/rest/blockpart/<BLOCK-HASH>.<bin|hex>?offset=<OFFSET>&size=<SIZE>`) has been introduced
  for efficiently fetching a range of bytes from block `<BLOCK-HASH>`.
