P2P and network changes
-----------------------

- The address a peer reports seeing us as is no longer used for
  self-advertisement on outbound clearnet connections made through a proxy,
  since it is the proxy's address (e.g. a Tor exit node's IP) rather than one
  we can be reached on. This only affects nodes running with `-proxy` that override
  the `-listen=0` and `-discover=0` parameter interactions it implies.
  Operators who want to advertise a specific address can set
  `-externalip=<addr>`. (#35578)
