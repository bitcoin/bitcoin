P2P and network changes
-----------------------

- Manual peers (added using `addnode` or `-connect`) are no longer disconnected
  during IBD for block stalling. Instead, their in-flight block requests are
  released so other peers can continue block download progress.
