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

#include <fstream>
#include <set>

#include "pop_service_impl.hpp"
#include "vbk/config.hpp"
#include "veriblock/entities/test_case_entity.hpp"

namespace VeriBlock {

namespace {

UniValue createPopTx(const CScript& scriptSig)
{
    LOCK(cs_main);

    auto tx = VeriBlock::MakePopTx(scriptSig);

    const uint256& hashTx = tx.GetHash();
    if (!::mempool.exists(hashTx)) {
        TxValidationState state;
        auto tx_ref = MakeTransactionRef<const CMutableTransaction&>(tx);

        auto result = AcceptToMemoryPool(mempool, state, tx_ref,
            nullptr /* plTxnReplaced */, false /* bypass_limits */, 0 /* nAbsurdFee */, false /* test accept */);
        if (result) {
            std::string err;
            if (!g_rpc_chain->broadcastTransaction(tx_ref, err, 0, true)) {
                throw JSONRPCError(RPC_TRANSACTION_ERROR, err);
            }
            //            RelayTransaction(hashTx, *this->connman);
            return hashTx.GetHex();
        }

        if (state.IsInvalid()) {
            throw JSONRPCError(RPC_TRANSACTION_REJECTED, FormatStateMessage(state));
        }

        throw JSONRPCError(RPC_TRANSACTION_ERROR, FormatStateMessage(state));
    }

    return hashTx.GetHex();
}

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
                bool res = parseBlockPopPayloadsImpl(block, *index->pprev, Params().GetConsensus(), state, &payloads);
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
    for (uint32_t idx = 0u, size = vtb_array.size(); idx < size; ++idx) {
        auto& vtbhex = vtb_array[idx];
        auto vtb = ParseHexV(vtbhex, "vtb[" + std::to_string(idx) + "]");
        script << vtb << OP_CHECKVTB;
    }

    auto& atvhex = request.params[0];
    auto atv = ParseHexV(atvhex, "atv");
    script << atv << OP_CHECKATV;
    script << OP_CHECKPOP;

    return createPopTx(script);
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
    {"pop_mining", "savepopstate", &savepopstate, {}},
};

void RegisterPOPMiningRPCCommands(CRPCTable& t)
{
    for (const auto& command : VeriBlock::commands) {
        t.appendCommand(command.name, &command);
    }
}


} // namespace VeriBlock
