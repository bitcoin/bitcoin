# Support for signing transactions outside of Bitcoin Core

Bitcoin Core can be launched with `-signer=<cmd>` where `<cmd>` is an external tool which can sign transactions and perform other functions. For example, it can be used to communicate with a hardware wallet.

Interaction with external signer is built upon a specialized construct [Partially Signed Bitcoin Transaction (PSBT)](psbt.md).

## Example usage

The following example is based on the [HWI](https://github.com/bitcoin-core/HWI) tool. Version 2.0 or newer is required. Although this tool is hosted under the Bitcoin Core GitHub organization and maintained by Bitcoin Core developers, it should be used with caution. It is considered experimental and has far less review than Bitcoin Core itself. Be particularly careful when running tools such as these on a computer with private keys on it.

When using a hardware wallet, consult the manufacturer website for (alternative) software they recommend. As long as their software conforms to the standard below, it should be able to work with Bitcoin Core.

Start Bitcoin Core:

```sh
$ bitcoind -signer=../HWI/hwi.py
```

`bitcoin node` can also be substituted for `bitcoind`.

### Device setup

Follow the hardware manufacturers instructions for the initial device setup, as well as their instructions for creating a backup. Alternatively, for some devices, you can use the `setup`, `restore` and `backup` commands provided by [HWI](https://github.com/bitcoin-core/HWI).

### Create wallet and import keys

Get a list of signing devices / services:

```
$ bitcoin-cli enumeratesigners
{
  "signers": [
    {
      "fingerprint": "c8df832a",
      "name": "trezor_t"
    }
  ]
}
```

The master key fingerprint is used to identify a device.

Create a wallet, this automatically imports the public keys:

```sh
$ bitcoin-cli rpc createwallet wallet_name="hww" disable_private_keys=true external_signer=true
```

You can also execute these commands using `bitcoin-qt` Debug Console instead of using `bitcoin-cli`.

### Verify an address

Display an address on the device:

```sh
$ bitcoin-cli -rpcwallet=<wallet> getnewaddress
$ bitcoin-cli -rpcwallet=<wallet> walletdisplayaddress <address>
```

Replace `<address>` with the result of `getnewaddress`.

### Spending

Under the hood this uses a [PSBT (Partially Signed Bitcoin Transaction)](psbt.md).

```sh
$ bitcoin-cli -rpcwallet=<wallet> sendtoaddress <address> <amount>
```

This constructs a PSBT and prompts your external signer to sign (will fail if it's not connected). If successful, Bitcoin Core finalizes and broadcasts the transaction.

```sh
{"complete": true, "txid": "<txid>"}
```

## Signer API

More recent versions of Bitcoin, around v29 and later, have made significant improvements. It is recommended to use more recent versions.

In order to be compatible with Bitcoin Core, any signer command should conform to the specification below. This specification is subject to change. Ideally a BIP should propose a standard so that other wallets can also make use of it.

Prerequisite knowledge:
* [Output Descriptors](descriptors.md)
* Partially Signed Bitcoin Transaction ([PSBT](psbt.md))

### Flag `--chain <name>` (required)

With `<name>` one of `main`, `test`, `signet`, `regtest`.

### Flag `--stdin` (required)

Indicate that (sub)command should be received over stdin and results returned in response to that. `--stdin` is a global flag, it is not used for all subcommands.

All subcommands SHOULD support both
- being called as commandline arguments; or
- being written to _external-signer_ process directly through stdin (with `--stdin`).

Usage:
```
$ <cmd> --stdin …
[…command and arguments written to stdin…]
```

Note: remember that shell-expansion is not available on _stdin_. Consequently, commands such as `signtx`, may write their arguments in either quoted or unquoted form.

### Flag `--fingerprint <fingerprint>` (required)

With `<fingerprint>` being the hexadecimal 8-symbol identifier for a wallet.

Commands will specify a fingerprint as an identifier for external-signer wallet.

### `enumerate` (required)

Usage:
```
$ <cmd> enumerate
[
    {
        "fingerprint": "00000000",
        "name": "trezor_t"
    }
]
```

The command MUST return an (empty) array with at least a `fingerprint` field.

A future extension could add an optional return field with device capabilities. Perhaps a descriptor with wildcards. For example: `["pkh("44'/0'/$'/{0,1}/*"), sh(wpkh("49'/0'/$'/{0,1}/*")), wpkh("84'/0'/$'/{0,1}/*")]`. This would indicate the device supports legacy, wrapped SegWit and native SegWit. In addition it restricts the derivation paths that can used for those, to maintain compatibility with other wallet software. It also indicates the device, or the driver, doesn't support multisig.

A future extension could add an optional return field `reachable`, in case `<cmd>` knows a signer exists but can't currently reach it.

### `signtx` (required)

`signtx` indicates a PSBT-signing-request, followed by Base64-encoded PSBT-content. Quotes are optional.

This command reads request `signtx <base64-encoded psbt>` from stdin and MUST return a JSON object. On success, it MUST contain `{"psbt": "<base64-encoded psbt>"}` updated to include any new signatures. On failure, it SHOULD contain `{"error": "<message>"}`. Presence of a key `error` with value `null` is not considered an error.

PSBT-content SHOULD include BIP32-derivations. The command SHOULD fail if none of the BIP32-derivations match a key owned by the device.

The command SHOULD fail if the user cancels.

The command MAY complain if `--chain` is set for `test` or `regtest`, but any of the BIP32 derivation paths contain a coin type other than `1h` (and vice versa).

### `getdescriptors` (optional)

Usage:

```
$ <cmd> --fingerprint=<fingerprint> getdescriptors <account>
<xpub>
```

Returns descriptors supported by the device. Example:

```
$ <cmd> --fingerprint=00000000 getdescriptors
{
  "receive": [
    "pkh([00000000/44h/0h/0h]xpub6C.../0/*)#fn95jwmg",
    "sh(wpkh([00000000/49h/0h/0h]xpub6B..../0/*))#j4r9hntt",
    "wpkh([00000000/84h/0h/0h]xpub6C.../0/*)#qw72dxa9"
  ],
  "internal": [
    "pkh([00000000/44h/0h/0h]xpub6C.../1/*)#c8q40mts",
    "sh(wpkh([00000000/49h/0h/0h]xpub6B..../1/*))#85dn0v75",
    "wpkh([00000000/84h/0h/0h]xpub6C..../1/*)#36mtsnda"
  ]
}
```

### `displayaddress` (optional)

Usage:
```
<cmd> --fingerprint=<fingerprint> displayaddress --desc <descriptor>
```

Example, display the first native SegWit receive address on Testnet:

```
<cmd> --chain test --fingerprint=00000000 displayaddress --desc "wpkh([00000000/84h/1h/0h]tpubDDUZ..../0/0)"
```

The command MUST be able to figure out the address type from the descriptor.

The command MUST return an object containing `{"address": "[the address]"}`.
As a sanity check, for devices that support this, it SHOULD ask the device to derive the address.

If `<descriptor>` contains a master key fingerprint, the command MUST fail if it does not match the fingerprint known by the device.

If `<descriptor>` contains an xpub, the command MUST fail if it does not match the xpub known by the device.

The command MAY complain if `--chain` is set to a test-network, but the BIP32 coin-type is not `1h` (and vice versa).

## How Bitcoin Core uses the Signer API

The `enumeratesigners` RPC simply calls `<cmd> enumerate`.

The `createwallet` RPC calls:

* `<cmd> --chain <name> --fingerprint=00000000 getdescriptors --account 0`

It then imports descriptors for all supported address types, in a BIP44/49/84/86 compatible manner.

The `walletdisplayaddress` RPC reuses some code from `getaddressinfo` on the provided address and obtains the inferred descriptor. It then calls `<cmd> --fingerprint=00000000 displayaddress --desc=<descriptor>`.

For external-signer wallets, spending uses `send` or `sendall`. Bitcoin Core builds a PSBT, calls the signer via stdin with `signtx`, and if signatures are sufficient, finalizes and broadcasts the transaction. If the signer is not connected or cancels, the call fails with an error. For fee-bumping on such wallets, use `psbtbumpfee` to involve an external signer.

`sendtoaddress` and `sendmany` check `inputs->bip32_derivs` to see if any inputs have the same `master_fingerprint` as the signer. If so, it calls `<cmd> --fingerprint=00000000 signtransaction <psbt>`. It waits for the device to return a (partially) signed psbt, tries to finalize it and broadcasts the transaction.
