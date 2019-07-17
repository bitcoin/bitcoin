Contents
========
This directory contains tools related to Signet, both for running a Signet yourself and for using one.

The `args.sh` script is a helper script used by the other scripts and should not be invoked directly.

getcoins.sh
===========

A script to call a faucet to get Signet coins.

Syntax: `getcoins.sh [--help] [--cmd=<bitcoin-cli path>] [--faucet=<faucet URL>] [--addr=<signet bech32 address>] [--password=<faucet password>] [--] [<bitcoin-cli args>]`

* `--cmd` lets you customize the bitcoin-cli path. By default it will look for it in the PATH, then in `../../src/`
* `--faucet` lets you specify which faucet to use; the faucet is assumed to be compatible with https://github.com/kallewoof/bitcoin-faucet
* `--addr` lets you specify a Signet address; by default, the address must be a bech32 address. This and `--cmd` above complement each other (i.e. you do not need `bitcoin-cli` if you use `--addr`)
* `--password` lets you specify a faucet password; this is handy if you are in a classroom and set up your own faucet for your students; (above faucet does not limit by IP when password is enabled)

If using the default network, invoking the script with no arguments should be sufficient under normal
circumstances, but if multiple people are behind the same IP address, the faucet will by default only
accept one claim per day. See `--password` above.

issuer.sh
=========

A script to regularly issue Signet blocks.

Syntax: `issuer.sh <idle time> [--help] [--cmd=<bitcoin-cli path>] [--] [<bitcoin-cli args>]`

* `<idle time>` is a time in seconds to wait between each block generation
* `--cmd` lets you customize the bitcoin-cli path. By default it will look for it in the PATH, then in `../../src/`

Signet, just like other bitcoin networks, uses proof of work alongside the block signature; this
includes the difficulty adjustment every 2016 blocks.
The `<idle time>` exists to allow you to maintain a relatively low difficulty over an extended period
of time. E.g. an idle time of 540 means your node will end up spending roughly 1 minute grinding
hashes for each block, and wait 9 minutes after every time.

mkblock.sh
==========

A script to generate one Signet block.

Syntax: `mkblock.sh <bitcoin-cli path> [<bitcoin-cli args>]`

This script is called by the other block issuing scripts, but can be invoked independently to generate
1 block immediately.

secondary.sh
============

A script to act as backup generator in case the primary issuer goes offline.

Syntax: `secondary.sh <trigger time> <idle time> [--cmd=<bitcoin-cli path>] [<bitcoin-cli args>]`

* `<trigger time>` is the time in seconds that must have passed since the last block was seen for the secondary issuer to kick into motion
* `<idle time>` is the time in seconds to wait after generating a block, and should preferably be the same as the idle time of the main issuer

Running a Signet network, it is recommended to have at least one secondary running in a different
place, so it doesn't go down together with the main issuer.
