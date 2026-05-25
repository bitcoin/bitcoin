P2P and network changes
-----------------------

- Fix a possible leak of the originator's IP address for transactions sent with
`sendrawtransaction` RPC when `-privatebroadcast=1`. When Bitcoin Core connects
to a peer, if that peer has been advertised to support P2P protocol v2, then
Bitcoin Core tries to use the v2 protocol and if that fails it retries the
connection using the v1 protocol. When the private broadcast is about to send a
transaction to an IPv4 or IPv6 peer it overrides the normal proxy selection and
forces the connection through the Tor proxy (and thus through the Tor network,
protecting the sender's IP address). However if v2 protocol is tried and it
fails, then the v1 retry connection would be made disregarding the "override
proxy" request, possibly making an IPv4 or IPv6 direct connection. In other
words, for this to happen the following must be true:
  1. An IPv4 or IPv6 address has been advertised to the sender, indicating v2
  support.
  2. The sender must have Tor configured.
  3. The sender's configuration must be such that connections to IPv4 or IPv6
  are made directly (no `-proxy=` is used for IPv4 or IPv6).
  4. The recipient does not support v2 (the flags from 1. are bogus). (#35319)
