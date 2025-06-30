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
                    {"prove_witness", RPCArg::Type::BOOL, RPCArg::Default{false}, "If true, also proves the associated wtxid/hash of the specified transactions"},
                },
            },
        },
        RPCResult{
            RPCResult::Type::STR, "data", "A string that is a serialized, hex-encoded data for the proof."
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

            CBlock block;
            if (!chainman.m_blockman.ReadBlockFromDisk(block, *pblockindex)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Can't read block from disk");
            }

            unsigned int ntxFound = 0;
            const UniValue options{request.params[2].isNull() ? UniValue::VOBJ : request.params[2]};
            bool prove_witness = options["prove_witness"].isNull() ? false : options["prove_witness"].get_bool();
            bool any_witness{!prove_witness};  // ignored if not proving witnesses
            for (const auto& tx : block.vtx) {
                if (setTxids.count(tx->GetHash())) {
                    ntxFound++;
                    if (!any_witness) any_witness = tx->HasWitness();
                }
            }
            if (ntxFound != setTxids.size()) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not all transactions found in specified or retrieved block");
            }

            if (!any_witness) prove_witness = false;

            DataStream ssMB{};
            CMerkleBlock mb(block, setTxids, /*prove_witness=*/ prove_witness);
            mb.SerializeWithWitness(ssMB);
            std::string strHex = HexStr(ssMB);
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
                "If verify_witness is true", RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::OBJ_DYN, "txids", "The txid(s) which the proof commits to, or empty array if the proof cannot be validated.", {
                        {RPCResult::Type::STR_HEX, "txid", "wtxid (or true, if not verified or transaction isn't segwit)", {}, /*skip_type_check=*/ true},
                    }},
                }
            }
        },
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            const UniValue options{request.params[1].isNull() ? UniValue::VOBJ : request.params[1]};
            const bool verify_witness = options["verify_witness"].isNull() ? false : options["verify_witness"].get_bool();

            DataStream ssMB{ParseHexV(request.params[0], "proof")};
            CMerkleBlock merkleBlock;
            merkleBlock.UnserializeWithWitness(ssMB, /*verify_witness=*/ verify_witness);

            UniValue res(verify_witness ? UniValue::VOBJ : UniValue::VARR);

            std::vector<uint256> vMatch;
            std::vector<unsigned int> vIndex;
            if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot)
                return res;

            if (vMatch.empty()) {
                return res;
            }

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

            if (!verify_witness) {
                for (size_t i = 0; i < vIndex.size(); ++i) {
                    const uint256& txid = vMatch.at(i);
                    const int idx = vIndex.at(i);
                    if (idx || merkleBlock.m_prove_gentx) {
                        res.push_back(txid.GetHex());
                    }
                }
                return res;
            }

            const auto wtxids = [&]() -> std::unordered_map<unsigned int, uint256> {
                if (!merkleBlock.m_gentx) return {};
                if (merkleBlock.m_gentx->GetHash() != vMatch[0]) return {};
                const int witness_commit_outidx = GetWitnessCommitmentIndex(*merkleBlock.m_gentx);
                // In theory, no commitment means all txids are also wtxids, but we don't ever prove witness for non-witness txs, so something is fishy if that happens here
                if (witness_commit_outidx == NO_WITNESS_COMMITMENT) return {};
                std::vector<uint256> wtxid_matches;
                std::vector<unsigned int> wtxid_indexes;
                const auto wtxid_root = merkleBlock.m_wtxid_tree.ExtractMatches(wtxid_matches, wtxid_indexes);
                if (memcmp(wtxid_root.begin(), &merkleBlock.m_gentx->vout[witness_commit_outidx].scriptPubKey[6], 32)) {
                    // FIXME: For some reason, this isn't matching currently
                    LogPrintf("%s vs %s DEBUG", wtxid_root.GetHex(), HexStr(MakeUCharSpan(merkleBlock.m_gentx->vout[witness_commit_outidx].scriptPubKey)));
                    return {};
                }

                std::unordered_map<unsigned int, uint256> wtxid_map;
                for (size_t i = 0; i < wtxid_matches.size(); ++i) {
                    wtxid_map[wtxid_indexes.at(i)] = wtxid_matches.at(i);
                }
                return wtxid_map;
            }();

            UniValue res_txids(UniValue::VOBJ);
            for (size_t i = 0; i < vIndex.size(); ++i) {
                const uint256& txid = vMatch.at(i);
                const int idx = vIndex.at(i);
                if (idx || merkleBlock.m_prove_gentx) {
                    const auto wtxid_it = wtxids.find(idx);
                    res_txids.pushKV(txid.GetHex(), (wtxid_it != wtxids.end()) ? UniValue(wtxid_it->second.GetHex()) : UniValue(true));
                }
            }
            res.pushKV("txids", res_txids);

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
