# v2 P2P Short ID Version Negotiation

The network protocol version has been updated to `70240` (`PLATFORMBAN_V2_SHORT_ID_VERSION`).

The `PLATFORMBAN` message has been added to the v2 P2P short ID mapping (short ID 168). When communicating with peers supporting version 70240+, this message uses 1-byte encoding instead of 13-byte encoding, reducing bandwidth.

The v2 transport layer now automatically negotiates short ID support per connection. Compatible peers use compact encoding, while older v2 peers automatically fall back to long encoding. This enables gradual rollout of new short IDs without breaking backwards compatibility.
