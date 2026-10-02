# Private broadcast

With `bitcoind -privatebroadcast`, Bitcoin Core sends the transactions submitted through
`sendrawtransaction` to the network without revealing your IP address, onion address or node
identity. Each broadcast is a job: it finds peers through the release DNS seeds (resolved through
Tor), connects to a small, fixed number of them through Tor on a schedule drawn when the job starts,
announces the transaction, and stops. A job does not retry.

What a job guarantees, and what it does not, is specified in
[design/private-broadcast.md](design/private-broadcast.md).

## Tor proxy

Jobs reach everything through the node's Tor proxy, set with `-proxy`, `-onion`, or `-listenonion`
with `-torcontrol` (see [tor.md](tor.md)).

The proxy must accept SOCKS username/password authentication and Tor's RESOLVE extension. Without
IPv6 it loses the IPv6 peers. A proxy that is not Tor delivers nothing: every stream uses fresh
credentials, which `ssh -D` and most VPN clients do not accept, and IPv4 and IPv6 peers come only
from RESOLVE answers.

Keep Tor's default `IsolateSOCKSAuth` on that SocksPort. With
`NoIsolateSOCKSAuth`, Tor may put several of a job's streams on one circuit, so one exit could see
the transaction reach several peers, or put a job's stream on a circuit that also carries the node's
own connections. Nothing at the SOCKS interface can detect this.

Whoever answers on the proxy address receives your unpublished transaction: the node cannot tell
whether it is Tor. On a host shared with other users, use a `unix:` proxy socket in a directory
only the node's user can read, or a privileged port, and start Tor before the node.

## Usage

With `-privatebroadcast`, `sendrawtransaction` queues a job instead of adding the transaction to the
mempool and announcing it to the node's peers. The transaction enters the node's mempool only when
it comes back from the network, and is then relayed like any other. Wallet sends and `submitpackage`
are not affected: they add to the mempool and announce to the node's peers as before.

- `sendrawtransaction` test-accepts the transaction, then queues it. A transaction whose txid is
  already in the mempool counts as accepted and is sent as you gave it.
- Jobs start in submission order, 35 to 55 seconds (drawn at random) after the previous start. A
  job submitted after that time, with nothing waiting, starts at once.
- A transaction whose job is still queued or running is not queued again. The match is by wtxid, so
  a witness variant gets its own job. Once a job has ended, the same transaction can be queued again.
- A job runs until its schedule is over, at most ten minutes, and does not retry. A job whose DNS
  queries find no peer, for example while Tor is down, ends at once with the error `no candidates`.
  If `getprivatebroadcastinfo` shows a finished job with `announced` false, or the transaction does
  not arrive, submit it again.
- `getprivatebroadcastinfo` lists the queued and running jobs and the last 100 finished ones, with
  their reports, which include every peer dialled, to any RPC user allowed to call it. They are
  kept in memory only.
  `abortprivatebroadcast` removes a queued job or stops a running one.
- `setnetworkactive false` and shutdown cancel all jobs; turning networking back on does not restart
  them. Networking turned off for less than about half a second may go unnoticed, and the jobs then
  continue. At shutdown the log gives the number of jobs stopped; their transactions may need
  submitting again after restart. While networking is off, submissions are refused.
- Settings that choose the node's peers (`-onlynet`, `-dnsseed`, `-fixedseeds`, `-connect`,
  `-seednode`, `-addnode`) do not apply to jobs, except that `-privatebroadcast` is refused at
  startup when `-onlynet` leaves out onion or `-onion=0` turns it off. `-connect` turns off
  listening and with it `-listenonion`, so with `-connect` set `-proxy` or `-onion` for jobs to
  reach Tor. Even with `-onlynet=onion`, jobs connect to IPv4 and IPv6 peers through Tor exits.
  Jobs use fresh proxy credentials for each connection and each set of DNS queries, whatever
  `-proxyrandomize` says.
- On signet, `-privatebroadcast` is also refused with `-signetseednode` or `-signetchallenge`, since
  jobs query only a chain's own seeds and a custom signet has none. It is refused with `-mocktime`
  except on regtest, and on regtest, which has no DNS seeds, it needs `-privatebroadcastseed`.
- `debug.log` records job progress, with the txid, only with `-debug=privatebroadcast`, and SOCKS
  failures only with `-debug=proxy`. Either names a recipient's address only with `-logips` as
  well. By default it names no transaction or peer.
- Jobs need file descriptors beyond those of the node's ordinary connections. If the node refuses to
  start for lack of them, lower `-maxconnections`.

## Limits

- Every peer a job dials comes from the release's DNS seeds and is reached through a Tor exit. If
  the seeds cannot be reached through Tor, the job has no one to send to and ends at once.
- A peer can accept the announcement and then withhold the transaction, and the job does not try
  that slot again. Watch for the transaction and submit it again if it does not arrive.
- A job schedules its connections on the system clock. If the clock jumps forward while it runs,
  the connections it skips are not made, and it can end without announcing the transaction: check
  `announced` in `getprivatebroadcastinfo` and submit it again.
- Tor exits and DNS seeds are untrusted: an exit can act as the peer on its own connection, and a
  seed can lie about which peers exist. Each affects only its share of a job's connections.
- Peers can recognise a job by its fixed profile. They learn only that someone used private
  broadcast.
- Jobs share the node's host and Tor daemon, and the design does not separate them from the node's
  own traffic there.
- The wallet is out of scope. If a wallet loaded in your node recognises the transaction, then once
  the transaction is back in the node's mempool, the wallet's periodic resend (every 12 to 36 hours)
  announces it to all of the node's peers for as long as it stays unconfirmed, however it was first
  broadcast. To avoid that, run the node with `-walletbroadcast=0` or keep the wallet on another
  node.
- Stopping the node or running `setnetworkactive false` closes a job's open connections at nearly
  the same moment the node drops its own peers. A peer of your node that is also one of the job's
  recipients could link the two.
- Jobs queued together start 35 to 55 seconds apart. Seeds and recipients that together see
  several jobs start at that rhythm could guess they come from one node.
- Tor does not protect against an observer who watches both your connection to Tor and the Bitcoin
  network. The burst of circuits a job opens when it starts makes it easier to spot.
