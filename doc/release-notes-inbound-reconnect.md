P2P and network changes
-----------------------

- When all inbound connection slots are in use, a new inbound connection from
  an address that already connected twice within the last 10 to 20 minutes no
  longer evicts an existing peer and is dropped instead. Connections from such
  addresses are still accepted whenever a slot is free. This protects existing
  peers from entities that reconnect within seconds of every disconnect.
