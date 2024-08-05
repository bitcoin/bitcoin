Notable changes
===============

P2P and network changes
-----------------------

- Previously if Bitcoin Core was listening for P2P connections, either using
  default settings or via `bind=addr:port` it would always also bind to
  `127.0.0.1:8334` to listen for Tor connections. It was not possible to switch
  this off, even if the node didn't use Tor. This has been changed and now
  `bind=addr:port` results in binding on `addr:port` only. The default behavior
  of binding to `0.0.0.0:8333` and `127.0.0.1:8334` has not been changed.

  If you are using a `bind=...` configuration without `bind=...=onion` and rely
  on the previous implied behavior to accept incoming Tor connections at
  `127.0.0.1:8334`, you need to now make this explicit by using
  `bind=... bind=127.0.0.1:8334=onion`. (#22729)

- Bitcoin Core will now fail to start up if any of its P2P binds fail, rather
  than the previous behaviour where it would only abort startup if all P2P
  binds had failed. (#22729)
