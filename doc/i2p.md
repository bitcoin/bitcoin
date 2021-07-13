# I2P support in Syscoin Core

It is possible to run Syscoin Core as an
[I2P (Invisible Internet Project)](https://en.wikipedia.org/wiki/I2P)
service and connect to such services.

This [glossary](https://geti2p.net/en/about/glossary) may be useful to get
started with I2P terminology.

## Run Syscoin Core with an I2P router (proxy)

A running I2P router (proxy) with [SAM](https://geti2p.net/en/docs/api/samv3)
enabled is required (there is an [official one](https://geti2p.net) and
[a few alternatives](https://en.wikipedia.org/wiki/I2P#Routers)). Notice the IP
address and port the SAM proxy is listening to; usually, it is
`127.0.0.1:7656`. Once it is up and running with SAM enabled, use the following
Syscoin Core options:

```
-i2psam=<ip:port>
     I2P SAM proxy to reach I2P peers and accept I2P connections (default:
     none)

-i2pacceptincoming
     If set and -i2psam is also set then incoming I2P connections are
     accepted via the SAM proxy. If this is not set but -i2psam is set
     then only outgoing connections will be made to the I2P network.
     Ignored if -i2psam is not set. Listening for incoming I2P
     connections is done through the SAM proxy, not by binding to a
     local address and port (default: 1)
```

In a typical situation, this suffices:

```
syscoind -i2psam=127.0.0.1:7656
```

The first time Syscoin Core connects to the I2P router, its I2P address (and
corresponding private key) will be automatically generated and saved in a file
named `i2p_private_key` in the Syscoin Core data directory.

## Additional configuration options related to I2P

You may set the `debug=i2p` config logging option to have additional
information in the debug log about your I2P configuration and connections. Run
`syscoin-cli help logging` for more information.

It is possible to restrict outgoing connections in the usual way with
`onlynet=i2p`. I2P support was added to Syscoin Core in version 22.0 (mid 2021)
and there may be fewer I2P peers than Tor or IP ones. Therefore, using
`onlynet=i2p` alone (without other `onlynet=`) may make a node more susceptible
to [Sybil attacks](https://en.syscoin.it/wiki/Weaknesses#Sybil_attack). Use
`syscoin-cli -addrinfo` to see the number of I2P addresses known to your node.

## I2P related information in Syscoin Core

There are several ways to see your I2P address in Syscoin Core:
- in the debug log (grep for `AddLocal`, the I2P address ends in `.b32.i2p`)
- in the output of the `getnetworkinfo` RPC in the "localaddresses" section
- in the output of `syscoin-cli -netinfo` peer connections dashboard

To see which I2P peers your node is connected to, use `syscoin-cli -netinfo 4`
or the `getpeerinfo` RPC (e.g. `syscoin-cli getpeerinfo`).

To see which I2P addresses your node knows, use the `getnodeaddresses 0 i2p`
RPC.

## Compatibility

Syscoin Core uses the [SAM v3.1](https://geti2p.net/en/docs/api/samv3) protocol
to connect to the I2P network. Any I2P router that supports it can be used.

## Ports in I2P and Syscoin Core

Syscoin Core uses the [SAM v3.1](https://geti2p.net/en/docs/api/samv3)
protocol. One particularity of SAM v3.1 is that it does not support ports,
unlike newer versions of SAM (v3.2 and up) that do support them and default the
port numbers to 0. From the point of view of peers that use newer versions of
SAM or other protocols that support ports, a SAM v3.1 peer is connecting to them
on port 0, from source port 0.

To allow future upgrades to newer versions of SAM, Syscoin Core sets its
listening port to 0 when listening for incoming I2P connections and advertises
its own I2P address with port 0. Furthermore, it will not attempt to connect to
I2P addresses with a non-zero port number because with SAM v3.1 the destination
port (`TO_PORT`) is always set to 0 and is not in the control of Syscoin Core.
