New settings
------------

- A new `-v2onlyclearnet` option ensures every outbound connection to an IPv4/IPv6
  peer is encrypted (v2/BIP324); connections to Tor/I2P/CJDNS peers are
  unaffected, being already encrypted. It requires `-listen=0`, which takes
  valuable listening capacity away from the network, so enable it only if
  avoiding unencrypted clearnet traffic is a genuine concern. This guards message
  contents against passive on-path observers (ISPs, firewalls); it does not hide
  that you run a Bitcoin node (default port 8333 and traffic patterns still reveal
  that) and offers no protection against active adversaries. (#30951)
