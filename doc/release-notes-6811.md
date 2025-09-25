Updated RPCs
------------

* The keys `platformP2PPort` and `platformHTTPPort` have been deprecated for the following RPCs, `decoderawtransaction`,
  `decodepsbt`, `getblock`, `getrawtransaction`, `gettransaction`, `masternode status` (only the `dmnState` key),
  `protx diff`, `protx listdiff` and has been replaced with the key `addresses`.
  * The deprecated key is still available without additional runtime arguments but is liable to be removed in future versions
    of Dash Core.
