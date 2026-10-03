P2P and network changes
-----------------------

Nodes configured with `-bind` but no specific `-bind=<addr>=onion` now refuse to start when `-listenonion` is enabled.
This includes nodes without Tor configured, since `-listenonion` is enabled by default when listening.
Shared and wildcard binds cannot distinguish Tor-forwarded connections from direct connections, which can grant Tor peers unintended IP-based whitelist permissions.
Nodes without `-bind`, including those using only `-whitebind`, continue to get the default onion target.
Users should add a specific `-bind=127.0.0.1:<port>=onion` to accept incoming Tor connections, or set `-listenonion=0` to disable automatic onion service creation.
