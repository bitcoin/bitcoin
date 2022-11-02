New RPCs
---

- The `addpermissionflags` RPC adds permission flags to the peers
  using the given IP address (e.g. 1.2.3.4) or CIDR-notated
  network (e.g. 1.2.3.0/24). Uses the same permissions as -whitebind.
  Additional flags `in` and `out` control whether permissions apply
  to incoming and/or outgoing connections (default: incoming only). (#26441)