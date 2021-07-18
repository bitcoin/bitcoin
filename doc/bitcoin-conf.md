# `bitcoin.conf` Configuration File

The configuration file is used by `bitcoind`, `bitcoin-qt` and `bitcoin-cli`.

All command-line options (except for `-?`, `-help`, `-version` and `-conf`) may be specified in a configuration file, and all configuration file options (except for `includeconf`) may also be specified on the command line. Command-line options override values set in the configuration file and configuration file options override values set in the GUI.

Changes to the configuration file while `bitcoind` or `bitcoin-qt` is running only take effect after restarting.

## Configuration File Format

The configuration file is a plain text file and consists of `option=value` entries, one per line. Leading and trailing whitespaces are removed.

In contrast to the command-line usage:
- an option must be specified without leading `-`;
- a value of the given option is mandatory; e.g., `testnet=1` (for chain selection options), `noconnect=1` (for negated options).

### Blank lines

Blank lines are allowed and ignored by the parser.

### Comments

A comment starts with a number sign (`#`) and extends to the end of the line. All comments are ignored by the parser.

Comments may appear in two ways:
- on their own on an otherwise empty line (_preferable_);
- after an `option=value` entry.

### Network specific options

Network specific options can be:
- placed into sections with headers `[main]` (not `[mainnet]`), `[test]` (not `[testnet]`), `[signet]` or `[regtest]`;
- prefixed with a chain name; e.g., `regtest.maxmempool=100`.

Network specific options take precedence over non-network specific options.
If multiple values for the same option are found with the same precedence, the
first one is generally chosen.

This means that given the following configuration, `regtest.rpcport` is set to `3000`:

```
regtest=1
rpcport=2000
regtest.rpcport=3000

[regtest]
rpcport=4000
```

## Configuration File Path

The configuration file is not automatically created; you can create it using your favorite text editor. By default, the configuration file name is `bitcoin.conf` and it is located in the Bitcoin data directory, but both the Bitcoin data directory and the configuration file path may be changed using the `-datadir` and `-conf` command-line options.

The `includeconf=<file>` option in the `bitcoin.conf` file can be used to include additional configuration files.

### Default configuration file locations

Operating System | Data Directory | Example Path
-- | -- | --
Windows | `%APPDATA%\Bitcoin\` | `C:\Users\username\AppData\Roaming\Bitcoin\bitcoin.conf`
Linux | `$HOME/.bitcoin/` | `/home/username/.bitcoin/bitcoin.conf`
macOS | `$HOME/Library/Application Support/Bitcoin/` | `/Users/username/Library/Application Support/Bitcoin/bitcoin.conf`

You can find an example bitcoin.conf file in [share/examples/bitcoin.conf](../share/examples/bitcoin.conf).
