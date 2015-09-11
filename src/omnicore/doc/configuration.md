Configuration
=============

Omni Core can be configured by providing one or more optional command-line arguments:
```bash
$ omnicored -setting=value -setting=value
```

All settings can alternatively also be configured via the `bitcoin.conf`.

Depending on the operating system, the default locations for the configuration file are:

- Unix systems: `$HOME/.bitcoin/bitcoin.conf`
- Mac OS X: `$HOME/Library/Application Support/Bitcoin/bitcoin.conf`
- Microsoft Windows: `%APPDATA%/Bitcoin/bitcoin.conf`

A typical `bitcoin.conf` may include:
```
server=1
rpcuser=omnicorerpc
rpcpassword=5hMTZI9iBGFqKxsWfOUF
rpcallowip=127.0.0.1
rpcport=8332
txindex=1
datacarriersize=80
logtimestamps=1
omnidebug=tally
omnidebug=packets
omnidebug=pending
```

## Optional settings

The following Omni Core specific settings are available:

##### General options:

- `-startclean` (boolean, clear persistence files, disabled by default)
- `-omnitxcache=n` (number, max. size of input transaction cache, `500000` transactions by default)
- `-omniprogressfrequency` (number, time in seconds after which the initial scanning progress is reported, 30 seconds by default)

##### Log options:

- `-omnilogfile=file` (string, log file, `omnicore.log` by default)
- `-omnidebug=category` (multi string, enable log categories, can be `all`, `none`)

##### Transaction options:

- `-autocommit` (boolean, create or broadcast transactions, enabled by default)
- `-datacarrier` and `-datacarriersize` determine whether to use class B (multisig) or class C encoding

##### RPC server options:

- `-rpcforceutf8` (boolean, replace invalid UTF-8 encoded characters with question marks in RPC response, enabled by default)

##### Alert and activation options:

- `-overrideforcedshutdown` (boolean, overwrite shutdown, triggered by an alert, disabled by default)
- `-omnialertallowsender=source` (multi string, authorize alert senders, can be `any`)
- `-omnialertignoresender=source` (multi string, ignore alert senders)
- `-omniactivationallowsender=source` (multi string, authorize activation senders, can be `any`)
- `-omniactivationignoresender=source` (multi string, ignore activation senders)

##### User interface options:

 - `-disclaimer` (boolean, explicitly show QT disclaimer on startup, disabled by default)
 - `-omniuiwalletscope=n` (number, max. transactions to show in history, `65535` by default)

More information about the configuration and Bitcoin Core specific options are available in the [Bitcoin wiki](https://en.bitcoin.it/wiki/Running_Bitcoin).
