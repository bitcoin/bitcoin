# Private broadcast

Bitcoin Core can send a transaction to the network without revealing your IP address, onion address
or node identity with the standalone `bitcoin-privbcast` program. Each broadcast is a job: it finds
peers through the release DNS seeds (resolved through Tor), connects to a small, fixed number of
them through Tor on a schedule drawn when the job starts, announces the transaction, and stops. A
job does not retry.

What a job guarantees, and what it does not, is specified in
[design/private-broadcast-tool.md](design/private-broadcast-tool.md).

## Tor proxy

Jobs reach everything through a Tor SOCKS5 proxy. `bitcoin-privbcast` expects Tor at
`127.0.0.1:9050`. Use `-tor=<ip:port|path>` for another loopback address or a unix socket, given as
`unix:<path>` as for `-onion`; a remote proxy is refused, because SOCKS5 carries destinations and
credentials in plaintext.

The proxy must accept SOCKS username/password authentication and Tor's RESOLVE extension. Without
IPv6 it loses the IPv6 peers. A proxy that is not Tor delivers nothing: every stream uses fresh
credentials, which `ssh -D` and most VPN clients do not accept, and IPv4 and IPv6 peers come only
from RESOLVE answers.

If the node and the jobs share a SocksPort, keep Tor's default `IsolateSOCKSAuth`. With
`NoIsolateSOCKSAuth`, Tor may put several of a job's streams on one circuit, so one exit could see
the transaction reach several peers, or put a job's stream on a circuit that also carries the node's
own connections. Nothing at the SOCKS interface can detect this.

## bitcoin-privbcast

1. Check the transaction with `bitcoin-cli testmempoolaccept`. The tool only checks that the
   transaction is well formed, standard in size, and burns nothing to an unspendable output
   (`-maxburnamount` raises that limit).
2. Feed the final hex on stdin, choosing the chain as with other tools (`-signet`, `-testnet4`, ...;
   a custom signet is refused, since it has no seeds):
   ```
   bitcoin-privbcast send < tx.hex
   ```
   The JSON report goes to stdout and progress lines to stderr.
3. Watch for the transaction with `bitcoin-cli getmempoolentry <txid>`. Do not also broadcast it the
   ordinary way. If it does not arrive, run another job.

`send` exits with status 0 if the announcement was fully sent to at least one peer, even if the job
failed afterwards; 1 on bad input or arguments, or if the job failed before that; and 2 if the job
ran without failing but sent the announcement in full to no peer. Status 0 does not mean that a peer
requested, received or accepted the transaction: the report's per-attempt `outcome` and its
`tx_written` and `pongs` counts say more. The report and the progress lines name the transaction and
every peer dialled, so keep them where you would keep a wallet log. Progress lines carry timestamps,
like `debug.log`, while the report gives times as offsets from the job's start. Progress lines are
best effort: on a pipe or a file, a line that stderr cannot take at once is dropped, while a paused
terminal holds the job up until the line is written.

`bitcoin-privbcast discover` runs only the peer discovery and prints the candidates it found. It
makes the same queries as a job.

## Limits

- Every peer a job dials comes from the release's DNS seeds and is reached through a Tor exit. If
  the seeds cannot be reached through Tor, the job has no one to send to.
- A peer can accept the announcement and then withhold the transaction, and the job does not try
  that slot again. Watch for the transaction and submit it again if it does not arrive.
- Tor exits and DNS seeds are untrusted: an exit can act as the peer on its own connection, and a
  seed can lie about which peers exist. Each affects only its share of a job's connections.
- Peers can recognise the tool by its fixed profile. They learn only that someone used it.
- The tool and the node usually share a host and a Tor daemon, and the design does not separate
  them.
- The wallet is out of scope. If a wallet loaded in your node recognises the transaction, then once
  the transaction is back in the node's mempool, the wallet's periodic resend (every 12 to 36 hours)
  announces it to all of the node's peers for as long as it stays unconfirmed, however it was first
  broadcast. To avoid that, run the node with `-walletbroadcast=0` or keep the wallet on another
  node.
- Tor does not protect against an observer who watches both your connection to Tor and the Bitcoin
  network. The burst of circuits a job opens when it starts makes it easier to spot.
