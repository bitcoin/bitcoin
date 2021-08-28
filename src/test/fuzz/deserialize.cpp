// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <addrman.h>
#include <blockencodings.h>
#include <blockfilter.h>
#include <chain.h>
#include <coins.h>
#include <primitives/transaction.h>
#include <consensus/merkle.h>
#include <key.h>
#include <merkleblock.h>
#include <net.h>
#include <netbase.h>
#include <node/utxo_snapshot.h>
#include <primitives/block.h>
#include <protocol.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/keyorigin.h>
#include <streams.h>
#include <undo.h>
#include <version.h>

#include <exception>
#include <optional>
#include <stdexcept>
#include <stdint.h>
#include <unistd.h>

#include <test/fuzz/fuzz.h>

void initialize_deserialize()
{
    // Fuzzers using pubkey must hold an ECCVerifyHandle.
    static const ECCVerifyHandle verify_handle;
}

#define FUZZ_TARGET_DESERIALIZE(name, code)                \
    FUZZ_TARGET_INIT(name, initialize_deserialize)         \
    {                                                      \
        try {                                              \
            code                                           \
        } catch (const invalid_fuzzing_input_exception&) { \
        }                                                  \
    }

namespace {

struct invalid_fuzzing_input_exception : public std::exception {
};

template <typename T>
CDataStream Serialize(const T& obj, const int version = INIT_PROTO_VERSION, const int ser_type = SER_NETWORK)
{
    CDataStream ds(ser_type, version);
    ds << obj;
    return ds;
}

template <typename T>
T Deserialize(CDataStream ds)
{
    T obj;
    ds >> obj;
    return obj;
}

template <typename T>
void DeserializeFromFuzzingInput(FuzzBufferType buffer, T& obj, const std::optional<int> protocol_version = std::nullopt, const int ser_type = SER_NETWORK)
{
    CDataStream ds(buffer, ser_type, INIT_PROTO_VERSION);
    if (protocol_version) {
        ds.SetVersion(*protocol_version);
    } else {
        try {
            int version;
            ds >> version;
            ds.SetVersion(version);
        } catch (const std::ios_base::failure&) {
            throw invalid_fuzzing_input_exception();
        }
    }
    try {
        ds >> obj;
    } catch (const std::ios_base::failure&) {
        throw invalid_fuzzing_input_exception();
    }
    assert(buffer.empty() || !Serialize(obj).empty());
}

template <typename T>
void AssertEqualAfterSerializeDeserialize(const T& obj, const int version = INIT_PROTO_VERSION, const int ser_type = SER_NETWORK)
{
    assert(Deserialize<T>(Serialize(obj, version, ser_type)) == obj);
}

} // namespace

FUZZ_TARGET_DESERIALIZE(block_filter_deserialize, {
    BlockFilter block_filter;
    DeserializeFromFuzzingInput(buffer, block_filter);
})
FUZZ_TARGET_DESERIALIZE(addr_info_deserialize, {
    CAddrInfo addr_info;
    DeserializeFromFuzzingInput(buffer, addr_info);
})
FUZZ_TARGET_DESERIALIZE(block_file_info_deserialize, {
    CBlockFileInfo block_file_info;
    DeserializeFromFuzzingInput(buffer, block_file_info);
})
FUZZ_TARGET_DESERIALIZE(block_header_and_short_txids_deserialize, {
    CBlockHeaderAndShortTxIDs block_header_and_short_txids;
    DeserializeFromFuzzingInput(buffer, block_header_and_short_txids);
})
FUZZ_TARGET_DESERIALIZE(fee_rate_deserialize, {
    CFeeRate fee_rate;
    DeserializeFromFuzzingInput(buffer, fee_rate);
    AssertEqualAfterSerializeDeserialize(fee_rate);
})
FUZZ_TARGET_DESERIALIZE(merkle_block_deserialize, {
    CMerkleBlock merkle_block;
    DeserializeFromFuzzingInput(buffer, merkle_block);
})
FUZZ_TARGET_DESERIALIZE(out_point_deserialize, {
    COutPoint out_point;
    DeserializeFromFuzzingInput(buffer, out_point);
    AssertEqualAfterSerializeDeserialize(out_point);
})
FUZZ_TARGET_DESERIALIZE(partial_merkle_tree_deserialize, {
    CPartialMerkleTree partial_merkle_tree;
    DeserializeFromFuzzingInput(buffer, partial_merkle_tree);
})
FUZZ_TARGET_DESERIALIZE(pub_key_deserialize, {
    CPubKey pub_key;
    DeserializeFromFuzzingInput(buffer, pub_key);
    AssertEqualAfterSerializeDeserialize(pub_key);
})
FUZZ_TARGET_DESERIALIZE(script_deserialize, {
    CScript script;
    DeserializeFromFuzzingInput(buffer, script);
})
FUZZ_TARGET_DESERIALIZE(tx_in_deserialize, {
    CTxIn tx_in;
    DeserializeFromFuzzingInput(buffer, tx_in);
    AssertEqualAfterSerializeDeserialize(tx_in);
})
FUZZ_TARGET_DESERIALIZE(flat_file_pos_deserialize, {
    FlatFilePos flat_file_pos;
    DeserializeFromFuzzingInput(buffer, flat_file_pos);
    AssertEqualAfterSerializeDeserialize(flat_file_pos);
})
FUZZ_TARGET_DESERIALIZE(key_origin_info_deserialize, {
    KeyOriginInfo key_origin_info;
    DeserializeFromFuzzingInput(buffer, key_origin_info);
    AssertEqualAfterSerializeDeserialize(key_origin_info);
})
FUZZ_TARGET_DESERIALIZE(partially_signed_transaction_deserialize, {
    PartiallySignedTransaction partially_signed_transaction;
    DeserializeFromFuzzingInput(buffer, partially_signed_transaction);
})
FUZZ_TARGET_DESERIALIZE(prefilled_transaction_deserialize, {
    PrefilledTransaction prefilled_transaction;
    DeserializeFromFuzzingInput(buffer, prefilled_transaction);
})
FUZZ_TARGET_DESERIALIZE(psbt_input_deserialize, {
    PSBTInput psbt_input;
    DeserializeFromFuzzingInput(buffer, psbt_input);
})
FUZZ_TARGET_DESERIALIZE(psbt_output_deserialize, {
    PSBTOutput psbt_output;
    DeserializeFromFuzzingInput(buffer, psbt_output);
})
FUZZ_TARGET_DESERIALIZE(block_deserialize, {
    CBlock block;
    DeserializeFromFuzzingInput(buffer, block);
})
FUZZ_TARGET_DESERIALIZE(blocklocator_deserialize, {
    CBlockLocator bl;
    DeserializeFromFuzzingInput(buffer, bl);
})
FUZZ_TARGET_DESERIALIZE(blockmerkleroot, {
    CBlock block;
    DeserializeFromFuzzingInput(buffer, block);
    bool mutated;
    BlockMerkleRoot(block, &mutated);
})
FUZZ_TARGET_DESERIALIZE(addrman_deserialize, {
    CAddrMan am(/* deterministic */ false, /* consistency_check_ratio */ 0);
    DeserializeFromFuzzingInput(buffer, am);
})
FUZZ_TARGET_DESERIALIZE(blockheader_deserialize, {
    CBlockHeader bh;
    DeserializeFromFuzzingInput(buffer, bh);
})
FUZZ_TARGET_DESERIALIZE(banentry_deserialize, {
    CBanEntry be;
    DeserializeFromFuzzingInput(buffer, be);
})
FUZZ_TARGET_DESERIALIZE(txundo_deserialize, {
    CTxUndo tu;
    DeserializeFromFuzzingInput(buffer, tu);
})
FUZZ_TARGET_DESERIALIZE(blockundo_deserialize, {
    CBlockUndo bu;
    DeserializeFromFuzzingInput(buffer, bu);
})
FUZZ_TARGET_DESERIALIZE(coins_deserialize, {
    Coin coin;
    DeserializeFromFuzzingInput(buffer, coin);
})
FUZZ_TARGET_DESERIALIZE(netaddr_deserialize, {
    CNetAddr na;
    DeserializeFromFuzzingInput(buffer, na);
    if (na.IsAddrV1Compatible()) {
        AssertEqualAfterSerializeDeserialize(na);
    }
    AssertEqualAfterSerializeDeserialize(na, INIT_PROTO_VERSION | ADDRV2_FORMAT);
})
FUZZ_TARGET_DESERIALIZE(service_deserialize, {
    CService s;
    DeserializeFromFuzzingInput(buffer, s);
    if (s.IsAddrV1Compatible()) {
        AssertEqualAfterSerializeDeserialize(s);
    }
    AssertEqualAfterSerializeDeserialize(s, INIT_PROTO_VERSION | ADDRV2_FORMAT);
    CService s1;
    DeserializeFromFuzzingInput(buffer, s1, INIT_PROTO_VERSION);
    AssertEqualAfterSerializeDeserialize(s1, INIT_PROTO_VERSION);
    assert(s1.IsAddrV1Compatible());
    CService s2;
    DeserializeFromFuzzingInput(buffer, s2, INIT_PROTO_VERSION | ADDRV2_FORMAT);
    AssertEqualAfterSerializeDeserialize(s2, INIT_PROTO_VERSION | ADDRV2_FORMAT);
})
FUZZ_TARGET_DESERIALIZE(messageheader_deserialize, {
    CMessageHeader mh;
    DeserializeFromFuzzingInput(buffer, mh);
    (void)mh.IsCommandValid();
})
FUZZ_TARGET_DESERIALIZE(address_deserialize_v1_notime, {
    CAddress a;
    DeserializeFromFuzzingInput(buffer, a, INIT_PROTO_VERSION);
    // A CAddress without nTime (as is expected under INIT_PROTO_VERSION) will roundtrip
    // in all 5 formats (with/without nTime, v1/v2, network/disk)
    AssertEqualAfterSerializeDeserialize(a, INIT_PROTO_VERSION);
    AssertEqualAfterSerializeDeserialize(a, PROTOCOL_VERSION);
    AssertEqualAfterSerializeDeserialize(a, 0, SER_DISK);
    AssertEqualAfterSerializeDeserialize(a, PROTOCOL_VERSION | ADDRV2_FORMAT);
    AssertEqualAfterSerializeDeserialize(a, ADDRV2_FORMAT, SER_DISK);
})
FUZZ_TARGET_DESERIALIZE(address_deserialize_v1_withtime, {
    CAddress a;
    DeserializeFromFuzzingInput(buffer, a, PROTOCOL_VERSION);
    // A CAddress in V1 mode will roundtrip in all 4 formats that have nTime.
    AssertEqualAfterSerializeDeserialize(a, PROTOCOL_VERSION);
    AssertEqualAfterSerializeDeserialize(a, 0, SER_DISK);
    AssertEqualAfterSerializeDeserialize(a, PROTOCOL_VERSION | ADDRV2_FORMAT);
    AssertEqualAfterSerializeDeserialize(a, ADDRV2_FORMAT, SER_DISK);
})
FUZZ_TARGET_DESERIALIZE(address_deserialize_v2, {
    CAddress a;
    DeserializeFromFuzzingInput(buffer, a, PROTOCOL_VERSION | ADDRV2_FORMAT);
    // A CAddress in V2 mode will roundtrip in both V2 formats, and also in the V1 formats
    // with time if it's V1 compatible.
    if (a.IsAddrV1Compatible()) {
        AssertEqualAfterSerializeDeserialize(a, PROTOCOL_VERSION);
        AssertEqualAfterSerializeDeserialize(a, 0, SER_DISK);
    }
    AssertEqualAfterSerializeDeserialize(a, PROTOCOL_VERSION | ADDRV2_FORMAT);
    AssertEqualAfterSerializeDeserialize(a, ADDRV2_FORMAT, SER_DISK);
})
FUZZ_TARGET_DESERIALIZE(inv_deserialize, {
    CInv i;
    DeserializeFromFuzzingInput(buffer, i);
})
FUZZ_TARGET_DESERIALIZE(bloomfilter_deserialize, {
    CBloomFilter bf;
    DeserializeFromFuzzingInput(buffer, bf);
})
FUZZ_TARGET_DESERIALIZE(diskblockindex_deserialize, {
    CDiskBlockIndex dbi;
    DeserializeFromFuzzingInput(buffer, dbi);
})
FUZZ_TARGET_DESERIALIZE(txoutcompressor_deserialize, {
    CTxOut to;
    auto toc = Using<TxOutCompression>(to);
    DeserializeFromFuzzingInput(buffer, toc);
})
FUZZ_TARGET_DESERIALIZE(blocktransactions_deserialize, {
    BlockTransactions bt;
    DeserializeFromFuzzingInput(buffer, bt);
})
FUZZ_TARGET_DESERIALIZE(blocktransactionsrequest_deserialize, {
    BlockTransactionsRequest btr;
    DeserializeFromFuzzingInput(buffer, btr);
})
FUZZ_TARGET_DESERIALIZE(snapshotmetadata_deserialize, {
    SnapshotMetadata snapshot_metadata;
    DeserializeFromFuzzingInput(buffer, snapshot_metadata);
})
FUZZ_TARGET_DESERIALIZE(uint160_deserialize, {
    uint160 u160;
    DeserializeFromFuzzingInput(buffer, u160);
    AssertEqualAfterSerializeDeserialize(u160);
})
FUZZ_TARGET_DESERIALIZE(uint256_deserialize, {
    uint256 u256;
    DeserializeFromFuzzingInput(buffer, u256);
    AssertEqualAfterSerializeDeserialize(u256);
})
// Classes intentionally not covered in this file since their deserialization code is
// fuzzed elsewhere:
// * Deserialization of CTxOut is fuzzed in test/fuzz/tx_out.cpp
// * Deserialization of CMutableTransaction is fuzzed in src/test/fuzz/transaction.cpp
