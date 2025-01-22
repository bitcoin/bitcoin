When Dash Core automatically opens outgoing P2P connections, it chooses
a peer (address and port) from its list of potential peers. This list is
populated with unchecked data gossiped over the P2P network by other peers.

A malicious actor may gossip an address:port where no Dash node is listening,
or one where a service is listening that is not related to the Dash network.
As a result, this service may occasionally get connection attempts from Dash
nodes.

"Bad" ports are ones used by services which are usually not open to the public
and usually require authentication. A connection attempt (by Dash Core,
trying to connect because it thinks there is a Dash node on that
address:port) to such service may be considered a malicious action by an
ultra-paranoid administrator. An example for such a port is 22 (ssh).

Additionally, ports below 1024 are classified as "system ports" by RFC 6335
and on some platforms, require administrative privileges in order to use them.
They are also considered "bad" ports as they require clients to either run Dash
Core with elevated privileges or configure their system to relax such requirements,
which may not be possible or desirable in some deployments.

Below is a list of "bad" ports which Dash Core avoids when choosing a peer to
connect to. If a node is listening on such a port, it will likely receive fewer
incoming connections.

    1719:  h323gatestat
    1720:  h323hostcall
    1723:  pptp
    2049:  nfs
    3659:  apple-sasl / PasswordServer
    4045:  lockd
    5060:  sip
    5061:  sips
    6000:  X11
    6566:  sane-port
    6665:  Alternate IRC
    6666:  Alternate IRC
    6667:  Standard IRC
    6668:  Alternate IRC
    6669:  Alternate IRC
    6697:  IRC + TLS
    8882:  Bitcoin RPC
    8883:  Bitcoin P2P
    10080: Amanda
    18882: Bitcoin testnet RPC
    18883: Bitcoin testnet P2P

For further information see:

[pull/23306](https://github.com/bitcoin/bitcoin/pull/23306#issuecomment-947516736)

[pull/23542](https://github.com/bitcoin/bitcoin/pull/23542)

[fetch.spec.whatwg.org](https://fetch.spec.whatwg.org/#port-blocking)

[chromium.googlesource.com](https://chromium.googlesource.com/chromium/src.git/+/refs/heads/main/net/base/port_util.cc)

[hg.mozilla.org](https://hg.mozilla.org/mozilla-central/file/tip/netwerk/base/nsIOService.cpp)

[RFC 6335, Section 6 ("Port Number Ranges")](https://datatracker.ietf.org/doc/html/rfc6335#section-6)
