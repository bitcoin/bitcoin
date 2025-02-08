P2P and network changes
-----------------------
When the `-port` configuration option is used, the default onion listening port will now
be derived to be that port + 1 instead of being set to a fixed value (8334 on mainnet).
This re-allows setups with multiple local nodes using different `-port` and not using `-bind`,
which would lead to a startup failure in v28.0 due to a port collision.

Note that a `HiddenServicePort` manually configured in `torrc` may need adjustment if used in
connection with the `-port` option.
For example, if you are using `-port=5555` with a non-standard value and not using `-bind=...=onion`,
previously Bitcoin Core would listen for incoming Tor connections on `127.0.0.1:8334`.
Now it would listen on `127.0.0.1:5556` (`-port` plus one). If you configured the hidden service manually
in torrc now you have to change it from `HiddenServicePort 8333 127.0.0.1:8334` to `HiddenServicePort 8333
127.0.0.1:5556`, or configure bitcoind with `-bind=127.0.0.1:8334=onion` to get the previous behavior.
(#31223)
