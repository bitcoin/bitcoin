P2P and network changes
-----------------------

Nodes configured with `-bind` but no dedicated `-bind=<addr>=onion` now refuse
to start when `-listenonion` is enabled, which is the default when listening.
A shared bind cannot distinguish Tor-forwarded connections from direct connections,
which can grant Tor peers unintended IP-based whitelist permissions.
Users should add a dedicated onion bind to accept incoming Tor connections, or set
`-listenonion=0` to disable automatic onion service creation.
