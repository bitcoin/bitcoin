# Compressed Transaction Schema
By (Tom Briar) and (Andrew Poelstra)

## 1. Abstract

With this Transaction Compression Schema we use several methods to compress transactions,
including dropping data and recovering it on decompression by grinding until we obtain
valid signatures.

The bulk of our size savings come from replacing the prevout of each input by a block
height and index. This requires the decompression to have access to the blockchain, and
also means that compression is ineffective for transactions that spend unconfirmed or
insufficiently confirmed outputs.

Even without compression, Taproot keyspends are very small: as witness data they
include only a single 64/65-byte signature and do not repeat the public key or
any other metadata. By using pubkey recovery, we obtain Taproot-like compression
for legacy and Segwit transactions.

The main applications for this schema are for steganography, satellite/radio broadcast, and
other low bandwidth channels with a high CPU availability on decompression. We
assume users have some ability to shape their transactions to improve their
compressibility, and therefore give special treatment to certain transaction forms.

This schema is easily reversible except for compressing the Txid/Vout input pairs(Method 4).
Compressing the input Txid/Vout is optional, and without it still gleans 50% of the
total compression. This allows for the additional use case of P2P communication.

## 2. Methods

The four main methods to achieve a lower transactions size are:

1. Packing transaction metadata before the transaction and each of its inputs and
outputs to determine the structure of the following data.
2. Replacing 32-bit numeric values with either variable-length integers (VarInts) or compact-integers (CompactSizes).
3. Using compressed signatures and public key recovery upon decompression.
4. Replacing the 36-byte txid/vout pair with a blockheight and output index.

Method 4 will cause the compressed transaction to be undecompressable if a block
reorg occurs at or before the block it's included in. Therefore, we'll only compress
the Txid if the transaction input is at least one hundred blocks old.


## 3 Schema

### 3.1 Primitives

| Name             | Width     | Description |
|------------------|-----------|-------------|
| CompactSize      | 1-5 Bytes | For 0-253, encode the value directly in one byte. For 254-65535, encode 254 followed by 2 little-endian bytes. For 65536-(2^32-1), encode 255 followed by 4 little-endian bytes. |
| CompactSize flag | 2 Bits    | 1, 2 or 3 indicate literal values. 0 indicates that the value will be encoded in a later CompactInt. |
| VarInt           | 1+ Bytes  | 7-bit little-endian encoding, with each 7-bit word encoded in a byte. The highest bit of each byte is 1 if more bytes follow, and 0 for the last byte. |
| VLP-Bytestream   | 2+ Bytes  | A VarInt Length Prefixed Bytestream. Has a VarInt prefixed to determine the length. |

### 3.2 General Schema

| Name                           | Width           | Description |
|--------------------------------|-----------------|-------------|
| Transaction Metadata           | 1 Byte    | Information on the structure of the transaction. See Section 3.3. |
| Version                        | 0-5 Bytes | An optional CompactSize containing the transactions version. |
| Input Count                    | 0-5 Bytes | An optional CompactSize containing the transactions input count. |
| Output Count                   | 0-5 Bytes | An optional CompactSize containing the transactions output count. |
| LockTime                       | 0-5 Bytes | An optional CompactSize containing the transaction LockTime if its non zero. |
| Minimum Blockheight            | 1-5 Bytes | A VarInt containing the Minimum Blockheight of which the transaction locktime and input blockheights are given as offsets. |
| Input Metadata+Output Metadata | 1+ Bytes  | A Encoding containing metadata on all the inputs and then all the outputs of the transaction. For each input see Section 3.4, for each output see Section 3.5. |
| Input Data                     | 66+ Bytes | See Section 3.6 for each input. |
| Output Data                    | 3+ Bytes  | See Section 3.7 for each output. |

For the four CompactSize listed above we could use a more compact bit encoding for these but they are already a fall back for the bit encoding of the Transaction Metadata.

### 3.3 Transaction Metadata

| Name         | Width  | Description |
|--------------|--------|-------------|
| Version      | 2 Bits | A CompactSize flag for the transaction version. |
| Input Count  | 2 Bits | A CompactSize flag for the transaction input count. |
| Output Count | 2 Bits | A CompactSize flag for the transaction output count. |
| LockTime     | 1 Bit  | A Boolean to indicate if the transaction has a LockTime. |

### 3.4 Input Metadata

| Name                 | Width  | Description |
|----------------------|--------|-------------|
| Compressed Signature | 1 Bit  | Signature compression flag. |
| Pubkey Hash Included | 1 Bit  | Weather or not the Hash of the Public key used in the Redeem Script for P2SH-WPKH has been included. |
| Standard Hash        | 1 Bit  | A flag to determine if this Input's Signature Hash Type is standard (0x00 for Taproot, 0x01 for Legacy/Segwit). |
| Standard Sequence    | 2 Bits | A CompactSize flag for the inputs sequence. Encode literal values as follows: 1 = 0x00000000, 2 = 0xFFFFFFFE, 3 = 0xFFFFFFFF. |

### 3.5.1 Output Metadata

| Name                | Width  | Description |
|---------------------|--------|-------------|
| Encoded Script Type | 3 Bits | Encoded Script Type. |

#### 3.5.2 Script Type encoding

| Script Type                | Value |
|----------------------------|-------|
| Uncompressed P2PK          | 0b000 |
| Compressed P2PK            | 0b001 |
| P2PKH                      | 0b010 |
| P2SH                       | 0b011 |
| P2WSH                      | 0b100 |
| P2WPKH                     | 0b101 |
| P2TR                       | 0b110 |
| Uncompressed Custom Script | 0b111 |

### 3.6 Input Data

| Name                    | Width     | Description |
|-------------------------|-----------|-------------|
| Sequence                | 0-5 Bytes | An Optional VarInt containing the sequence if it was non-standard. |
| Txid Blockheight        | 1-5 Bytes | A VarInt Either containing 0 if this an uncompressed input, or it contains the offset from Minimum Blockheight for this Txid. |
| Txid/Signature Data     | 65+ Bytes | Txid/Signatures are determined to be uncompressed either by the output script of the previous transaction, or if the Txid Blockheight is zero. For each Compressed Txid/Signature See Section 3.6.1. For each Uncompressed Txid/Signature See Section 3.6.2. |

### 3.6.1 Compressed Txid/Signature Data

| Name              | Width     | Description |
|-------------------|-----------|-------------|
| Txid Block Index  | 1-5 Bytes | A VarInt containing the flattened index from the Txid Blockheight for the Vout. |
| Signature         | 64 Bytes  | Contains the 64 byte signature. |
| Public Key Hash   | 0-20 Bytes| If input is P2SH-P2WPKH contains the 20 byte hash of the public key. |
| Hash Type         | 0-1 Bytes | An Optional Byte containing the Hash Type if it was non-standard.|

### 3.6.2 Uncompressed Txid/Signature Data

| Name      | Width     | Description |
|-----------|-----------|-------------|
| Txid      | 32 Bytes  | Contains the 32 byte Txid. |
| Vout      | 1-5 Bytes | A CompactSize Containing the Vout of the Txid. |
| Signature | 2+ Bytes  | A VLP-Bytestream containing the signature. |

### 3.7 Output Data

| Name          | Width     | Description |
|---------------|-----------|-------------|
| Output Script | 2+ Bytes  | A VLP-Bytestream containing the output script. |
| Amount        | 1-9 Bytes | A VarInt containing the output amount. |

## 4 Ideal Transaction

The target transaction for the most optimal compression was chosen
based off the most common transactions that are likely to be used
for purposes that requires the best compression.

| Field           | Requirements                      | Possible Savings                  |
|-----------------|-----------------------------------|-----------------------------------|
| Version         | Less than four                    | 30 Bits                           |
| Input Count     | Less then four                    | 30 Bits                           |
| Output Count    | Less then four                    | 30 Bits                           |
| LockTime        | 0                                 | 30 Bits                           |
| Input Sequence  | 0x00, 0xFFFFFFFE, or 0xFFFFFFFF   | 62 Bits For Each Input            |
| Input Txid      | Compressed Outpoint               | 23-31 Bytes For Each Input        |
| Input Vout      | Compressed Outpoint               | (-1)-3 Bytes For Each Input       |
| Input Signature | Non-custom Script Signing         | 40-72 Bytes For Each Legacy Input |
| Input Hash Type | 0x00 for Taproot, 0x01 for Legacy | 7 Bits For Each Input             |
| Output Script   | Non-custom Scripts                | 2-5 Bytes For Each Output         |
| Output Amount   | No Restrictions                   | (-1)-7 Bytes For Each Output      |

## 5 Test Vectors

| Transaction                  | Before Compression | After Compression | Savings         |
|------------------------------|--------------------|-------------------|-----------------|
| 1-(input/output) P2TR        | 150 Bytes          | 101 Bytes         | 98 Bytes  / 32% |
| 1-(input/output) P2WPKH      | 191 Bytes          | 101 Bytes         | 180 Bytes / 47% |
| 1-(input/output) P2SH-P2WPKH | 214 Bytes          | 121 Bytes         | 186 Bytes / 43% |
| 1-(input/output) P2PKH       | 188 Bytes          | 101 Bytes         | 174 Bytes / 46% |

### 5.1 Taproot

#### 5.1.1 Uncompressed
```
020000000001017ad1d0cc314504ec06f1b5c786c50cf3cda30bd5be88cf08ead571b0ce7481fb0000000000fdffffff0188130000000000001600142da377ed4978fefa043a58489912f8e28e16226201408ce65b3170d3fbc68e3b6980650514dc53565f915d14351f83050ff50c8609495b7aa96271c3c99cdac1a92b1b45e77a4a870251fc1673596793adf2494565e500000000
```

#### 5.1.2 Compressed
```
16b1ec7f2db1ed00b0218ce65b3170d3fbc68e3b6980650514dc53565f915d14351f83050ff50c8609495b7aa96271c3c99cdac1a92b1b45e77a4a870251fc1673596793adf2494565e58efefefe7d2da377ed4978fefa043a58489912f8e28e162262a608
```

### 5.2 P2WPKH

#### 5.2.1 Uncompressed
```
0200000000010144bcf05ab48b8789268a7ca07133241ad654c0739ac7165015b2d669eadb10ea0000000000fdffffff0188130000000000001600142da377ed4978fefa043a58489912f8e28e16226202473044022043ab639a98dfbc704f16a35bf25b8b72acb4cb928fd772285f1fcf63725caa85022001c9ff354504e7024708bce61f30370c8db13da8170cef4e8e4c4cdad0f71bfe0121030072484c24705512bfb1f7f866d95f808d81d343e552bc418113e1b9a1da0eb400000000
```

#### 5.2.2 Compressed
```
16b1ec712db1ec72932643ab639a98dfbc704f16a35bf25b8b72acb4cb928fd772285f1fcf63725caa8501c9ff354504e7024708bce61f30370c8db13da8170cef4e8e4c4cdad0f71bfe8efefefe7d2da377ed4978fefa043a58489912f8e28e162262a608
```

### 5.3 P2SH-P2WPKH

#### 5.3.1 Uncompressed
```
0200000000010192fb2e4332b43dc9a73febba67f3b7d97ba890673cb08efde2911330f77bbdfc00000000171600147a1979232206857167b401fdac1ffbf33f8204fffdffffff0188130000000000001600142da377ed4978fefa043a58489912f8e28e16226202473044022041eb682e63c25b85a5a400b11d41cf4b9c25f309090a5f3e0b69dc15426da90402205644ddc3d5179bab49cce4bf69ebfaeab1afa34331c1a0a70be2927d2836b0e8012103c483f1b1bd24dd23b3255a68d87ef9281f9d080fd707032ccb81c1cc56c5b00200000000
```

#### 5.3.2 Compressed
```
16b1ec7c3db1ec7d981641eb682e63c25b85a5a400b11d41cf4b9c25f309090a5f3e0b69dc15426da9045644ddc3d5179bab49cce4bf69ebfaeab1afa34331c1a0a70be2927d2836b0e87a1979232206857167b401fdac1ffbf33f8204ff8efefefe7d2da377ed4978fefa043a58489912f8e28e162262a608
```

### 5.4 P2PKH

#### 5.4.1 Uncompressed
```
02000000015f5be26862482fe2fcc900f06ef26ee256fb205bc4773e5a402d0c1b88b82043000000006a473044022031a20f5d9212023b510599c9d53d082f8e07faaa2d51482e078f8e398cb50d770220635abd99220ad713a081c4f20b83cb3f491ed8bd032cb151a3521ed144164d9c0121027977f1b6357cead2df0a0a19570088a1eb9115468b2dfa01439493807d8f1294fdffffff0188130000000000001600142da377ed4978fefa043a58489912f8e28e16226200000000
```

#### 5.4.2 Compressed
```
16b1ec7c2db1ec7d981431a20f5d9212023b510599c9d53d082f8e07faaa2d51482e078f8e398cb50d77635abd99220ad713a081c4f20b83cb3f491ed8bd032cb151a3521ed144164d9c8efefefe7d2da377ed4978fefa043a58489912f8e28e162262a608
```
