Consensus changes
-----------------

- A version 3 (extended addresses) ProRegTx or ProUpServTx must list an IPv4 address as its first
  `coreP2PAddrs` entry, and an EvoNode's first `platformP2PAddrs` and `platformHTTPSAddrs` entries must use
  that same IPv4 address. Further entries may use other addresses and network types (IPv6, Tor, I2P, CJDNS,
  and domains for Platform HTTPS). Violations are rejected with `bad-protx-netinfo-addr-type` or
  `bad-protx-netinfo-addr`. This applies until masternodes are allowed to operate without IPv4, which is
  not scheduled on mainnet. Testnet and devnets already allow it, so they are unaffected. Regtest follows
  mainnet unless `-testactivationheight=ipv4_optional@<height>` is set.
- On mainnet, the first entry of each address list must also use that list's standard port: 9999 for
  `coreP2PAddrs`, 26656 for `platformP2PAddrs` and 443 for `platformHTTPSAddrs`, as legacy ProTx versions
  already require there. Additional entries may use any otherwise valid port. Violations are rejected
  with `bad-protx-netinfo-port`.
