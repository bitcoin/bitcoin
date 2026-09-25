# Privacy in Bitcoin Core

## Transaction broadcasting privacy

By default, when Bitcoin Core submits a transaction to the network it announces
it to all connected peers. This allows any peer to correlate the transaction
with the announcing node's IP address and identify it as the likely origin.

### Private broadcast (`-privatebroadcast`)

Transactions submitted via the `sendrawtransaction` RPC can be broadcast
privately through a short-lived Tor or I2P connection, without putting them
in the local mempool first. The transaction reaches the wider network only
once a recipient relays it, which prevents linking the submission to the
node's IP address.

To enable:

```
-privatebroadcast=1
```

This option is disabled by default.

**Prerequisites:** Tor (via `-proxy` or `-onion`) or I2P (via `-i2psam`) must
be configured and reachable. See [doc/tor.md](tor.md) and [doc/i2p.md](i2p.md)
for setup instructions. If neither network is reachable when a transaction is
submitted, `sendrawtransaction` returns an error.

**Scope:** Only `sendrawtransaction` is affected. Wallet RPCs (`sendtoaddress`,
`send`, `sendmany`, `sendall`, etc.) broadcast transactions through the standard method
regardless of this setting. Wallet-level private broadcast is not yet supported;
see the [private broadcast tracking issue](https://github.com/bitcoin/bitcoin/issues/34476)
for planned extensions.

**Related RPCs:**

- `getprivatebroadcastinfo` — returns transactions currently in the private
  broadcast queue with per-peer send timestamps and acknowledgment times where available.
- `abortprivatebroadcast <id>` — removes a transaction from the private
  broadcast queue by txid or wtxid.

Both RPCs are only available when `-privatebroadcast=1` is set.

## Network connectivity privacy

### Node identity across multiple networks

Operating a node that listens on multiple networks simultaneously (e.g. IPv4
and Tor, or IPv4 and I2P) can help strengthen the Bitcoin network, as nodes in
this configuration (i.e. bridge nodes) increase the cost and complexity of
launching eclipse and partition attacks. However, under certain conditions, an
adversary that can connect to your node on multiple networks may be able to
correlate those identities by observing shared runtime characteristics. It is
not recommended to expose your node over multiple networks if you require
unlinkability across those identities.

### Onion service port isolation

Do not add anything but Bitcoin Core ports to the onion service created in
[doc/tor.md § 3](tor.md#3-manually-create-a-bitcoin-core-onion-service). If
you run a web service too, create a new onion service for that. Otherwise it is
trivial to link them, which may reduce privacy. Onion services created
automatically (as in [§ 2](tor.md#2-automatically-create-a-bitcoin-core-onion-service))
always have only one port open.

## See also

- [doc/tor.md](tor.md) — Tor proxy setup, onion service creation and configuration
- [doc/i2p.md](i2p.md) — I2P setup, configuration, and address types
- [doc/cjdns.md](cjdns.md) — CJDNS setup (note: CJDNS encrypts traffic but
  does not hide the sender or recipient from intermediate routers)
