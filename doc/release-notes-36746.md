RPC
---

- IPv6 addresses with an embedded IPv4 address, such as 6to4 (`2002::/16`)
  and Teredo (`2001::/32`), are now treated as `ipv6` instead of `ipv4` by
  `getnodeaddresses` and `getrawaddrman`. `getnodeaddresses` now returns them
  when filtering by `ipv6` rather than `ipv4`, and both RPCs report their
  `network` (and `source_network`) as `ipv6`. This is consistent with the
  per-network counts reported by `getaddrmaninfo`. `getpeerinfo` is unchanged
  and still reports `ipv4` for peers connected through such addresses. (#36746)
