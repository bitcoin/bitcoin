Added RPCs
--------

The following RPCs were added: `protx register_hpmn`, `protx register_fund_hpmn`, `protx register_prepare_hpmn` and `protx update_service_hpmn`.
These HPMN RPCs correspond to the standard masternode RPCs but have the following additional mandatory arguments: `platformNodeID`, `platformP2PPort` and `platformHTTPPort`.
- `platformNodeID`: Platform P2P node ID, derived from P2P public key.
- `platformP2PPort`: TCP port of Dash Platform peer-to-peer communication between nodes (network byte order).
- `platformHTTPPort`: TCP port of Platform HTTP/API interface (network byte order).
Notes:
- `platformNodeID` must be unique across the network.
- `platformP2PPort`, `platformHTTPPort` and the Core port must be distinct.


Updated RPCs
--------

The RPC's `gobject getcurrentvotes` reply is enriched by adding the vote weight at the end of each line. Possible values are 1 or 4. Example:
`"7cb20c883c6093b8489f795b3ec0aad0d9c2c2821610ae9ed938baaf42fec66d": "277e6345359071410ab691c21a3a16f8f46c9229c2f8ec8f028c9a95c0f1c0e7-1:1670019339:yes:funding:4"`