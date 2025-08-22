// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <coins.h>
#include <consensus/validation.h>
#include <index/txindex.h>
#include <merkleblock.h>
#include <node/blockstorage.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <validation.h>

using node::GetTransaction;

static RPCHelpMan gettxoutproof()
{
    return RPCHelpMan{"gettxoutproof",
        "\nReturns a hex-encoded proof that \"txid\" was included in a block.\n"
        "\nNOTE: By default this function only works sometimes. This is when there is an\n"
        "unspent output in the utxo for this transaction. To make it always work,\n"
        "you need to maintain a transaction index, using the -txindex command line option or\n"
        "specify the block in which the transaction is included manually (by blockhash).\n",
        {
            {"txids", RPCArg::Type::ARR, RPCArg::Optional::NO, "The txids to filter",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "A transaction id"},
                },
            },
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "If specified, looks for txid in the block with this hash"},
            {"options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "",
                {
                    {"prove_witness", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, proves the associated wtxid/hash of the specified transactions instead of txid"},
                },
            },
        },
        {
        RPCResult{
                "If prove_witness is false or unspecified",
            RPCResult::Type::STR, "data", "A string that is a serialized, hex-encoded data for the proof."
        },
            RPCResult{
                "If prove_witness is true", RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "proof", "The produced txout proof, hex-encoded."},
                    {RPCResult::Type::OBJ, "proven", "Information about the proof.", {
                        {RPCResult::Type::STR_HEX, "blockhash", "The block hash the proof links to"},
                        {RPCResult::Type::NUM, "blockheight", "The height of the block the proof links to"},
                        {RPCResult::Type::ARR, "tx", "Information about transactions", {
                            {RPCResult::Type::OBJ, "", "Information about a transaction", {
                                {RPCResult::Type::STR_HEX, "txid", "Transaction id this is for (parameter; NOT proven by proof)"},
                                {RPCResult::Type::STR_HEX, "wtxid", "Wtxid/hash of a transaction"},
                                {RPCResult::Type::NUM, "blockindex", "Index of transaction in block"},
                            }},
                        }},
                    }},
                }
            },
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::set<Txid> setTxids;
            UniValue txids = request.params[0].get_array();
            if (txids.empty()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter 'txids' cannot be empty");
            }
            for (unsigned int idx = 0; idx < txids.size(); idx++) {
                auto ret{setTxids.insert(Txid::FromUint256(ParseHashV(txids[idx], "txid")))};
                if (!ret.second) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameter, duplicated txid: ") + txids[idx].get_str());
                }
            }
            const UniValue options{request.params[2].isNull() ? UniValue::VOBJ : request.params[2]};
            bool prove_witness = options["prove_witness"].isNull() ? false : options["prove_witness"].get_bool();

            const CBlockIndex* pblockindex = nullptr;
            uint256 hashBlock;
            ChainstateManager& chainman = EnsureAnyChainman(request.context);
            if (!request.params[1].isNull()) {
                LOCK(cs_main);
                hashBlock = ParseHashV(request.params[1], "blockhash");
                pblockindex = chainman.m_blockman.LookupBlockIndex(hashBlock);
                if (!pblockindex) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
                }
            } else {
                LOCK(cs_main);
                Chainstate& active_chainstate = chainman.ActiveChainstate();

                // Loop through txids and try to find which block they're in. Exit loop once a block is found.
                for (const auto& tx : setTxids) {
                    const Coin& coin{AccessByTxid(active_chainstate.CoinsTip(), tx)};
                    if (!coin.IsSpent()) {
                        pblockindex = active_chainstate.m_chain[coin.nHeight];
                        break;
                    }
                }
            }


            // Allow txindex to catch up if we need to query it and before we acquire cs_main.
            if (g_txindex && !pblockindex) {
                g_txindex->BlockUntilSyncedToCurrentChain();
            }

            if (pblockindex == nullptr) {
                const CTransactionRef tx = GetTransaction(/*block_index=*/nullptr, /*mempool=*/nullptr, *setTxids.begin(), hashBlock, chainman.m_blockman);
                if (!tx || hashBlock.IsNull()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not yet in block");
                }

                LOCK(cs_main);
                pblockindex = chainman.m_blockman.LookupBlockIndex(hashBlock);
                if (!pblockindex) {
                    throw JSONRPCError(RPC_INTERNAL_ERROR, "Transaction index corrupt");
                }
            }

            {
                LOCK(cs_main);
                CheckBlockDataAvailability(chainman.m_blockman, *pblockindex, /*check_for_undo=*/false);
            }
            CBlock block;
            if (!chainman.m_blockman.ReadBlock(block, *pblockindex)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
            }

            unsigned int ntxFound = 0;
            UniValue txs{UniValue::VARR};
            for (size_t i{0}; i < block.vtx.size(); ++i) {
                const auto& tx = block.vtx.at(i);
                if (setTxids.count(tx->GetHash())) {
                    ntxFound++;
                    if (prove_witness) {
                        UniValue txinfo{UniValue::VOBJ};
                        txinfo.pushKV("txid", tx->GetHash().GetHex());
                        txinfo.pushKV("wtxid", tx->GetWitnessHash().GetHex());
                        txinfo.pushKV("blockindex", i);
                        txs.push_back(txinfo);
                    }
                }
            }
            if (ntxFound != setTxids.size()) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not all transactions found in specified or retrieved block");
            }

            DataStream ssMB{};
            CMerkleBlock mb(block, setTxids, /*prove_witness=*/ prove_witness);
            if (prove_witness) {
                mb.SerializeWithWitness(ssMB);
            } else {
            ssMB << mb;
            }
            std::string strHex = HexStr(ssMB);

            if (prove_witness) {
                UniValue proven{UniValue::VOBJ};
                proven.pushKV("blockhash", block.GetHash().GetHex());
                proven.pushKV("blockheight", pblockindex->nHeight);
                proven.pushKV("tx", txs);
                UniValue res{UniValue::VOBJ};
                res.pushKV("proof", strHex);
                res.pushKV("proven", proven);
                return res;
            }

            return strHex;
        },
    };
}

static RPCHelpMan verifytxoutproof()
{
    return RPCHelpMan{"verifytxoutproof",
        "\nVerifies that a proof points to a transaction in a block, returning the transaction it commits to\n"
        "and throwing an RPC error if the block is not in our best chain\n",
        {
            {"proof", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex-encoded proof generated by gettxoutproof"},
            {"options", RPCArg::Type::OBJ_NAMED_PARAMS, RPCArg::Optional::OMITTED, "",
                {
                    {"verify_witness", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, also verifies the associated wtxid/hash of the specified transactions (if included in proof)"},
                },
            },
        },
        {
        RPCResult{
            "If verify_witness is false or unspecified",
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR_HEX, "txid", "The txid(s) which the proof commits to, or empty array if the proof cannot be validated."},
            }
        },
            RPCResult{
                "If verify_witness is true and the proof valid", RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR_HEX, "blockhash", "The block hash this proof links to"},
                    {RPCResult::Type::NUM, "blockheight", "The height of the block this proof links to"},
                    {RPCResult::Type::NUM, "confirmations", /*optional=*/true, "Number of blocks (including the one with the transactions) confirming these transactions"},
                    {RPCResult::Type::NUM, "confirmations_assumed", /*optional=*/true, "The number of unverified blocks confirming these transactions (eg, in an assumed-valid UTXO set)"},
                    {RPCResult::Type::ARR, "tx", "Information about transactions", {
                        {RPCResult::Type::OBJ, "", "Information about a transaction", {
                            {RPCResult::Type::STR_HEX, "wtxid", "Wtxid/hash of a transaction"},
                            {RPCResult::Type::NUM, "blockindex", "Index of transaction in block"},
                        }},
                    }},
                }
            },
            RPCResult{"If verify_witness is true and the proof invalid", RPCResult::Type::OBJ, "", ""},
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const UniValue options{request.params[1].isNull() ? UniValue::VOBJ : request.params[1].get_obj()};
            const bool verify_witness = options["verify_witness"].isNull() ? false : options["verify_witness"].get_bool();

            DataStream ssMB{ParseHexV(request.params[0], "proof")};
            CMerkleBlock merkleBlock;
            if (verify_witness) {
                merkleBlock.UnserializeWithWitness(ssMB);
            } else {
            ssMB >> merkleBlock;
            }

            UniValue res(verify_witness ? UniValue::VOBJ : UniValue::VARR);

            std::vector<uint256> vMatch;
            std::vector<unsigned int> vIndex;
            if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot)
                return res;

            if (vMatch.empty()) {
                return res;
            }

            int witness_commit_outidx{NO_WITNESS_COMMITMENT};
            if (verify_witness) {
                if (vIndex.at(0) != 0) return res;
                if (!merkleBlock.m_gentx) return res;
                if (merkleBlock.m_gentx->GetHash() != vMatch[0]) return res;
                if (!merkleBlock.m_gentx->IsCoinBase()) return res;
                witness_commit_outidx = GetWitnessCommitmentIndex(*merkleBlock.m_gentx);
                if (witness_commit_outidx == NO_WITNESS_COMMITMENT) {
                    if (!merkleBlock.m_prove_gentx) {
                        // We must always prove the gentx to either reveal the wtxid root or prove it has none
                        // But the user may not care to prove the gentx itself, and in this case, we need some way to disambiguate
                        vMatch.erase(vMatch.begin());
                        vIndex.erase(vIndex.begin());
                    }
                } else {
                    vIndex.clear();
                    uint256 wtxid_root = merkleBlock.m_wtxid_tree.ExtractMatches(vMatch, vIndex);
                    if (vMatch.empty()) return res;
                    const auto& gentx_witness_stack{merkleBlock.m_gentx->vin[0].scriptWitness.stack};
                    if (gentx_witness_stack.size() != 1 || gentx_witness_stack[0].size() != 32) return res;
                    CHash256().Write(wtxid_root).Write(gentx_witness_stack[0]).Finalize(wtxid_root);
                    if (memcmp(wtxid_root.begin(), &merkleBlock.m_gentx->vout[witness_commit_outidx].scriptPubKey[6], 32)) {
                        return res;
                    }
                    if (vIndex.at(0) == 0) {
                        auto& gentx_match = vMatch[0];
                        if (!gentx_match.IsNull()) return res;
                        gentx_match = merkleBlock.m_gentx->GetHash();
                    }
                }
            }

            {
            ChainstateManager& chainman = EnsureAnyChainman(request.context);
            LOCK(cs_main);

            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(merkleBlock.header.GetHash());
            if (!pindex || !chainman.ActiveChain().Contains(pindex) || pindex->nTx == 0) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");
            }

            // Check if proof is valid, only add results if so
                if (pindex->nTx != merkleBlock.txn.GetNumTransactions()) {
                    return res;
                }

                if (verify_witness) {
                    res.pushKV("blockheight", pindex->nHeight);

                    const auto pindex_tip = chainman.ActiveChain().Tip();
                    CHECK_NONFATAL(pindex_tip);
                    const auto assumed_base_height = chainman.GetSnapshotBaseHeight();
                    if (assumed_base_height && pindex->nHeight < *assumed_base_height) {
                        res.pushKV("confirmations", 0);
                        res.pushKV("confirmations_assumed", (int64_t)(pindex_tip->nHeight - pindex->nHeight + 1));
                    } else {
                        res.pushKV("confirmations", (int64_t)(pindex_tip->nHeight - pindex->nHeight + 1));
                    }
                }
            }

            if (verify_witness) {
                res.pushKV("blockhash", merkleBlock.header.GetHash().GetHex());
                UniValue txs{UniValue::VARR};
                for (size_t i{0}; i < vMatch.size(); ++i) {
                    UniValue tx{UniValue::VOBJ};
                    tx.pushKV("wtxid", vMatch.at(i).GetHex());
                    tx.pushKV("blockindex", vIndex.at(i));
                    txs.push_back(tx);
                }
                res.pushKV("tx", txs);
                return res;
            }

                for (const uint256& hash : vMatch) {
                    res.push_back(hash.GetHex());
                }
            return res;
        },
    };
}

void RegisterTxoutProofRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"blockchain", &gettxoutproof},
        {"blockchain", &verifytxoutproof},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
