# Privacy in Bitcoin Core

## Network connectivity privacy

### Node identity across multiple networks

Operating a node that listens on multiple networks simultaneously (e.g. IPv4
and Tor, or IPv4 and I2P) can help strengthen the Bitcoin network, as nodes in
this configuration (i.e. bridge nodes) increase the cost and complexity of
launching eclipse and partition attacks. However, under certain conditions, an
adversary that can connect to your node on multiple networks may be able to
correlate those identities by observing shared runtime characteristics. It is
not recommended to expose your node over multiple networks if you require
unlinkability across those identities.

### Onion service port isolation

Do not add anything but Bitcoin Core ports to the onion service created in
[doc/tor.md § 3](tor.md#3-manually-create-a-bitcoin-core-onion-service). If
you run a web service too, create a new onion service for that. Otherwise it is
trivial to link them, which may reduce privacy. Onion services created
automatically (as in [§ 2](tor.md#2-automatically-create-a-bitcoin-core-onion-service))
always have only one port open.

## See also

- [doc/tor.md](tor.md) — Tor proxy setup, onion service creation and configuration
- [doc/i2p.md](i2p.md) — I2P setup, configuration, and address types
- [doc/cjdns.md](cjdns.md) — CJDNS setup (note: CJDNS encrypts traffic but
  does not hide the sender from intermediate routers)
