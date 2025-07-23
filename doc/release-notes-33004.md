Updated settings
----------------

* The `-natpmp` option is now set to `1` by default. This means nodes with `-listen` enabled (the
  default) but running behind a firewall, such as a local network router, will be reachable if the
  firewall/router supports any of the `PCP` or `NAT-PMP` protocols.
