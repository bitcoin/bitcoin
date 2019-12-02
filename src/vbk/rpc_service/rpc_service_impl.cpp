#include "rpc_service_impl.hpp"

#include <chainparams.h>            // for Params()
#include <consensus/merkle.h>       // for BlockMerkleRoot
#include <consensus/validation.h>   // for CValidationState
#include <index/txindex.h>          // for g_txindex
#include <key_io.h>                 // for EncodeDestination
#include <primitives/transaction.h> // for CMutableTransaction
#include <rpc/protocol.h>           // for RPC_TRANSACTION_REJECTED
#include <rpc/util.h>               // for HelpExampleCli/HelpExampleRpc
#include <util/validation.h>        // for FormatStateMessage
#include <validation.h>             // for mempool
#include <vbk/util.hpp>
#include <wallet/rpcwallet.h> // for GetWalletForJSONRPCRequest
#include <wallet/wallet.h>    // for CWallet

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

} // namespace

UniValue RpcServiceImpl::submitpop(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "submitpop ( atv vtbs )\n"
            "\nCreates and submits a POP transaction constructed from the provided ATV and VTBs.\n"
            "\nArguments:\n"
            "1. atv       (string, required) Hex-encoded ATV record.\n"
            "2. vtbs      (array, required) Array of hex-encoded VTB records.\n"
            "\nResult:\n"
            "             (string) Transaction hash\n"
            "\nExamples:\n" +
            HelpExampleCli("submitpop", "ATV_HEX [VTB_HEX VTB_HEX]") + HelpExampleRpc("submitpop", "ATV_HEX, [VTB_HEX, VTB_HEX]"));

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR});

    LogPrintf("submitpop executed with: \n");
    const UniValue& vtb_array = request.params[1].get_array();
    std::vector<std::vector<uint8_t>> vtbs;
    for (uint32_t idx = 0u, size = vtb_array.size(); idx < size; ++idx) {
        LogPrintf(" - VTB: %s\n", vtb_array[idx].get_str());
        vtbs.emplace_back(ParseHexV(vtb_array[idx], "vtb[" + std::to_string(idx) + "]"));
    }

    LogPrintf(" - ATV: %s\n", request.params[0].get_str());
    auto atv = ParseHexV(request.params[0], "atv");

    return doSubmitPop(atv, vtbs);
}

UniValue RpcServiceImpl::getpopdata(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "getpopdata block_height\n"
            "\nFetches the data relevant to POP-mining the given block.\n"
            "\nArguments:\n"
            "1. block_height         (numeric, required) The height index\n"
            "\nResult:\n"
            "{\n"
            "    \"block_header\" : \"block_header_hex\",  (string) Hex-encoded block header\n"
            "    \"raw_contextinfocontainer\" : \"contextinfocontainer\",  (string) Hex-encoded raw authenticated ContextInfoContainer structure\n"
            "    \"first_address\" : \"address\",  (string) The first address in the address book of the wallet\n"
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
    return this->doGetPopData(height, wallet);
}

UniValue RpcServiceImpl::doGetPopData(int height, const std::shared_ptr<CWallet>& wallet)
{
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

    auto& pop_service = getService<PopService>();
    auto& util_service = getService<UtilService>();
    //context info
    uint256 txRoot = BlockMerkleRoot(block);
    auto keystones = util_service.getKeystoneHashesForTheNextBlock(pBlockIndex->pprev);
    auto contextInfo = VeriBlock::ContextInfoContainer(pBlockIndex->nHeight, keystones, txRoot);
    auto authedContext = contextInfo.getAuthenticated();
    result.pushKV("raw_contextinfocontainer", HexStr(authedContext.begin(), authedContext.end()));


    //first address in the wallet
    const auto& addressBook = wallet->mapAddressBook;
    if (!addressBook.empty()) {
        result.pushKV("first_address",
            EncodeDestination(addressBook.cbegin()->first));
    }

    auto lastVBKBlocks = pop_service.getLastKnownVBKBlocks(16);

    UniValue univalueLastVBKBlocks(UniValue::VARR);
    for (const auto& b : lastVBKBlocks) {
        univalueLastVBKBlocks.push_back(HexStr(b));
    }
    result.pushKV("last_known_veriblock_blocks", univalueLastVBKBlocks);

    auto lastBTCBlocks = pop_service.getLastKnownBTCBlocks(512);
    UniValue univalueLastBTCBlocks(UniValue::VARR);
    for (const auto& block : lastBTCBlocks) {
        univalueLastBTCBlocks.push_back(HexStr(block));
    }
    result.pushKV("last_known_bitcoin_blocks", univalueLastBTCBlocks);

    return result;
}

UniValue RpcServiceImpl::doSubmitPop(const std::vector<uint8_t>& atv,
    const std::vector<std::vector<uint8_t>>& vtbs)
{
    LOCK(cs_main);

    CMutableTransaction tx;

    tx.vout.resize(1);
    tx.vout[0].nValue = 0;
    tx.vout[0].scriptPubKey << OP_RETURN;

    tx.vin.resize(1);
    VeriBlock::setVBKNoInput(tx.vin[0].prevout);

    tx.vin[0].scriptSig << atv << OP_CHECKATV;
    for (const auto& vtb : vtbs) {
        tx.vin[0].scriptSig << vtb << OP_CHECKVTB;
    }

    tx.vin[0].scriptSig << OP_CHECKPOP;

    assert(VeriBlock::isPopTx(CTransaction(tx)));

    const uint256& hashTx = tx.GetHash();
    if (!::mempool.exists(hashTx)) {
        CValidationState state;
        bool fMissingInputs;
        auto tx_ref = MakeTransactionRef<const CMutableTransaction&>(tx);

        auto result = AcceptToMemoryPool(mempool, state, tx_ref, &fMissingInputs,
            nullptr /* plTxnReplaced */, false /* bypass_limits */,
            0 /* nAbsurdFee */);
        if (result) {
            return hashTx.GetHex();
        }

        if (state.IsInvalid()) {
            throw JSONRPCError(RPC_TRANSACTION_REJECTED, FormatStateMessage(state));
        }

        if (fMissingInputs) {
            throw JSONRPCError(RPC_TRANSACTION_ERROR, "Missing inputs");
        }

        throw JSONRPCError(RPC_TRANSACTION_ERROR, FormatStateMessage(state));
    }

    return hashTx.GetHex();
}

} // namespace VeriBlock