Contents
========
This directory contains tools related to Signet, both for running a Signet yourself and for using one.

getcoins.py
===========

A script to call a faucet to get Signet coins.

Syntax: `getcoins.py [-h|--help] [-c|--cmd=<bitcoin-cli path>] [-f|--faucet=<faucet URL>] [-a|--addr=<signet bech32 address>] [-p|--password=<faucet password>] [--] [<bitcoin-cli args>]`

* `--cmd` lets you customize the bitcoin-cli path. By default it will look for it in the PATH
* `--faucet` lets you specify which faucet to use; the faucet is assumed to be compatible with https://github.com/kallewoof/bitcoin-faucet
* `--addr` lets you specify a Signet address; by default, the address must be a bech32 address. This and `--cmd` above complement each other (i.e. you do not need `bitcoin-cli` if you use `--addr`)
* `--password` lets you specify a faucet password; this is handy if you are in a classroom and set up your own faucet for your students; (above faucet does not limit by IP when password is enabled)

If using the default network, invoking the script with no arguments should be sufficient under normal
circumstances, but if multiple people are behind the same IP address, the faucet will by default only
accept one claim per day. See `--password` above.
