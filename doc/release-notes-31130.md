P2P and network changes
-----------------------

Support for UPnP was dropped. If you want to open a port automatically, consider using the `-natpmp`
option instead, which uses PCP or NAT-PMP depending on router support.

Updated settings
------

- Setting `-upnp` will now return an error. Consider using `-natpmp` instead.
