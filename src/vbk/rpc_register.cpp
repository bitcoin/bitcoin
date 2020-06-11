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
#include <key_io.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/validation.h>
#include <validation.h>
#include <wallet/rpcwallet.h>
#include <wallet/rpcwallet.h> // for GetWalletForJSONRPCRequest
#include <wallet/wallet.h>    // for CWallet
#include <rpc/blockchain.h>
#include <node/context.h>

#include <fstream>
#include <set>

#include "pop_service_impl.hpp"
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
        VeriBlock::p2p::sendATVs(g_rpc_node->connman.get(), msgMaker, {atv});
        VeriBlock::p2p::sendVTBs(g_rpc_node->connman.get(), msgMaker, vtbs);
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

const CRPCCommand commands[] = {
    {"pop_mining", "submitpop", &submitpop, {"atv", "vtbs"}},
    {"pop_mining", "getpopdata", &getpopdata, {"blockheight"}},
    {"pop_mining", "debugpop", &debugpop, {}},
};

void RegisterPOPMiningRPCCommands(CRPCTable& t)
{
    for (const auto& command : VeriBlock::commands) {
        t.appendCommand(command.name, &command);
    }
}


} // namespace VeriBlock
