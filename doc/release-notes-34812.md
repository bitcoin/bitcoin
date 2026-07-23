P2P and network changes
-----------------------

- Nodes configured with `-cjdnsreachable` now advertise their CJDNS address
  even when `-externalip` is set. Previously, specifying `-externalip`
  implicitly disabled local address discovery, which also suppressed the
  node's CJDNS address. CJDNS addresses live on a separate overlay network
  and do not conflict with a manually specified external IP. Setting
  `-discover=0` explicitly still disables CJDNS address discovery. (#34812)
