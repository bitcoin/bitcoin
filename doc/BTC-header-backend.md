## BTC header backend

What Syscoin exposes that is tied to the **BTC header backend**; managed bitcoind vs external RPC and what applies to both modes.

- **Managed:** `-btcheadermanaged=1` + binary/datadir/ports/extra bitcoind args; Syscoin sets `-btcheadercmd` internally.
- **Unmanaged:** `-btcheadermanaged=0` + **required** `-btcheadercmd=…`; policy/watchdog flags still apply to RPC behavior and (for watchdog) only affect restarts when a managed process exists.

---

### Mode

| Setting | Default | Role |
|--------|---------|------|
| -btcheadermanaged | 1 (true) | 1: spawn and supervise local headers-only bitcoind; 0: no child process — use *-btcheadercmd* only. |

Managed BTC header node mode is supported on non-Windows builds only (i.e. Linux and macOS).

---

### Managed only (-btcheadermanaged=1)

These only affect the child bitcoind process and how Syscoin finds it. If you set *-btcheadercmd* yourself, it is **ignored** once the managed node starts (Syscoin forces *-btcheadercmd* to the internally built bitcoin-cli line).

| Setting | Default | Description |
|--------|---------|-------------|
| -btcheaderbinary=<path> | *(search paths)* | Path to bitcoind. |
| -btcheaderclibinary=<path> | *(next to bitcoind)* | Path to bitcoin-cli for RPC checks and *stop*. |
| -btcheaderdatadir=<dir> | <syscoin datadir>/btcheader | Child bitcoind datadir. |
| -btcheaderport=<port> | main 18544, test 19444, signet 20444, regtest 21444 | Child P2P *-port*. |
| -btcheaderrpcport=<port> | main 18543, test 19443, signet 20443, regtest 21443 | Child *-rpcport*. |
| -btcheadercommandline=<arg> | *(none)* | Extra args to child bitcoind; repeat the option for multiple args. |

**Always injected** by Syscoin for the child (you do not set these as Syscoin flags): `-headersonly=1`, `-server=1`, `-daemon=0`, `-datadir=…`, `-port=…`, `-rpcport=…`, plus `-testnet` / `-signet` / `-regtest` matching Syscoin’s chain. Watchdog may add `-reindex=1` on some restart paths.

**Not a config flag:** child stderr log is written to *<syscoin datadir>/btcheadernode.log*.

---

### Unmanaged (-btcheadermanaged=0)

| Setting | Required | Description |
|--------|----------|-------------|
| -btcheadercmd=<cmd> | **Yes** | Shell command prefix; Syscoin appends RPC method and args (same as bitcoin-cli usage). Must print JSON on stdout. |

Managed-only options do **not** start a node in this mode; put host/port/auth in `-btcheadercmd` (e.g. full `bitcoin-cli …` line).

---

### Both modes (BTCC signer policy / watchdog)

These tune **how Syscoin uses** the header RPCs (*getblockchaininfo*, *getblockheader*, *getblockhash*, *getchaintips*), not how the Bitcoin binary is launched — except where noted.

| Setting | Default | Description |
|--------|---------|-------------|
| -btcheaderpolicyondemand | 0 | Enforce BTC-header policy on mine-on-demand chains (e.g. regtest-style). |
| `-btcheaderwatchdog | 1 | Probe backend health; can restart **managed** node on stall/failure. |
| -btcheaderwatchdogprobeinterval=<n> | 15 | Seconds between probes. |
| -btcheaderwatchdogrestartcooldown=<n> | 60 | Min seconds between **managed** restart attempts. |
| -btcheaderwatchdogstalltimeout=<n> | 1800 | No header height progress during BTC IBD → consider stall (managed restart). |
| -btcheaderwatchdogreindexafter=<n> | 3 | After this many failed restarts, try one start with `-reindex=1` (0 disables). |
| -btcheadertipmaxage=<n> | 7200 (2 h) | Max BTC tip age (seconds) before signing pauses (0 disables). |
| -btcheadertipmaxnoprogress=<n> | 1800 | Max seconds without BTC tip height progress before signing pauses (0 disables). |
| -btcheadermaxlagblocks=<n> | 36 | Max lag (BTC blocks) between active tip and candidate BTCPREV (0 disables). |
| -btcheaderrecentforkdepth=<n> | 2 | Pause signing if a non-active tip is within this depth (0 disables). |

---

### Build / packaging (not runtime syscoind args)

- `./configure --enable-btcheadernode-build` — optional build of pinned Bitcoin Core + headers patch into `btcheadernode` layout (see *contrib/btcheadernode/README.md*).

---

### Not user-configurable (code constants)

Defined in *src/validation.h* / *quorums_btccheckpoints.cpp*: e.g. default P2P/RPC port **defaults** above, `BTCCHECK_PERIOD`, sign offsets, and `DEFAULT_BTC_HEADER_MIN_CONFIRMATIONS` (1) used in policy — **no** `-btcheader…` switch for that last one.

---

**NOTE:**
Required conditions for BTC header node to activate are:
Masternode mode (masternodeblsprivkey=...) or NEVM miner mode (NEVM is active, gethcommandline includes --miner.etherbase or --miner.pendingfeerecipient).
The chain must be “policy active” (mainnet, testnet, signet by default)
(regtest: mine-blocks-on-demand is true, needs btcheaderpolicyondemand=1).
