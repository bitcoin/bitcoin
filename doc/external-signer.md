# Support for signing transactions outside of Bitcoin Core

Bitcoin Core can be launched with `-signer=<cmd>` where `<cmd>` is an external tool which can sign transactions and perform other functions. For example, it can be used to communicate with a hardware wallet.

Interaction with external signer uses [Partially Signed Bitcoin Transaction (PSBT)](psbt.md).

## Example usage

The following example is based on the [HWI](https://github.com/bitcoin-core/HWI) tool. Version 2.0 or newer is required. Although this tool is hosted under the Bitcoin Core GitHub organization and maintained by Bitcoin Core developers, it should be used with caution. It is considered experimental and has far less review than Bitcoin Core itself. Be particularly careful when running tools such as these on a computer with private keys on it.

When using a hardware wallet, consult the manufacturer website for (alternative) software they recommend. As long as their software conforms to the standard below, it should be able to work with Bitcoin Core.

Start Bitcoin Core:

```sh
bitcoind -signer=../HWI/hwi.py
```

`bitcoin node` can also be substituted for `bitcoind`.

### Device setup

Follow the hardware manufacturers instructions for the initial device setup, as well as their instructions for creating a backup. Alternatively, for some devices, you can use the `setup`, `restore` and `backup` commands provided by [HWI](https://github.com/bitcoin-core/HWI).

### Create wallet and import keys

Get a list of signing devices / services:

```sh
bitcoin-cli enumeratesigners
```

```
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
bitcoin rpc createwallet wallet_name="hww2" disable_private_keys=true descriptors=true external_signer=true
```

Creation of the external wallet can be confirmed with `getwalletinfo`, which will report `"external_signer": true`. These commands can also be executed using `bitcoin-qt` Debug Console instead of using `bitcoin rpc` or `bitcoin-cli`.

### Verify an address

Display an address on the device:

```sh
bitcoin-cli -rpcwallet=<walletname> getnewaddress
bitcoin-cli -rpcwallet=<walletname> walletdisplayaddress <address>
```

Replace `<address>` with the result of `getnewaddress`.

### Spending

Under the hood this uses a [PSBT (Partially Signed Bitcoin Transaction)](psbt.md).

```sh
bitcoin rpc -rpcwallet=<walletname> send outputs='{"<address>": <amount>}'
```

This constructs a PSBT and prompts your external signer to sign (will fail if it's not connected). If successful, Bitcoin Core finalizes and broadcasts the transaction.

```
{"complete": true, "txid": "<txid>"}
```

## Signer API

In order to be compatible with Bitcoin Core, any signer command should conform to the specification below. This specification is subject to change. Ideally a BIP should propose a standard so that other wallets can also make use of it.

Prerequisite knowledge:
* [Output Descriptors](descriptors.md)
* Partially Signed Bitcoin Transaction ([PSBT](psbt.md))

### Flag `--chain <name>` (required except for `enumerate`)

With `<name>` one of `main`, `test`, `signet`, `regtest`, `testnet4`.

Bitcoin Core passes this flag to commands that operate on a chain-specific signer, for example `getdescriptors`, `displayaddress` and `signtx`.

### Flag `--stdin` (required)

Indicate that (sub)command should be received over stdin and results returned in response to that. `--stdin` is a global flag, it is not specific to any subcommand.

Bitcoin Core currently uses this flag for `signtx`.

All subcommands SHOULD support both
- being called as commandline arguments; or
- being written to _external-signer_ process directly through stdin (with `--stdin`).

Usage:
```sh
<cmd> --stdin …
```

```
[…command and arguments written to stdin…]
```

Note: remember that shell-expansion is not available on _stdin_. Consequently, commands such as `signtx`, may write their arguments in either quoted or unquoted form.

### Flag `--fingerprint <fingerprint>` (required except for `enumerate`)

With `<fingerprint>` being the hexadecimal 8-symbol identifier for a wallet.

Bitcoin Core passes this flag to commands that operate on a specific external-signer wallet, for example `getdescriptors`, `displayaddress` and `signtx`.

### `enumerate` (required)

Usage:

```sh
<cmd> enumerate
```

```
[
    {
        "fingerprint": "00000000",
        "model": "trezor_t"
    }
]
```

The command MUST return an array, possibly empty, of entries that contain at least a `fingerprint` field.

If present, the optional `model` field is used as the device name in the `enumeratesigners` RPC result.

A future extension could add an optional return field with device capabilities. Perhaps a descriptor with wildcards. For example: `["pkh("44'/0'/$'/{0,1}/*"), sh(wpkh("49'/0'/$'/{0,1}/*")), wpkh("84'/0'/$'/{0,1}/*")]`. This would indicate the device supports legacy, wrapped SegWit and native SegWit. In addition it restricts the derivation paths that can used for those, to maintain compatibility with other wallet software. It also indicates the device, or the driver, doesn't support multisig.

A future extension could add an optional return field `reachable`, in case `<cmd>` knows a signer exists but can't currently reach it.

### `signtx` (required)

`signtx` indicates a PSBT signing-request, followed by Base64-encoded PSBT. Quotes are optional.

This command reads request `<base64-encoded psbt>` and MUST return a JSON object. On success, it MUST contain `{"psbt": "<base64-encoded psbt>"}` updated to include any new signatures. On failure, it SHOULD contain `{"error": "<message>"}`. Presence of a key `error` with value `null` is not considered an error.

PSBT SHOULD include BIP32 derivations. The command SHOULD fail if none of the BIP32 derivations match a key owned by the device.

The command SHOULD fail if the user cancels.

The command MAY complain if `--chain` is set to a test-network, but any of the BIP32 derivation paths contain a coin type other than `1h` (and vice versa).

### `getdescriptors` (optional)

Usage:

```sh
<cmd> --fingerprint <fingerprint> --chain <name> getdescriptors --account <account>
```

Returns descriptors supported by the device. Example:

```sh
<cmd> --fingerprint 00000000 --chain main getdescriptors --account 0
```

```
{
  "receive": [
    "pkh([00000000/44h/0h/0h]xpub6C.../0/*)#fn95jwmg",
    "sh(wpkh([00000000/49h/0h/0h]xpub6B..../0/*))#j4r9hntt",
    "wpkh([00000000/84h/0h/0h]xpub6C.../0/*)#qw72dxa9",
    "tr([00000000/86h/0h/0h]xpub6C.../0/*)#4d8tq2ns"
  ],
  "internal": [
    "pkh([00000000/44h/0h/0h]xpub6C.../1/*)#c8q40mts",
    "sh(wpkh([00000000/49h/0h/0h]xpub6B..../1/*))#85dn0v75",
    "wpkh([00000000/84h/0h/0h]xpub6C..../1/*)#36mtsnda",
    "tr([00000000/86h/0h/0h]xpub6C.../1/*)#d63h6jpt"
  ]
}
```

### `displayaddress` (optional)

Usage:
```sh
<cmd> --fingerprint <fingerprint> --chain <name> displayaddress --desc <descriptor>
```

Example, display the first native SegWit receive address on Testnet:

```sh
<cmd> --fingerprint 00000000 --chain test displayaddress --desc "wpkh([00000000/84h/1h/0h]tpubDDUZ..../0/0)"
```

The command MUST be able to figure out the address type from the descriptor.

The command MUST return an object containing `{"address": "[the address]"}`.
As a sanity check, for devices that support this, it SHOULD ask the device to derive the address.

If `<descriptor>` contains a master key fingerprint, the command MUST fail if it does not match the fingerprint known by the device.

If `<descriptor>` contains an xpub, the command MUST fail if it does not match the xpub known by the device.

The command MAY complain if `--chain` is set to a test-network, but the BIP32 coin-type is not `1h` (and vice versa).

## How Bitcoin Core uses the Signer API

The `enumeratesigners` RPC calls `<cmd> enumerate`, skips duplicate entries with the same `fingerprint`, and maps the optional `model` field to the RPC `name` field.

Wallet operations that need a signer (`createwallet`, `walletdisplayaddress` and spending) also call `<cmd> enumerate` and fail unless exactly one signer is found, so only one device should be connected at a time.

The `createwallet` RPC calls:

* `<cmd> --fingerprint 00000000 --chain <name> getdescriptors --account 0`

It then imports descriptors for all supported address types, in a BIP44/49/84/86 compatible manner.

The `walletdisplayaddress` RPC obtains the inferred descriptor for the provided address. It then calls `<cmd> --fingerprint 00000000 --chain <name> displayaddress --desc <descriptor>`.

For external-signer wallets, spending uses `send` or `sendall`, and fee-bumping uses `bumpfee`. Bitcoin Core builds a PSBT, adds key origin information, checks whether any input key origin fingerprint matches the signer, calls `<cmd> --stdin --fingerprint 00000000 --chain <name>`, and sends `signtx <psbt>` over stdin. If signatures are sufficient, it finalizes the transaction and, for broadcasting RPCs, broadcasts it. If signing cannot complete, the call fails with an error. For manual fee-bumping, use `psbtbumpfee` to obtain a PSBT for signing.
