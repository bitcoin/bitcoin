Notable Changes
---------------

* Dash Core has added support for the ability to register multiple addr:port pairs to a masternode and for specifying
  distinct addresses for platform P2P and platform HTTPS endpoints. The consensus and format changes needed to enable
  this capability is referred to as "extended addresses" and is enabled by the deployment of the v23 fork, affecting
  new masternode registrations and service updates to basic BLS masternodes.
  * Operators must upgrade from legacy BLS scheme to basic BLS scheme before utilizing extended address capabilities

Additional Notes
----------------

* While the field `service` is deprecated (see dash#6665), its effective value can be obtained by querying
  `addresses['core_p2p'][0]`.

* If the masternode is eligible for extended addresses, operators may register non-IPv4 addresses, subject to validation
  and consensus rules.

Updated RPCs
------------

* The input field `platformP2PPort` has been renamed to `platformP2PAddrs`. In addition to numeric inputs (i.e. ports),
  the field can now accept a string (i.e. an addr:port pair) and arrays of strings (i.e. multiple addr:port pairs),
  subject to validation rules.

* The input field `platformHTTPPort` has been renamed to `platformHTTPSAddrs`. In addition to numeric inputs (i.e. ports),
  the field can now accept a string (i.e. an addr:port pair) and arrays of strings (i.e. multiple addr:port pairs),
  subject to validation rules.

* The field `addresses` will now also report on platform P2P and platform HTTPS endpoints as `addresses['platform_p2p']`
  and `addresses['platform_https']` respectively.
  * On payloads before extended addresses, if a masternode update affects `platformP2PPort` and/or `platformHTTPPort`
    but does not affect `netInfo`, `protx listdiff` does not contain enough information to report on the masternode's
    address and will report the changed port paired with the dummy address `255.255.255.255`.

    This does not affect `protx listdiff` queries where `netInfo` was updated or diffs relating to masternodes that
    have upgraded to extended addresses.

* If the masternode is eligible for extended addresses, `protx register{,_evo}` and `register_fund{,_evo}` will continue
  allowing `coreP2PAddrs` to be left blank, as long as `platformP2PAddrs` and `platformHTTPSAddrs` are _also_ left blank.
  * Attempting to populate any three address fields will make populating all fields mandatory.
  * This does not affect nodes ineligible for extended addresses (i.e. all nodes before fork activation or legacy BLS nodes)
    and they will have to continue specifying `platformP2PAddrs` and `platformHTTPSAddrs` even if they wish to keep
    `coreP2PAddrs` blank.

* If the masternode is eligible for extended addresses, `protx register{,_evo}` and `register_fund{,_evo}` will no longer
  default to the core P2P port if a port is not specified in the addr:port pair. All ports must be specified explicitly.
  * This does not affect nodes ineligible for extended addresses, continuing to default to the core P2P port if provided an
    addr without a port.

* `protx register{,_evo}` and `register_fund{,_evo}` will continue to allow specifying only the port number for `platformP2PAddrs`
  and `platformHTTPSAddrs`, pairing it with the address from the first `coreP2PAddrs` entry. This mirrors existing behavior.
  * This method of entry may not be available in future releases of Dash Core and operators are recommended to switch over to
    explicitly specifying (arrays of) addr:port strings for all address fields.

* When reporting on extended address payloads, `platformP2PPort` and `platformHTTPPort` will read the port value from
  `netInfo[PLATFORM_P2P][0]` and `netInfo[PLATFORM_HTTPS][0]` respectively as both fields are subsumed into `netInfo`.
  * If `netInfo` is blank (which is allowed by ProRegTx), `platformP2PPort` and `platformHTTPPort` will report `-1` to indicate
    that the port number cannot be determined.
  * `protx listdiff` will not report `platformP2PPort` or `platformHTTPPort` if the legacy fields were not updated (i.e.
    changes to `netInfo` will not translate into reporting). This is because `platformP2PPort` or `platformHTTPPort` have
    dedicated diff flags and post-consolidation, all changes are now affected by `netInfo`'s diff flag.

    To avoid the perception of changes to fields that not serialized by extended address payloads, data from `netInfo` will
    not be translated for this RPC call.
