P2P and network changes
-----------------------

With `-listenonion` enabled, nodes configured with normal `-bind` options (but no dedicated onion bind) now skip automatic onion service creation instead of forwarding Tor connections to a normal P2P listener.
These nodes start with their configured listeners and keep Tor control available for outbound proxy discovery, logging a warning after authenticating with Tor.
This removes the shared forwarding that could misclassify incoming Tor connections and grant them unintended IP-based whitelist permissions.
To accept incoming Tor connections, add a specific `-bind=<addr>:<port>=onion` reachable from the Tor daemon.
Nodes without `-bind` options (including those using only `-whitebind`) retain the default dedicated onion listener.
When `-listenonion` is enabled, wildcard onion binds are rejected, so use a specific address such as `-bind=127.0.0.1:<port>=onion` instead.
With `-listenonion=0`, wildcard onion binds are still accepted, but incoming connections to them are not identified as Tor connections.
