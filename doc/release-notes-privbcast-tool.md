New binaries
------------

- `bitcoin-privbcast` broadcasts a transaction without revealing the sender's
  IP address, onion address or node identity. It reads the transaction in hex
  on stdin, finds peers through the DNS seeds, resolved through Tor, then
  connects to a small, fixed number of them on a schedule drawn when it
  starts, announces the transaction and stops, without retrying. It prints a
  JSON report of every connection on stdout and progress lines on stderr;
  `bitcoin-privbcast discover` runs only the peer discovery.

  It reaches the network only through a Tor SOCKS5 proxy, at `127.0.0.1:9050`
  unless `-tor` names another loopback address or a unix socket; a remote proxy
  is refused. The proxy must accept username and password authentication and
  Tor's RESOLVE extension.

  `send` exits with status 0 if the announcement was fully sent to at least one
  peer, even if the job failed afterwards; otherwise 1 on bad arguments or
  input, or if the job failed, and 2 if the job ran without failing. Status 0
  does not mean that the transaction was accepted: watch for it with your node
  and run another job if it does not arrive. See
  [Private broadcast](/doc/private-broadcast.md).
