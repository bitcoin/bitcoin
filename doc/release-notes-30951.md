New settings
------------

- `-v2onlyclearnet` requires v2 transport (BIP324) for outbound and inbound
  connections with IPv4/IPv6 peers: outbound connections to v1-only clearnet
  peers are not attempted, the v1 reconnect after a failed v2 handshake is
  suppressed, and inbound v1 connections from clearnet peers are disconnected.
  Requires `-v2transport=1`. Tor/I2P/CJDNS peers are unaffected, being encrypted
  already, as are peers on non-routable addresses (LAN, loopback). This guards
  message contents against passive on-path observers such as your ISP; it does
  not hide that you are running a Bitcoin node - the default port (8333), the
  plaintext handshake of inbound v1 peers before they are disconnected, and
  traffic patterns all still reveal that - and it offers no protection against
  active adversaries. (#30951)
