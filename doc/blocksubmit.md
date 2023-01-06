**Is your feature request related to a problem? Please describe.**

Yes. I am working to using the `bitcoin-cli submitblock` method. However, one problem is there is no way to determine two keystone issues with respect to block validation.

1. What, exactly, is being hashed?
2. What, exactly, is the required format for submissions?

**Describe the solution you'd like**

I'd like a standard form example for `submitblock`. For example, a `JSON` file or `txt` file that could be used as a template. The template could be stored as `mydata` and submitted to the network for validation via `bitcoin-cli submitblock mydata`. It would also be great if there was a guide that provided exactly what was required to be hashed. Generally, the solution would define the mathematical inputs and outputs required for block validation, as suggested in the [Forum](https://bitcointalk.org/index.php?topic=5417027.0). 

As I understand it, the goal is to find a nonce, that when hashed is equal to or less than the hash of the previous block. However, if that were true, then in theory it would only require submitting one `32-bit number` to the network via `submitblock`. Still, the documentation discusses so much more, such as `Merkle Roots` and `Coinbase Transactions`. So, the ideal solution would be able to cut out all noise and take a minimalist approach to answering **1** and **2**.


**Describe alternatives you've considered**

The [Bitcoin Core Documentation](https://bitcoincore.org/en/doc/0.16.0/rpc/mining/submitblock/) for the `bitcoin-cli submitblock` method is extremely vague regarding the data formatting. According to the Docs,

```
submitblock "hexdata"  ( "dummy" )

Attempts to submit new block to network.
See https://en.bitcoin.it/wiki/BIP_0022 for full specification.

Arguments
1. "hexdata"        (string, required) the hex-encoded block data to submit
2. "dummy"          (optional) dummy value, for compatibility with BIP22. This value is ignored.

Result:

Examples:
> bitcoin-cli submitblock "mydata"
> curl --user myusername --data-binary '{"jsonrpc": "1.0", "id":"curltest", "method": "submitblock", "params": ["mydata"] }' -H 'content-type: text/plain;' http://127.0.0.1:8332/
```

So, while this tells the reader one way to run this method is `bitcoin-cli submitblock`, no explanation is given for `"mydata"`. As a result, it isn't clear how to actually run the function because there is no way to know what `"mydata"` is or how to appropriately format it.

In the [Bitcoin Developer Reference Guide](https://developer.bitcoin.org/reference/block_chain.html#serialized-blocks), there is a lot of information about block headers, but no actual file regarding the needed structure for `bitcoin-cli submitblock`. We do see an example in hex format of a block header, but it isn't clear what file format the data needs to be stored in or how the block header is generated.

```
02000000 ........................... Block version: 2

b6ff0b1b1680a2862a30ca44d346d9e8
910d334beb48ca0c0000000000000000 ... Hash of previous block's header
9d10aa52ee949386ca9385695f04ede2
70dda20810decd12bc9b048aaab31471 ... Merkle root

24d95a54 ........................... [Unix time][unix epoch time]: 1415239972
30c31b18 ........................... Target: 0x1bc330 * 256**(0x18-3)
fe9f0864 ........................... Nonce
```

Perhaps the best existing solution for what to submit can be found on [Stack Exchange](https://bitcoin.stackexchange.com/questions/43105/how-to-use-bitcoin-cores-submitblock-method).

```
version:     01000000
prev block:  0000000000000000000000000000000000000000000000000000000000000000 
merkle root: 3BA3EDFD7A7B12B27AC72C3E67768F617FC81BC3888A51323A9FB8AA4B1E5E4A
timestamp:   29AB5F49
bits:        FFFF001D
nonce        1DAC2B7C
num txs:     01
tx1:         01000000010000000000000000000000000000000000000000000000000000000000000000FFFFFFFF4D04FFFF001D0104455468652054696D65732030332F4A616E2F32303039204368616E63656C6C6F72206F6E206272696E6B206F66207365636F6E64206261696C6F757420666F722062616E6B73FFFFFFFF0100F2052A01000000434104678AFDB0FE5548271967F1A67130B7105CD6A828E03909A67962E0EA1F61DEB649F6BC3F4CEF38C4F35504E51EC112DE5C384DF7BA0B8D578A4C702B6BF11D5FAC00000000
```

Still, there is no reference to the proper format for this data or how it is derived.

Another example, comes from the [Block Hashing Algorithm Wiki](https://en.bitcoin.it/wiki/Block_hashing_algorithm). The general requirements for the block header are made available.
<img width="946" alt="Screenshot 2023-01-05 at 8 00 27 PM" src="https://user-images.githubusercontent.com/43055154/210927550-0d4e60f9-a750-4c45-b72f-292847c0c359.png">

Moreover, also included is code for the `header_hex`.

```
>>> import hashlib
>>> from binascii import unhexlify, hexlify
>>> header_hex = ("01000000" +
 "81cd02ab7e569e8bcd9317e2fe99f2de44d49ab2b8851ba4a308000000000000" +
 "e320b6c2fffc8d750423db8b1eb942ae710e951ed797f7affc8892b0f1fc122b" +
 "c7f5d74d" +
 "f2b9441a" +
 "42a14695")
>>> header_bin = unhexlify(header_hex)
>>> hash = hashlib.sha256(hashlib.sha256(header_bin).digest()).digest()
>>> hexlify(hash).decode("utf-8")
'1dbd981fe6985776b644b173a4d0385ddc1aa2a829688d1e0000000000000000'
>>> hexlify(hash[::-1]).decode("utf-8")
'00000000000000001e8d6829a8a21adc5d38d0a473b144b6765798e61f98bd1d'

```
The issue here though is that there is no guidance on how to get the block header data, what format to use for storage, or what to submit to the network using the `bitcoin-cli submitblock` method.

Finally, [BIP0022](https://github.com/bitcoin/bips/blob/master/bip-0022.mediawiki) defines the protocol for block submission. 
<img width="1045" alt="Screenshot 2023-01-05 at 7 49 48 PM" src="https://user-images.githubusercontent.com/43055154/210926418-665f4416-91c8-4a19-ab89-e580266f6771.png">

But, the issue here is that there isn't any information on what the required parameters are for `submitblock`.  There is much mention of JSON formatting throughout the Bitcoin Core protocol, but it is not clear if the proper template for `submitblock` requires JSON formatting or some other hexadigit form. While the BIP says there are two arguments taken, no information is given regarding those arguments: 1) a string of the hex-encoded block data to submit; and 2) an object of parameters. Additionally, there is no further information on `workid` other than a data type and the description "if the server provided a workid, it MUST be included with submissions." 

**Additional context**

Running `bitcoin-cli getblocktemplate` gives a lot of data about the blockchain and previous transaction. However, one issue is that it provides both the `previousblockhash` and `target` variables. So, it isn't clear what should be solved for with the hash algorithm.

I apologize for the noob feature request here. I've done my best to aggregate information and knowledge sources on this feature request, but I have found the information to answer the two most important questions. I think it could be possible that there is more than one right answer, but I need to start with one first.

Thanks in advance for any suggestions or feedback here.
