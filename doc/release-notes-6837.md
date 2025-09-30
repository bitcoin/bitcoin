Statistics
----------

- IPv6 hosts are now supported by the StatsD client.

- `-statshost` now accepts URLs to allow specifying the protocol, host and port in one argument.

- Specifying invalid values will no longer result in silent disablement of the StatsD client and will now cause errors
  at startup.

### Deprecations

- `-statsport` has been deprecated and ports are now specified using the new URL syntax supported by `-statshost`.
  `-statsport` will be removed in a future release.

  - If both `-statsport` and `-statshost` with a URL specifying a port is supplied, the `-statsport` value will be
    ignored.
