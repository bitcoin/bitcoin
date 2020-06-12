// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpc_register.hpp"

#include "merkle.hpp"
#include "pop_service.hpp"
#include "vbk/service_locator.hpp"
#include <chainparams.h>
#include <consensus/merkle.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/validation.h>
#include <validation.h>
#include <vbk/p2p_sync.hpp>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h> // for CWallet

#include <fstream>
#include <set>

#include "pop_service_impl.hpp"
#include "vbk/adaptors/univalue_json.hpp"
#include "vbk/config.hpp"
#include "veriblock/entities/test_case_entity.hpp"

namespace VeriBlock {

namespace {

uint256 GetBlockHashByHeight(const int height)
{
    if (height < 0 || height > ChainActive().Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

    return ChainActive()[height]->GetBlockHash();
}

CBlock GetBlockChecked(const CBlockIndex* pblockindex)
{
    CBlock block;
    if (IsBlockPruned(pblockindex)) {
        throw JSONRPCError(RPC_MISC_ERROR, "Block not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        // Block not found on disk. This could be because we have the block
        // header in our index but don't have the block (for example if a
        // non-whitelisted node sends us an unrequested long chain of valid
        // blocks, we add the headers to our index, but don't accept the
        // block).
        throw JSONRPCError(RPC_MISC_ERROR, "Block not found on disk");
    }

    return block;
}

void SaveState(std::string file_name)
{
    LOCK2(cs_main, mempool.cs);

    altintegration::TestCase vbtc_state;
    vbtc_state.config = VeriBlock::getService<VeriBlock::Config>().popconfig;

    auto& vbtc_tree = BlockIndex();

    auto cmp = [](CBlockIndex* a, CBlockIndex* b) -> bool {
        return a->nHeight < b->nHeight;
    };
    std::vector<CBlockIndex*> block_index;
    block_index.reserve(vbtc_tree.size());
    for (const auto& el : vbtc_tree) {
        block_index.push_back(el.second);
    }
    std::sort(block_index.begin(), block_index.end(), cmp);

    BlockValidationState state;

    for (const auto& index : block_index) {
        auto alt_block = blockToAltBlock(*index);
        std::vector<altintegration::AltPayloads> payloads;
        if (index->pprev) {
            CBlock block;
            if (ReadBlockFromDisk(block, index, Params().GetConsensus())) {
                bool res = popDataToPayloads(block, *index->pprev, state, payloads);
                assert(res);
            }
        }
        vbtc_state.alt_tree.push_back(std::make_pair(alt_block, payloads));
    }

    std::ofstream file(file_name, std::ios::binary);

    altintegration::WriteStream stream;
    vbtc_state.toRaw(stream);

    file.write((const char*)stream.data().data(), stream.data().size());

    file.close();
}

} // namespace

UniValue getpopdata(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getpopdata block_height\n"
            "\nFetches the data relevant to PoP-mining the given block.\n"
            "\nArguments:\n"
            "1. block_height         (numeric, required) The height index\n"
            "\nResult:\n"
            "{\n"
            "    \"block_header\" : \"block_header_hex\",  (string) Hex-encoded block header\n"
            "    \"raw_contextinfocontainer\" : \"contextinfocontainer\",  (string) Hex-encoded raw authenticated ContextInfoContainer structure\n"
            "    \"last_known_veriblock_blocks\" : [ (array) last known VeriBlock blocks at the given Bitcoin block\n"
            "        \"blockhash\",                (string) VeriBlock block hash\n"
            "       ... ]\n"
            "    \"last_known_bitcoin_blocks\" : [ (array) last known Bitcoin blocks at the given Bitcoin block\n"
            "        \"blockhash\",                (string) Bitcoin block hash\n"
            "       ... ]\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getpopdata", "1000") + HelpExampleRpc("getpopdata", "1000"));

    auto wallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(wallet.get(), request.fHelp)) {
        return NullUniValue;
    }

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet->BlockUntilSyncedToCurrentChain();

    int height = request.params[0].get_int();

    LOCK2(cs_main, wallet->cs_wallet);

    uint256 blockhash = GetBlockHashByHeight(height);

    UniValue result(UniValue::VOBJ);

    //get the block and its header
    const CBlockIndex* pBlockIndex = LookupBlockIndex(blockhash);

    if (!pBlockIndex) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
    }

    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << pBlockIndex->GetBlockHeader();
    result.pushKV("block_header", HexStr(ssBlock));

    auto block = GetBlockChecked(pBlockIndex);

    auto& pop = getService<PopService>();
    //context info
    uint256 txRoot = BlockMerkleRoot(block);
    auto keystones = VeriBlock::getKeystoneHashesForTheNextBlock(pBlockIndex->pprev);
    auto contextInfo = VeriBlock::ContextInfoContainer(pBlockIndex->nHeight, keystones, txRoot);
    auto authedContext = contextInfo.getAuthenticated();
    result.pushKV("raw_contextinfocontainer", HexStr(authedContext.begin(), authedContext.end()));

    auto lastVBKBlocks = pop.getLastKnownVBKBlocks(16);

    UniValue univalueLastVBKBlocks(UniValue::VARR);
    for (const auto& b : lastVBKBlocks) {
        univalueLastVBKBlocks.push_back(HexStr(b));
    }
    result.pushKV("last_known_veriblock_blocks", univalueLastVBKBlocks);

    auto lastBTCBlocks = pop.getLastKnownBTCBlocks(16);
    UniValue univalueLastBTCBlocks(UniValue::VARR);
    for (const auto& b : lastBTCBlocks) {
        univalueLastBTCBlocks.push_back(HexStr(b));
    }
    result.pushKV("last_known_bitcoin_blocks", univalueLastBTCBlocks);

    return result;
}

UniValue submitpop(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2)
        throw std::runtime_error(
            "submitpop [vtbs] (atv)\n"
            "\nCreates and submits a PoP transaction constructed from the provided ATV and VTBs.\n"
            "\nArguments:\n"
            "1. atv       (string, required) Hex-encoded ATV record.\n"
            "2. vtbs      (array, required) Array of hex-encoded VTB records.\n"
            "\nResult:\n"
            "             (string) Transaction hash\n"
            "\nExamples:\n" +
            HelpExampleCli("submitpop", "ATV_HEX [VTB_HEX VTB_HEX]") + HelpExampleRpc("submitpop", "ATV_HEX [VTB_HEX, VTB_HEX]"));

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR});

    auto wallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(wallet.get(), request.fHelp)) {
        return NullUniValue;
    }
    wallet->BlockUntilSyncedToCurrentChain();

    CScript script;

    const UniValue& vtb_array = request.params[1].get_array();
    LogPrint(BCLog::POP, "VeriBlock-PoP: submitpop RPC called with 1 ATV and %d VTBs\n", vtb_array.size());
    std::vector<altintegration::VTB> vtbs;
    for (uint32_t idx = 0u, size = vtb_array.size(); idx < size; ++idx) {
        auto& vtbhex = vtb_array[idx];
        auto vtb_bytes = ParseHexV(vtbhex, "vtb[" + std::to_string(idx) + "]");
        vtbs.push_back(altintegration::VTB::fromVbkEncoding(vtb_bytes));
    }

    auto& atvhex = request.params[0];
    auto atv_bytes = ParseHexV(atvhex, "atv");
    auto& pop_service = VeriBlock::getService<VeriBlock::PopService>();
    auto& pop_mempool = pop_service.getMemPool();

    {
        LOCK(cs_main);
        altintegration::ValidationState state;
        altintegration::ATV atv = altintegration::ATV::fromVbkEncoding(atv_bytes);
        if (!pop_mempool.submitATV({atv}, state)) {
            LogPrint(BCLog::POP, "VeriBlock-PoP: %s ", state.GetPath());
            return "ivalid ATV";
        }
        if (!pop_mempool.submitVTB(vtbs, state)) {
            LogPrint(BCLog::POP, "VeriBlock-PoP: %s ", state.GetPath());
            return "invalid oone of the VTB";
        }

        const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
        VeriBlock::p2p::sendPopData<altintegration::ATV>(g_rpc_node->connman.get(), msgMaker, {atv});
        VeriBlock::p2p::sendPopData<altintegration::VTB>(g_rpc_node->connman.get(), msgMaker, vtbs);
    }

    return "successful added";
}

UniValue debugpop(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        throw std::runtime_error(
            "debugpop\n"
            "\nPrints alt-cpp-lib state into log.\n");
    }
    auto& pop = VeriBlock::getService<VeriBlock::PopService>();
    LogPrint(BCLog::POP, "%s", pop.toPrettyString());
    return UniValue();
}

UniValue savepopstate(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 1) {
        throw std::runtime_error(
            "savepopstate [file]\n"
            "\nSave pop state into the file.\n"
            "\nArguments:\n"
            "1. file       (string, optional) the name of the file, by default 'vbtc_state'.\n");
    }

    std::string file_name = "vbtc_state";

    if (!request.params.empty()) {
        RPCTypeCheck(request.params, {UniValue::VSTR});
        file_name = request.params[0].getValStr();
    }

    LogPrint(BCLog::POP, "Save vBTC state to the file %s \n", file_name);
    SaveState(file_name);

    return UniValue();
}

using VbkTree = altintegration::VbkBlockTree;
using BtcTree = altintegration::VbkBlockTree::BtcTree;

static VbkTree& vbk()
{
    auto& pop = VeriBlock::getService<VeriBlock::PopService>();
    return pop.getAltTree().vbk();
}

static BtcTree& btc()
{
    auto& pop = VeriBlock::getService<VeriBlock::PopService>();
    return pop.getAltTree().btc();
}

// getblock
namespace {

void check_getblock(const JSONRPCRequest& request, const std::string& chain)
{
    auto cmdname = strprintf("get%sblock", chain);
    RPCHelpMan{
        cmdname,
        "Get block data identified by block hash",
        {
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The block hash"},
        },
        {},
        RPCExamples{
            HelpExampleCli(cmdname, "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"") +
            HelpExampleRpc(cmdname, "\"00000000c937983704a73af28acdec37b049d214adbda81d7e2a3dd146f6ed09\"")},
    }
        .Check(request);
}

template <typename Tree>
UniValue getblock(const JSONRPCRequest& req, Tree& tree, const std::string& chain)
{
    check_getblock(req, chain);
    LOCK(cs_main);

    using block_t = typename Tree::block_t;
    using hash_t = typename block_t::hash_t;
    std::string strhash = req.params[0].get_str();
    hash_t hash;

    try {
        hash = hash_t::fromHex(strhash);
    } catch (const std::exception& e) {
        throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Bad hash: %s", e.what()));
    }

    auto* index = tree.getBlockIndex(hash);
    if (!index) {
        // no block found
        return UniValue(UniValue::VNULL);
    }

    return altintegration::ToJSON<UniValue>(*index);
}

UniValue getvbkblock(const JSONRPCRequest& req)
{
    return getblock(req, vbk(), "vbk");
}
UniValue getbtcblock(const JSONRPCRequest& req)
{
    return getblock(req, btc(), "btc");
}

} // namespace

// getbestblockhash
namespace {
void check_getbestblockhash(const JSONRPCRequest& request, const std::string& chain)
{
    auto cmdname = strprintf("get%bestblockhash", chain);
    RPCHelpMan{
        cmdname,
        "\nReturns the hash of the best (tip) block in the most-work fully-validated chain.\n",
        {},
        RPCResult{
            "\"hex\"      (string) the block hash, hex-encoded\n"},
        RPCExamples{
            HelpExampleCli(cmdname, "") + HelpExampleRpc(cmdname, "")},
    }
        .Check(request);
}

template <typename Tree>
UniValue getbestblockhash(const JSONRPCRequest& request, Tree& tree, const std::string& chain)
{
    check_getbestblockhash(request, chain);

    LOCK(cs_main);
    auto* tip = tree.getBestChain().tip();
    if (!tip) {
        // tree is not bootstrapped
        return UniValue(UniValue::VNULL);
    }

    return UniValue(tip->getHash().toHex());
}

UniValue getvbkbestblockhash(const JSONRPCRequest& request)
{
    return getbestblockhash(request, vbk(), "vbk");
}

UniValue getbtcbestblockhash(const JSONRPCRequest& request)
{
    return getbestblockhash(request, btc(), "btc");
}
} // namespace

// getblockhash
namespace {

void check_getblockhash(const JSONRPCRequest& request, const std::string& chain)
{
    auto cmdname = strprintf("get%sblockhash", chain);

    RPCHelpMan{
        cmdname,
        "\nReturns hash of block in best-block-chain at height provided.\n",
        {
            {"height", RPCArg::Type::NUM, RPCArg::Optional::NO, "The height index"},
        },
        RPCResult{
            "\"hash\"         (string) The block hash\n"},
        RPCExamples{
            HelpExampleCli(cmdname, "1000") +
            HelpExampleRpc(cmdname, "1000")},
    }
        .Check(request);
}

template <typename Tree>
UniValue getblockhash(const JSONRPCRequest& request, Tree& tree, const std::string& chain)
{
    check_getblockhash(request, chain);
    LOCK(cs_main);
    auto& best = tree.getBestChain();
    if (best.blocksCount() == 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Chain %s is not bootstrapped", chain));
    }

    int height = request.params[0].get_int();
    if (height < best.first()->height) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Chain %s starts at %d, provided %d", chain, best.first()->height, height));
    }
    if (height > best.tip()->height) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Chain %s tip is at %d, provided %d", chain, best.tip()->height, height));
    }

    auto* index = best[height];
    assert(index);
    return altintegration::ToJSON<UniValue>(*index);
}

UniValue getvbkblockhash(const JSONRPCRequest& request)
{
    return getblockhash(request, vbk(), "vbk");
}
UniValue getbtcblockhash(const JSONRPCRequest& request)
{
    return getblockhash(request, btc(), "btc");
}

} // namespace

// getpoprawmempool
namespace {

UniValue getrawpopmempool(const JSONRPCRequest& request)
{
    auto cmdname = "getrawpopmempool";
    RPCHelpMan{
        cmdname,
        "\nReturns the list of VBK blocks, ATVs and VTBs stored in POP mempool.\n",
        {},
        RPCResult{"TODO"},
        RPCExamples{
            HelpExampleCli(cmdname, "") +
            HelpExampleRpc(cmdname, "")},
    }
        .Check(request);

    auto& pop = VeriBlock::getService<VeriBlock::PopService>();
    auto& mp = pop.getMemPool();
    return altintegration::ToJSON<UniValue>(mp);
}

} // namespace

// getrawatv
// getrawvtb
// getrawvbkblock
namespace {

template <typename T>
bool GetPayload(const typename T::id_t& hash, T& out, const Consensus::Params& consensusParams, uint256& hashBlock, const CBlockIndex* const block_index)
{
    LOCK(cs_main);
    auto& pop = VeriBlock::getService<VeriBlock::PopService>();

    if (!block_index) {
        auto& mp = pop.getMemPool();
        auto* pl = mp.get<T>(hash);
        if (pl) {
            out = *pl;
            return true;
        }

        // TODO: when payload repository is implemented, search here for payload by ID
        return false;
    } else {
        CBlock block;
        if (ReadBlockFromDisk(block, block_index, consensusParams)) {
            if (VeriBlock::FindPayloadInBlock<T>(block, hash, out)) {
                hashBlock = block_index->GetBlockHash();
                return true;
            }
        }
    }

    return false;
}

template <typename T>
UniValue getrawpayload(const JSONRPCRequest& request, const std::string& name)
{
    auto cmdname = strprintf("getraw%s", name);
    // clang-format off
    RPCHelpMan{
        cmdname,
        "\nReturn the raw " + name + " data.\n"

        "\nBy default this function only works for mempool " + name + ". When called with a blockhash\n"
        "argument, " + cmdname + " will return the " +name+ " if the specified block is available and\n"
        "the " + name + " is found in that block. When called without a blockhash argument, " + cmdname + "\n"
        "will return the " + name + " if it is in the POP mempool, or in local payload repository.\n"

        "\nIf verbose is 'true', returns an Object with information about 'id'.\n"
        "If verbose is 'false' or omitted, returns a string that is serialized, hex-encoded data for 'id'.\n",
        {
            {"id", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The " + name + " id"},
            {"verbose", RPCArg::Type::BOOL, /* default */ "false", "If false, return a string, otherwise return a json object"},
            {"blockhash", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED_NAMED_ARG, "The block in which to look for the " + name + ""},
        },
        {
            RPCResult{"if verbose is not set or set to false",
                "\"data\"      (string) The serialized, hex-encoded data for 'id'\n"},
            RPCResult{"if verbose is set to true", "TODO"},
        },
        RPCExamples{
            HelpExampleCli(cmdname, "\"id\"") +
            HelpExampleCli(cmdname, "\"id\" true") +
            HelpExampleRpc(cmdname, "\"id\", true") +
            HelpExampleCli(cmdname, "\"id\" false \"myblockhash\"") +
            HelpExampleCli(cmdname, "\"id\" true \"myblockhash\"")},
    }
        .Check(request);
    // clang-format on

    bool in_active_chain = true;
    using id_t = typename T::id_t;
    id_t hash;
    try {
        hash = id_t::fromHex(request.params[0].get_str());
    } catch (const std::exception& e) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Bad hash: %s", e.what()));
    }

    CBlockIndex* blockindex = nullptr;

    // Accept either a bool (true) or a num (>=1) to indicate verbose output.
    bool fVerbose = false;
    if (!request.params[1].isNull()) {
        fVerbose = request.params[1].isNum() ? (request.params[1].get_int() != 0) : request.params[1].get_bool();
    }

    if (!request.params[2].isNull()) {
        LOCK(cs_main);

        uint256 blockhash = ParseHashV(request.params[2], "parameter 3");
        blockindex = LookupBlockIndex(blockhash);
        if (!blockindex) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block hash not found");
        }
        in_active_chain = ::ChainActive().Contains(blockindex);
    }

    T out;
    uint256 hash_block;
    if (!GetPayload<T>(hash, out, Params().GetConsensus(), hash_block, blockindex)) {
        std::string errmsg;
        if (blockindex) {
            if (!(blockindex->nStatus & BLOCK_HAVE_DATA)) {
                throw JSONRPCError(RPC_MISC_ERROR, "Block not available");
            }
            errmsg = "No such " + name + " found in the provided block";
        } else {
            errmsg = "No such mempool or blockchain " + name;
        }
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, errmsg);
    }

    if (!fVerbose) {
        return altintegration::ToJSON<UniValue>(out.toHex());
    }

    UniValue result(UniValue::VOBJ);
    if (blockindex) result.pushKV("in_active_chain", in_active_chain);

    result.pushKV(name, altintegration::ToJSON<UniValue>(out));
    result.pushKV("blockhash", hash_block.GetHex());

    return result;
}

UniValue getrawatv(const JSONRPCRequest& req)
{
    return getrawpayload<altintegration::ATV>(req, "atv");
}
UniValue getrawvtb(const JSONRPCRequest& req)
{
    return getrawpayload<altintegration::VTB>(req, "vtb");
}
UniValue getrawvbkblock(const JSONRPCRequest& req)
{
    return getrawpayload<altintegration::VbkBlock>(req, "vbkblock");
}

} // namespace

const CRPCCommand commands[] = {
    {"pop_mining", "submitpop", &submitpop, {"atv", "vtbs"}},
    {"pop_mining", "getpopdata", &getpopdata, {"blockheight"}},
    {"pop_mining", "debugpop", &debugpop, {}},
    {"pop_mining", "savepopstate", &savepopstate, {"path"}},
    {"pop_mining", "getvbkblock", &getvbkblock, {"hash"}},
    {"pop_mining", "getbtcblock", &getbtcblock, {"hash"}},
    {"pop_mining", "getvbkbestblockhash", &getvbkbestblockhash, {}},
    {"pop_mining", "getbtcbestblockhash", &getbtcbestblockhash, {}},
    {"pop_mining", "getvbkblockhash", &getvbkblockhash, {"height"}},
    {"pop_mining", "getbtcblockhash", &getbtcblockhash, {"height"}},
    {"pop_mining", "getrawatv", &getrawatv, {"id"}},
    {"pop_mining", "getrawvtb", &getrawvtb, {"id"}},
    {"pop_mining", "getrawvbkblock", &getrawvbkblock, {"id"}},
    {"pop_mining", "getrawpopmempool", &getrawpopmempool, {}}};

void RegisterPOPMiningRPCCommands(CRPCTable& t)
{
    for (const auto& command : VeriBlock::commands) {
        t.appendCommand(command.name, &command);
    }
}


} // namespace VeriBlock
