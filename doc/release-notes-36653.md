Tor
---

- The cookie file named by Tor for authentication must now be a regular file.
  Previously a FIFO there made the node hang at shutdown.

- The SOCKS proxy for onion addresses, which Bitcoin Core gets from Tor's
  control port when `-onion` is not set, is now only taken from a listener on
  the same host as the control port: a loopback address if the control port is
  on loopback, otherwise the `-torcontrol` host. Other listeners, including
  unix sockets, are ignored and a warning is logged. If none is usable, the
  onion proxy set by `-proxy` is now kept instead of being replaced by
  `127.0.0.1:9050`. A remote Tor that listens for SOCKS on a different address
  than its control port needs `-onion` or a `-proxy` that points to it.
