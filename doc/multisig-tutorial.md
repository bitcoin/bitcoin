# 1. Multisig Tutorial

Currently, it is possible to create a multisig wallet using Bitcoin Core only.

Although there is already a brief explanation about the multisig in the [Descriptors documentation](https://github.com/bitcoin/bitcoin/blob/master/doc/descriptors.md#multisig), this tutorial proposes to use the signet (instead of regtest), bringing the reader closer to a real environment and explaining some functions in more detail.

This tutorial uses [jq](https://github.com/stedolan/jq) JSON processor to process the results from RPC and stores the relevant values in bash variables. This makes the tutorial reproducible and easier to follow step by step.

Before starting this tutorial, start the bitcoin node on the signet network.

```bash
./build/bin/bitcoin node -signet -daemon
```

This tutorial also uses the default WPKH derivation path to get the xpubs and does not conform to [BIP 45](https://github.com/bitcoin/bips/blob/master/bip-0045.mediawiki) or [BIP 87](https://github.com/bitcoin/bips/blob/master/bip-0087.mediawiki).

## 1.1 Basic Multisig Workflow

### 1.1 Create the Descriptor Wallets

For a 2-of-3 multisig, create 3 wallets. These wallets contain HD seed and private keys, which will be used to sign the PSBTs and derive the xpub.

These three wallets should not be used directly for privacy reasons (public key reuse). They should only be used to sign transactions for the (watch-only) multisig wallet.

```bash
for ((n=1;n<=3;n++))
do
 ./build/bin/bitcoin rpc -signet createwallet "participant_${n}"
done
```

Extract the xpub of each wallet. To do this, the `derivehdkey` RPC is used.

Note that previously at least two descriptors were usually used, one for external derivation paths and one for internal ones. Since https://github.com/bitcoin/bitcoin/pull/22838 this redundancy has been eliminated by a multipath descriptor with <code><0;1></code> at the [BIP-44](https://github.com/bitcoin/bips/blob/master/bip-0044.mediawiki#change) change level expanding to external and internal descriptors when imported.

```bash
declare -A xpubs

for ((n=1;n<=3;n++))
do
 xpubs["xpub_${n}"]=$(./build/bin/bitcoin rpc -signet -rpcwallet="participant_${n}" derivehdkey "m/44h/1h/0h" | jq -r '.xpub')
done
```

The following command can be used to verify if the xpubs were obtained successfully:

```bash
for x in "${!xpubs[@]}"; do printf "[%s]=%s\n" "$x" "${xpubs[$x]}" ; done
```

As previously mentioned, this step extracts the `m/44'/1'/0'` account instead of the path defined in [BIP 45](https://github.com/bitcoin/bips/blob/master/bip-0045.mediawiki) or [BIP 87](https://github.com/bitcoin/bips/blob/master/bip-0087.mediawiki), because the wallet currently can't sign for a derivation path that's not used in one of its descriptors.

### 1.2 Define the Multisig Descriptor

Define the multisig descriptors.

All RPCs in Bitcoin Core will include the checksum in their output.

```bash
desc="wsh(sortedmulti(2,${xpubs["xpub_1"]}/<0;1>/*,${xpubs["xpub_2"]}/<0;1>/*,${xpubs["xpub_3"]}/<0;1>/*))"

desc_sum=$(./build/bin/bitcoin rpc -signet getdescriptorinfo $desc | jq -r '.checksum')

multisig_desc="[{\"desc\": \"$desc#$desc_sum\", \"active\": true, \"timestamp\": \"now\"}]"
```

`desc` specifies the output type (`wsh`, in this case) and the xpubs involved. They also use BIP 67 (`sortedmulti`), so the wallet can be recreated without worrying about the order of xpubs. Conceptually, descriptors describe a list of scriptPubKey (along with information for spending from it) [[source](https://github.com/bitcoin/bitcoin/issues/21199#issuecomment-780772418)].

The checksum for a descriptor without one can be computed using the `getdescriptorinfo` RPC. The response has the `checksum` field, which is the checksum for the input descriptor, append "#" and this checksum to the input descriptor.

The checksum for a descriptor without one can be computed using the `getdescriptorinfo` RPC. The response has the `descriptor` field, which is the descriptor with the checksum added. The suffix (after #) is the checksum. Descriptors can optionally be suffixed with a checksum to protect against typos or copy-paste errors.

There are other fields that can be added to the descriptors:

* `active`: Sets the descriptor to be the active one for the corresponding output type (`wsh`, in this case).
* `timestamp`: Sets the time from which to start rescanning the blockchain for the descriptor, in UNIX epoch time.

Documentation for these and other parameters can be found by typing `./build/bin/bitcoin rpc help importdescriptors`.

`multisig_desc` concatenates the descriptor in a JSON array and then it will be used to create the multisig wallet.

### 1.3 Create the Multisig Wallet

To create the multisig wallet, first create an empty one (no keys, HD seed and private keys disabled).

Then import the descriptor created in the previous step using the `importdescriptors` RPC.

After that, `listdescriptors` can be used to check if the wallet was created successfully.

```bash
./build/bin/bitcoin rpc -signet -named createwallet wallet_name="multisig_wallet_01" disable_private_keys=true blank=true

./build/bin/bitcoin rpc -signet -rpcwallet="multisig_wallet_01" importdescriptors "$multisig_desc"

./build/bin/bitcoin rpc -signet -rpcwallet="multisig_wallet_01" listdescriptors
```

The `<0;1>` notation in `desc` caused the creation of two descriptors. One uses the chain 0 for external addresses the other the chain 1 for internal ones (change).

Once the wallets have already been created and this tutorial needs to be repeated or resumed, it is not necessary to recreate them, just load them with the command below:

```bash
for ((n=1;n<=3;n++)); do ./build/bin/bitcoin rpc -signet loadwallet "participant_${n}"; done
./build/bin/bitcoin rpc -signet loadwallet "multisig_wallet_01"
```

### 1.4 Fund the wallet

The wallet can receive signet coins by generating a new address and passing it as parameters to `getcoins.py` script.

This script will print a captcha in dot-matrix to the terminal, using unicode Braille characters. After solving the captcha, the coins will be sent directly to the address or wallet (according to the parameters).

The url used by the script can also be accessed directly. At time of writing, the url is [`https://signetfaucet.com`](https://signetfaucet.com).

Coins received by the wallet must have at least 1 confirmation before they can be spent. It is necessary to wait for a new block to be mined before continuing.

```bash
receiving_address=$(./build/bin/bitcoin rpc -signet -rpcwallet="multisig_wallet_01" getnewaddress)

./contrib/signet/getcoins.py -c ./build/bin/bitcoin-cli -a $receiving_address
```

To copy the receiving address onto the clipboard, use the following command. This can be useful when getting coins via the signet faucet mentioned above.

```bash
echo -n "$receiving_address" | xclip -sel clip
```

The `getbalances` RPC may be used to check the balance. Coins with `trusted` status can be spent.

```bash
./build/bin/bitcoin rpc -signet -rpcwallet="multisig_wallet_01" getbalances
```

### 1.5 Create a PSBT

Unlike singlesig wallets, multisig wallets cannot create and sign transactions directly because they require the signatures of the co-signers. Instead they create a Partially Signed Bitcoin Transaction (PSBT).

PSBT is a data format that allows wallets and other tools to exchange information about a Bitcoin transaction and the signatures necessary to complete it. [[source](https://bitcoinops.org/en/topics/psbt/)]

The current PSBT version (v0) is defined in [BIP 174](https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki).

For simplicity, the destination address is taken from the `participant_1` wallet in the code above, but it can be any valid bitcoin address.

The `walletcreatefundedpsbt` RPC is used to create and fund a transaction in the PSBT format. It is the first step in creating the PSBT.

```bash
balance=$(./build/bin/bitcoin rpc -signet -rpcwallet="multisig_wallet_01" getbalance)

amount=$(echo "$balance * 0.8" | bc -l | sed -e 's/^\./0./' -e 's/^-\./-0./')

destination_addr=$(./build/bin/bitcoin rpc -signet -rpcwallet="participant_1" getnewaddress)

funded_psbt=$(./build/bin/bitcoin rpc -signet -rpcwallet="multisig_wallet_01" walletcreatefundedpsbt outputs="{\"$destination_addr\": $amount}" | jq -r '.psbt')
```

There is also the `createpsbt` RPC, which serves the same purpose, but it has no access to the wallet or to the UTXO set. It is functionally the same as `createrawtransaction` and just drops the raw transaction into an otherwise blank PSBT. [[source](https://bitcointalk.org/index.php?topic=5131043.msg50573609#msg50573609)] In most cases, `walletcreatefundedpsbt` solves the problem.

The `send` RPC can also return a PSBT if more signatures are needed to sign the transaction.

### 1.6 Decode or Analyze the PSBT

Optionally, the PSBT can be decoded to a JSON format using `decodepsbt` RPC.

The `analyzepsbt` RPC analyzes and provides information about the current status of a PSBT and its inputs, e.g. missing signatures.

```bash
./build/bin/bitcoin rpc -signet decodepsbt $funded_psbt

./build/bin/bitcoin rpc -signet analyzepsbt $funded_psbt
```

### 1.7 Update the PSBT

In the code above, two PSBTs are created. One signed by `participant_1` wallet and other, by the `participant_2` wallet.

The `walletprocesspsbt` is used by the wallet to sign a PSBT.

```bash
psbt_1=$(./build/bin/bitcoin rpc -signet -rpcwallet="participant_1" walletprocesspsbt $funded_psbt | jq '.psbt')

psbt_2=$(./build/bin/bitcoin rpc -signet -rpcwallet="participant_2" walletprocesspsbt $funded_psbt | jq '.psbt')
```

### 1.8 Combine the PSBT

The PSBT, if signed separately by the co-signers, must be combined into one transaction before being finalized. This is done by `combinepsbt` RPC.

```bash
combined_psbt=$(./build/bin/bitcoin rpc -signet combinepsbt txs="[$psbt_1, $psbt_2]")
```

There is an RPC called `joinpsbts`, but it has a different purpose than `combinepsbt`. `joinpsbts` joins the inputs from multiple distinct PSBTs into one PSBT.

In the example above, the PSBTs are the same, but signed by different participants. If the user tries to merge them using `joinpsbts`, the error `Input txid:pos exists in multiple PSBTs` is returned. To be able to merge different PSBTs into one, they must have different inputs and outputs.

### 1.9 Finalize and Broadcast the PSBT

The `finalizepsbt` RPC is used to produce a network serialized transaction which can be broadcast with `sendrawtransaction`.

It checks that all inputs have complete scriptSigs and scriptWitnesses and, if so, encodes them into network serialized transactions.

```bash
finalized_psbt_hex=$(./build/bin/bitcoin rpc -signet finalizepsbt $combined_psbt | jq -r '.hex')

./build/bin/bitcoin rpc -signet sendrawtransaction $finalized_psbt_hex
```

### 1.10 Alternative Workflow (PSBT sequential signatures)

Instead of each wallet signing the original PSBT and combining them later, the wallets can also sign the PSBTs sequentially. This is less scalable than the previously presented parallel workflow, but it works.

After that, the rest of the process is the same: the PSBT is finalized and transmitted to the network.

```bash
psbt_1=$(./build/bin/bitcoin rpc -signet -rpcwallet="participant_1" walletprocesspsbt $funded_psbt | jq -r '.psbt')

psbt_2=$(./build/bin/bitcoin rpc -signet -rpcwallet="participant_2" walletprocesspsbt $psbt_1 | jq -r '.psbt')

finalized_psbt_hex=$(./build/bin/bitcoin rpc -signet finalizepsbt $psbt_2 | jq -r '.hex')

./build/bin/bitcoin rpc -signet sendrawtransaction $finalized_psbt_hex
```
