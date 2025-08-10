Updated RPCs
------------

* The input field `ipAndPort` has been renamed to `coreP2PAddrs`.
  * `coreP2PAddrs` can now, in addition to accepting a string, accept an array of strings, subject to validation rules.

* The key `service` has been deprecated for some RPCs (`decoderawtransaction`, `decodepsbt`, `getblock`, `getrawtransaction`,
  `gettransaction`, `masternode status` (only for the `dmnState` key), `protx diff`, `protx listdiff`) and has been replaced
  with the key `addresses`.
  * This deprecation also extends to the functionally identical key, `address` in `masternode list` (and its alias, `masternodelist`)
  * The deprecated key is still available without additional runtime arguments but is liable to be removed in future versions
    of Dash Core.
  * This change does not affect `masternode status` (except for the `dmnState` key) as `service` does not represent a payload
    value but the external address advertised by the active masternode.
