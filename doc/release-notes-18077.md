P2P and network changes
-----------------------

- Added NAT-PMP port mapping support via [`libnatpmp`](https://miniupnp.tuxfamily.org/libnatpmp.html)

Command-line options
--------------------

- The `-natpmp` option has been added to use NAT-PMP to map the listening port. If both UPnP
and NAT-PMP are enabled, a successful allocation from UPnP prevails over one from NAT-PMP.
