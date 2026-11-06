P2P and network changes
-----------------------

- IPv4 and IPv6 are now automatically treated as unreachable if the operating
  system has no route to the internet on them, so that no futile outbound
  connections are attempted. This is re-checked periodically. Detection is
  disabled if `-onlynet` is specified, which can be used to opt out. (#36784)
