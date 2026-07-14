P2P and network changes
-----------------------

- A new `-outboundbind=<addr>` option binds outgoing connections to the given
  local address (one per address family). The address must be assigned to a
  local interface. Proxied connections (Tor, I2P, SOCKS5) are not affected.
  Existing `-bind` semantics are unchanged. (#6476)
