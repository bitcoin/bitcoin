# I2P support in Bitcoin Core

It is possible to run Bitcoin Core as an
[I2P (Invisible Internet Project)](https://en.wikipedia.org/wiki/I2P)
service and connect to such services.

This [glossary](https://geti2p.net/en/about/glossary) may be useful to get
started with I2P terminology.

## Run Bitcoin Core with an I2P router (proxy)

A running I2P router (proxy) with [SAM](https://geti2p.net/en/docs/api/samv3)
enabled is required (there is an [official one](https://geti2p.net) and
[a few alternatives](https://en.wikipedia.org/wiki/I2P#Routers)). Notice the IP
address and port the SAM proxy is listening to; usually, it is
`127.0.0.1:7656`. Once it is up and running with SAM enabled, use the following
Bitcoin Core options:

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
bitcoind -i2psam=127.0.0.1:7656
```

The first time Bitcoin Core connects to the I2P router, its I2P address (and
corresponding private key) will be automatically generated and saved in a file
named `i2p_private_key` in the Bitcoin Core data directory.

## Additional configuration options related to I2P

```
-debug=i2p
```

Set the `debug=i2p` config logging option to see additional information in the
debug log about your I2P configuration and connections. Run `bitcoin-cli help
logging` for more information.

```
-onlynet=i2p
```

Make outgoing connections only to I2P addresses. Incoming connections are not
affected by this option. It can be specified multiple times to allow multiple
network types, e.g. onlynet=ipv4, onlynet=ipv6, onlynet=onion, onlynet=i2p.

Warning: if you use -onlynet with values other than onion, and the -onion or
-proxy option is set, then outgoing onion connections will still be made; use
-noonion or -onion=0 to disable outbound onion connections in this case.

I2P support was added to Bitcoin Core in version 22.0 and there may be fewer I2P
peers than Tor or IP ones. Therefore, using I2P alone without other networks may
make a node more susceptible to [Sybil
attacks](https://en.bitcoin.it/wiki/Weaknesses#Sybil_attack). You can use
`bitcoin-cli -addrinfo` to see the number of I2P addresses known to your node.

Another consideration with `onlynet=i2p` is that the initial blocks download
phase when syncing up a new node can be very slow. This phase can be sped up by
using other networks, for instance `onlynet=onion`, at the same time.

In general, a node can be run with both onion and I2P hidden services (or
any/all of IPv4/IPv6/onion/I2P), which can provide a potential fallback if one
of the networks has issues.

## I2P-related information in Bitcoin Core

There are several ways to see your I2P address in Bitcoin Core:
- in the debug log (grep for `AddLocal`, the I2P address ends in `.b32.i2p`)
- in the output of the `getnetworkinfo` RPC in the "localaddresses" section
- in the output of `bitcoin-cli -netinfo` peer connections dashboard

To see which I2P peers your node is connected to, use `bitcoin-cli -netinfo 4`
or the `getpeerinfo` RPC (e.g. `bitcoin-cli getpeerinfo`).

To see which I2P addresses your node knows, use the `getnodeaddresses 0 i2p`
RPC.

## Compatibility

Bitcoin Core uses the [SAM v3.1](https://geti2p.net/en/docs/api/samv3) protocol
to connect to the I2P network. Any I2P router that supports it can be used.

## Ports in I2P and Bitcoin Core

Bitcoin Core uses the [SAM v3.1](https://geti2p.net/en/docs/api/samv3)
protocol. One particularity of SAM v3.1 is that it does not support ports,
unlike newer versions of SAM (v3.2 and up) that do support them and default the
port numbers to 0. From the point of view of peers that use newer versions of
SAM or other protocols that support ports, a SAM v3.1 peer is connecting to them
on port 0, from source port 0.

To allow future upgrades to newer versions of SAM, Bitcoin Core sets its
listening port to 0 when listening for incoming I2P connections and advertises
its own I2P address with port 0. Furthermore, it will not attempt to connect to
I2P addresses with a non-zero port number because with SAM v3.1 the destination
port (`TO_PORT`) is always set to 0 and is not in the control of Bitcoin Core.
