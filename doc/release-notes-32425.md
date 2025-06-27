Updated settings
----------------

- Previously, `-proxy` specified the proxy for all networks (except I2P which
  uses `-i2psam`) and only the Tor proxy could have been specified separately
  via `-onion`. Now, the syntax of `-proxy` has been extended and it is possible
  to specify separately the proxy for IPv4, IPv6, Tor and CJDNS by appending `=`
  followed by the network name, for example `-proxy=127.0.0.1:5555=ipv6`
  configures a proxy only for IPv6. The `-proxy` option can be used multiple
  times to define different proxies for different networks, such as
  `-proxy=127.0.0.1:4444=ipv4 -proxy=10.0.0.1:6666=ipv6`. Later settings
  override earlier ones for the same network; this can be used to remove an
  earlier all-networks proxy and use direct connections only for a given
  network, for example `-proxy=127.0.0.1:5555 -proxy=0=cjdns`. (#32425)
