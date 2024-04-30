# I2P support in Bitcoin Core

It is possible to run Bitcoin Core as an
[I2P (Invisible Internet Project)](https://en.wikipedia.org/wiki/I2P)
service and connect to such services.

This [glossary](https://geti2p.net/en/about/glossary) may be useful to get
started with I2P terminology.

## Run Bitcoin Core with an I2P router (proxy)

A running I2P router (proxy) is required with the [SAM](https://geti2p.net/en/docs/api/samv3)
application bridge enabled. The following routers are recommended for use with Bitcoin Core:

- [i2prouter (I2P Router)](https://geti2p.net), the official implementation in
  Java. The SAM bridge is not enabled by default; it must be started manually,
  or configured to start automatically, in the Clients page in the
  router console (`http://127.0.0.1:7657/configclients`) or in the `clients.config` file.
- [i2pd (I2P Daemon)](https://github.com/PurpleI2P/i2pd)
  ([documentation](https://i2pd.readthedocs.io/en/latest)), a lighter
  alternative in C++. It enables the SAM bridge by default.

Note the IP address and port the SAM proxy is listening to; usually, it is
`127.0.0.1:7656`.

Once an I2P router with SAM enabled is up and running, use the following Bitcoin
Core configuration options:

```
-i2psam=<ip:port>
     I2P SAM proxy to reach I2P peers and accept I2P connections (default:
     none)

-i2pacceptincoming
     Whether to accept inbound I2P connections (default: 1). Ignored if
     -i2psam is not set. Listening for inbound I2P connections is
     done through the SAM proxy, not by binding to a local address and
     port.
```

In a typical situation, this suffices:

```
bitcoind -i2psam=127.0.0.1:7656
```

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

Make automatic outbound connections only to I2P addresses. Inbound and manual
connections are not affected by this option. It can be specified multiple times
to allow multiple networks, e.g. onlynet=onion, onlynet=i2p.

I2P support was added to Bitcoin Core in version 22.0 and there may be fewer I2P
peers than Tor or IP ones. Therefore, using I2P alone without other networks may
make a node more susceptible to [Sybil
attacks](https://en.bitcoin.it/wiki/Weaknesses#Sybil_attack). You can use
`bitcoin-cli -addrinfo` to see the number of I2P addresses known to your node.

Another consideration with `onlynet=i2p` is that the initial blocks download
phase when syncing up a new node can be very slow. This phase can be sped up by
using other networks, for instance `onlynet=onion`, at the same time.

In general, a node can be run with both onion and I2P hidden services (or
any/all of IPv4/IPv6/onion/I2P/CJDNS), which can provide a potential fallback if
one of the networks has issues.

## Persistent vs transient I2P addresses

The first time Bitcoin Core connects to the I2P router, it automatically
generates a persistent I2P address and its corresponding private key by default,
unless `-i2pacceptincoming=0` is set.  The private key is saved in a file named
`i2p_private_key` in the Bitcoin Core data directory.  The persistent I2P
address is used for making outbound connections and accepting inbound
connections.

In the I2P network, the receiver of an inbound connection sees the address of
the initiator.  This is unlike the Tor network, where the recipient does not
know who is connecting to it.

If your node is configured by setting `-i2pacceptincoming=0` to not accept
inbound I2P connections, then it will use a random transient I2P address for
itself on each outbound connection to make it harder to discriminate,
fingerprint or analyze it based on its I2P address.

I2P addresses are designed to be long-lived.  Waiting for tunnels to be built
for every peer connection adds delay to connection setup time.  Therefore, I2P
listening should only be turned off if really needed.

## Fetching I2P-related information from Bitcoin Core

There are several ways to see your I2P address in Bitcoin Core if accepting
incoming I2P connections (`-i2pacceptincoming`):
- in the "Local addresses" output of CLI `-netinfo`
- in the "localaddresses" output of RPC `getnetworkinfo`
- in the debug log (grep for `AddLocal`; the I2P address ends in `.b32.i2p`)

To see which I2P peers your node is connected to, use `bitcoin-cli -netinfo 4`
or the `getpeerinfo` RPC (e.g. `bitcoin-cli getpeerinfo`).

You can use the `getnodeaddresses` RPC to fetch a number of I2P peers known to your node; run `bitcoin-cli help getnodeaddresses` for details.

## Compatibility

Bitcoin Core uses the [SAM v3.1](https://geti2p.net/en/docs/api/samv3) protocol
to connect to the I2P network. Any I2P router that supports it can be used.

## Ports in I2P and Bitcoin Core

One particularity of SAM v3.1 is that it does not support ports,
unlike newer versions of SAM (v3.2 and up) that do support them and default the
port numbers to 0. From the point of view of peers that use newer versions of
SAM or other protocols that support ports, a SAM v3.1 peer is connecting to them
on port 0, from source port 0.

To allow future upgrades to newer versions of SAM, Bitcoin Core sets its
listening port to 0 when listening for incoming I2P connections and advertises
its own I2P address with port 0. Furthermore, it will not attempt to connect to
I2P addresses with a non-zero port number because with SAM v3.1 the destination
port (`TO_PORT`) is always set to 0 and is not in the control of Bitcoin Core.

## Bandwidth

By default, your node shares bandwidth and transit tunnels with the I2P network
in order to increase your anonymity with cover traffic, help the I2P router used
by your node integrate optimally with the network, and give back to the network.
It's important that the nodes of a popular application like Bitcoin contribute
as much to the I2P network as they consume.

It is possible, though strongly discouraged, to change your I2P router
configuration to limit the amount of I2P traffic relayed by your node.

With `i2pd`, this can be done by adjusting the `bandwidth`, `share` and
`transittunnels` options in your `i2pd.conf` file. For example, to limit total
I2P traffic to 256KB/s and share 50% of this limit for a maximum of 20 transit
tunnels:

```
bandwidth = 256
share = 50

[limits]
transittunnels = 20
```

Similar bandwidth configuration options for the Java I2P router can be found in
`http://127.0.0.1:7657/config` under the "Bandwidth" tab.

Before doing this, please see the "Participating Traffic Considerations" section
in [Embedding I2P in your Application](https://geti2p.net/en/docs/applications/embedding).

In most cases, the default router settings should work fine.

## Bundling I2P in a Bitcoin application

Please see the "General Guidance for Developers" section in https://geti2p.net/en/docs/api/samv3
if you are developing a downstream application that may be bundling I2P with Bitcoin.
