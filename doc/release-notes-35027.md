P2P and network changes
-----------------------

- A new `-outboundbind=<addr>` option binds outgoing clearnet connections to
  the given local address (one per address family). The address must be
  assigned to a local interface. Connections over Tor, I2P and CJDNS are not
  affected. Existing `-bind` semantics are unchanged. (#35027)
