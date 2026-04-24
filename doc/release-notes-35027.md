P2P and network changes
-----------------------

- When `-bind` is used, outgoing connections now originate from the bound address.
  Previously only the listening socket was affected. Proxied connections (Tor, I2P,
  SOCKS5) are not affected. (#6476)
