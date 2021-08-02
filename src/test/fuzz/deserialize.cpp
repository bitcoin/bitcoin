// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <blockencodings.h>
#include <blockfilter.h>
#include <chain.h>
#include <coins.h>
#include <compressor.h>
#include <consensus/merkle.h>
#include <key.h>
#include <merkleblock.h>
#include <net.h>
#include <primitives/block.h>
#include <protocol.h>
#include <psbt.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/sign.h>
#include <streams.h>
#include <undo.h>
#include <version.h>

#include <stdexcept>
#include <stdint.h>
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <vector>

#include <test/fuzz/fuzz.h>

void initialize()
{
    // Fuzzers using pubkey must hold an ECCVerifyHandle.
    static const auto verify_handle = MakeUnique<ECCVerifyHandle>();
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    CDataStream ds(buffer, SER_NETWORK, INIT_PROTO_VERSION);
    try {
        int nVersion;
        ds >> nVersion;
        ds.SetVersion(nVersion);
    } catch (const std::ios_base::failure&) {
        return;
    }

#if BLOCK_FILTER_DESERIALIZE
    return; // unimplemented
    // try {
    //     BlockFilter block_filter;
    //     ds >> block_filter;
    // } catch (const std::ios_base::failure&) {
    // }
#elif ADDR_INFO_DESERIALIZE
    try {
        CAddrInfo addr_info;
        ds >> addr_info;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCK_FILE_INFO_DESERIALIZE
    try {
        CBlockFileInfo block_file_info;
        ds >> block_file_info;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCK_HEADER_AND_SHORT_TXIDS_DESERIALIZE
    try {
        CBlockHeaderAndShortTxIDs block_header_and_short_txids;
        ds >> block_header_and_short_txids;
    } catch (const std::ios_base::failure&) {
    }
#elif FEE_RATE_DESERIALIZE
    try {
        CFeeRate fee_rate;
        ds >> fee_rate;
    } catch (const std::ios_base::failure&) {
    }
#elif MERKLE_BLOCK_DESERIALIZE
    try {
        CMerkleBlock merkle_block;
        ds >> merkle_block;
    } catch (const std::ios_base::failure&) {
    }
#elif OUT_POINT_DESERIALIZE
    try {
        COutPoint out_point;
        ds >> out_point;
    } catch (const std::ios_base::failure&) {
    }
#elif PARTIAL_MERKLE_TREE_DESERIALIZE
    try {
        CPartialMerkleTree partial_merkle_tree;
        ds >> partial_merkle_tree;
    } catch (const std::ios_base::failure&) {
    }
#elif PUB_KEY_DESERIALIZE
    try {
        CPubKey pub_key;
        ds >> pub_key;
    } catch (const std::ios_base::failure&) {
    }
#elif SCRIPT_DESERIALIZE
    try {
        CScript script;
        ds >> script;
    } catch (const std::ios_base::failure&) {
    }
#elif SUB_NET_DESERIALIZE
    try {
        CSubNet sub_net;
        ds >> sub_net;
    } catch (const std::ios_base::failure&) {
    }
#elif TX_IN_DESERIALIZE
    try {
        CTxIn tx_in;
        ds >> tx_in;
    } catch (const std::ios_base::failure&) {
    }
#elif FLAT_FILE_POS_DESERIALIZE
    return; // unimplemented
    // try {
    //     FlatFilePos flat_file_pos;
    //     ds >> flat_file_pos;
    // } catch (const std::ios_base::failure&) {
    // }
#elif KEY_ORIGIN_INFO_DESERIALIZE
    return; // unimplemented
    // try {
    //     KeyOriginInfo key_origin_info;
    //     ds >> key_origin_info;
    // } catch (const std::ios_base::failure&) {
    // }
#elif PARTIALLY_SIGNED_TRANSACTION_DESERIALIZE
    try {
        PartiallySignedTransaction partially_signed_transaction;
        ds >> partially_signed_transaction;
    } catch (const std::ios_base::failure&) {
    }
#elif PREFILLED_TRANSACTION_DESERIALIZE
    try {
        PrefilledTransaction prefilled_transaction;
        ds >> prefilled_transaction;
    } catch (const std::ios_base::failure&) {
    }
#elif PSBT_INPUT_DESERIALIZE
    try {
        PSBTInput psbt_input;
        ds >> psbt_input;
    } catch (const std::ios_base::failure&) {
    }
#elif PSBT_OUTPUT_DESERIALIZE
    try {
        PSBTOutput psbt_output;
        ds >> psbt_output;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCK_DESERIALIZE
    try {
        CBlock block;
        ds >> block;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCKLOCATOR_DESERIALIZE
    try {
        CBlockLocator bl;
        ds >> bl;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCKMERKLEROOT
    try {
        CBlock block;
        ds >> block;
        bool mutated;
        BlockMerkleRoot(block, &mutated);
    } catch (const std::ios_base::failure&) {
    }
#elif ADDRMAN_DESERIALIZE
    try {
        CAddrMan am;
        ds >> am;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCKHEADER_DESERIALIZE
    try {
        CBlockHeader bh;
        ds >> bh;
    } catch (const std::ios_base::failure&) {
    }
#elif BANENTRY_DESERIALIZE
    try {
        CBanEntry be;
        ds >> be;
    } catch (const std::ios_base::failure&) {
    }
#elif TXUNDO_DESERIALIZE
    try {
        CTxUndo tu;
        ds >> tu;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCKUNDO_DESERIALIZE
    try {
        CBlockUndo bu;
        ds >> bu;
    } catch (const std::ios_base::failure&) {
    }
#elif COINS_DESERIALIZE
    try {
        Coin coin;
        ds >> coin;
    } catch (const std::ios_base::failure&) {
    }
#elif NETADDR_DESERIALIZE
    try {
        CNetAddr na;
        ds >> na;
    } catch (const std::ios_base::failure&) {
    }
#elif SERVICE_DESERIALIZE
    try {
        CService s;
        ds >> s;
    } catch (const std::ios_base::failure&) {
    }
#elif MESSAGEHEADER_DESERIALIZE
    CMessageHeader::MessageStartChars pchMessageStart = {0x00, 0x00, 0x00, 0x00};
    try {
        CMessageHeader mh(pchMessageStart);
        ds >> mh;
        (void)mh.IsValid(pchMessageStart);
    } catch (const std::ios_base::failure&) {
    }
#elif ADDRESS_DESERIALIZE
    try {
        CAddress a;
        ds >> a;
    } catch (const std::ios_base::failure&) {
    }
#elif INV_DESERIALIZE
    try {
        CInv i;
        ds >> i;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOOMFILTER_DESERIALIZE
    try {
        CBloomFilter bf;
        ds >> bf;
    } catch (const std::ios_base::failure&) {
    }
#elif DISKBLOCKINDEX_DESERIALIZE
    try {
        CDiskBlockIndex dbi;
        ds >> dbi;
    } catch (const std::ios_base::failure&) {
    }
#elif TXOUTCOMPRESSOR_DESERIALIZE
    try
    {
        CTxOut to;
        auto toc = Using<TxOutCompression>(to);
        ds >> toc;
    } catch (const std::ios_base::failure& e) {
    }
#elif BLOCKTRANSACTIONS_DESERIALIZE
    try {
        BlockTransactions bt;
        ds >> bt;
    } catch (const std::ios_base::failure&) {
    }
#elif BLOCKTRANSACTIONSREQUEST_DESERIALIZE
    try {
        BlockTransactionsRequest btr;
        ds >> btr;
    } catch (const std::ios_base::failure&) {
    }
#else
#error Need at least one fuzz target to compile
#endif
}
