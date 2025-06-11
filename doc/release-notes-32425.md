Updated settings
----------------

- Previously `-proxy=addr:port` specified the proxy for all networks (except
  I2P) and only the Tor proxy could have been specified separately via
  `-onion=addr:port`. Now the syntax of `-proxy` has been extended and it is
  possible to specify separately the proxy for IPv4, IPv6, Tor and CJDNS by
  appending `=` followed by the network name, for example
  `-proxy=addr:port=ipv6` would configure a proxy only for IPv6. The `-proxy`
  option can be used multiple times to define different proxies for different
  networks, such as `-proxy=addr1:port1=ipv4 -proxy=addr2:port2=ipv6`. It is
  also possible to override an earlier setting, which can be used to remove
  an earlier all-networks proxy and use direct connections for a given network
  only, e.g. `-proxy=addr:port -proxy=0=cjdns`. (#32425)
