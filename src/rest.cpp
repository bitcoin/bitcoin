// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <chainlock/chainlock.h>
#include <context.h>
#include <core_io.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/txindex.h>
#include <instantsend/instantsend.h>
#include <llmq/context.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/blockchain.h>
#include <rpc/mempool.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <streams.h>
#include <sync.h>
#include <txmempool.h>
#include <util/check.h>
#include <validation.h>
#include <version.h>

#include <univalue.h>

using node::GetTransaction;
using node::NodeContext;
using node::ReadBlockFromDisk;

static const size_t MAX_GETUTXOS_OUTPOINTS = 15; //allow a max of 15 outpoints to be queried at once
static constexpr unsigned int MAX_REST_HEADERS_RESULTS = 2000;

enum class RetFormat {
    UNDEF,
    BINARY,
    HEX,
    JSON,
};

static const struct {
    RetFormat rf;
    const char* name;
} rf_names[] = {
      {RetFormat::UNDEF, ""},
      {RetFormat::BINARY, "bin"},
      {RetFormat::HEX, "hex"},
      {RetFormat::JSON, "json"},
};

struct CCoin {
    uint32_t nHeight;
    CTxOut out;

    CCoin() : nHeight(0) {}
    explicit CCoin(Coin&& in) : nHeight(in.nHeight), out(std::move(in.out)) {}

    SERIALIZE_METHODS(CCoin, obj)
    {
        uint32_t nTxVerDummy = 0;
        READWRITE(nTxVerDummy, obj.nHeight, obj.out);
    }
};

static bool RESTERR(HTTPRequest* req, enum HTTPStatusCode status, std::string message)
{
    req->WriteHeader("Content-Type", "text/plain");
    req->WriteReply(status, message + "\r\n");
    return false;
}

/**
 * Get the node context.
 *
 * @param[in]  req  The HTTP request, whose status code will be set if node
 *                  context is not found.
 * @returns         Pointer to the node context or nullptr if not found.
 */
static NodeContext* GetNodeContext(const CoreContext& context, HTTPRequest* req)
{
    auto* node_context = GetContext<NodeContext>(context);
    if (!node_context) {
        RESTERR(req, HTTP_INTERNAL_SERVER_ERROR,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: Node context not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node_context;
}

/**
 * Get the node context mempool.
 *
 * @param[in]  req The HTTP request, whose status code will be set if node
 *                 context mempool is not found.
 * @returns        Pointer to the mempool or nullptr if no mempool found.
 */
static CTxMemPool* GetMemPool(const CoreContext& context, HTTPRequest* req)
{
    auto* node_context = GetContext<NodeContext>(context);
    if (!node_context || !node_context->mempool) {
        RESTERR(req, HTTP_NOT_FOUND, "Mempool disabled or instance not found");
        return nullptr;
    }
    return node_context->mempool.get();
}

/**
 * Get the node context chainstatemanager.
 *
 * @param[in]  req The HTTP request, whose status code will be set if node
 *                 context chainstatemanager is not found.
 * @returns        Pointer to the chainstatemanager or nullptr if none found.
 */
static ChainstateManager* GetChainman(const CoreContext& context, HTTPRequest* req)
{
    auto node_context = GetContext<NodeContext>(context);
    if (!node_context || !node_context->chainman) {
        RESTERR(req, HTTP_INTERNAL_SERVER_ERROR,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: Chainman disabled or instance not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node_context->chainman.get();
}

/**
 * Get the node context LLMQContext.
 *
 * @param[in]  req The HTTP request, whose status code will be set if node
 *                 context LLMQContext is not found.
 * @returns        Pointer to the LLMQContext or nullptr if none found.
 */
static LLMQContext* GetLLMQContext(const CoreContext& context, HTTPRequest* req)
{
    auto node_context = GetContext<NodeContext>(context);
    if (!node_context || !node_context->llmq_ctx) {
        RESTERR(req, HTTP_INTERNAL_SERVER_ERROR,
                strprintf("%s:%d (%s)\n"
                          "Internal bug detected: LLMQ context not found!\n"
                          "You may report this issue here: %s\n",
                          __FILE__, __LINE__, __func__, PACKAGE_BUGREPORT));
        return nullptr;
    }
    return node_context->llmq_ctx.get();
}

static RetFormat ParseDataFormat(std::string& param, const std::string& strReq)
{
    const std::string::size_type pos = strReq.rfind('.');
    if (pos == std::string::npos)
    {
        param = strReq;
        return rf_names[0].rf;
    }

    param = strReq.substr(0, pos);
    const std::string suff(strReq, pos + 1);

    for (const auto& rf_name : rf_names) {
        if (suff == rf_name.name)
            return rf_name.rf;
    }

    /* If no suffix is found, return original string.  */
    param = strReq;
    return rf_names[0].rf;
}

static std::string AvailableDataFormatsString()
{
    std::string formats;
    for (const auto& rf_name : rf_names) {
        if (strlen(rf_name.name) > 0) {
            formats.append(".");
            formats.append(rf_name.name);
            formats.append(", ");
        }
    }

    if (formats.length() > 0)
        return formats.substr(0, formats.length() - 2);

    return formats;
}

static bool CheckWarmup(HTTPRequest* req)
{
    std::string statusmessage;
    if (RPCIsInWarmup(&statusmessage))
         return RESTERR(req, HTTP_SERVICE_UNAVAILABLE, "Service temporarily unavailable: " + statusmessage);
    return true;
}

static bool rest_headers(const CoreContext& context,
                         HTTPRequest* req,
                         const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);
    std::vector<std::string> path = SplitString(param, '/');

    if (path.size() != 2)
        return RESTERR(req, HTTP_BAD_REQUEST, "No header count specified. Use /rest/headers/<count>/<hash>.<ext>.");

    const auto parsed_count{ToIntegral<size_t>(path[0])};
    if (!parsed_count.has_value() || *parsed_count < 1 || *parsed_count > MAX_REST_HEADERS_RESULTS) {
        return RESTERR(req, HTTP_BAD_REQUEST, strprintf("Header count is invalid or out of acceptable range (1-%u): %s", MAX_REST_HEADERS_RESULTS, path[0]));
    }

    std::string hashStr = path[1];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    const CBlockIndex* tip = nullptr;
    std::vector<const CBlockIndex*> headers;
    headers.reserve(*parsed_count);
    {
        ChainstateManager* maybe_chainman = GetChainman(context, req);
        if (!maybe_chainman) return false;
        ChainstateManager& chainman = *maybe_chainman;
        LOCK(cs_main);
        CChain& active_chain = chainman.ActiveChain();
        tip = active_chain.Tip();
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(hash);
        while (pindex != nullptr && active_chain.Contains(pindex)) {
            headers.push_back(pindex);
            if (headers.size() == *parsed_count) {
                break;
            }
            pindex = active_chain.Next(pindex);
        }
    }

    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
        for (const CBlockIndex *pindex : headers) {
            ssHeader << pindex->GetBlockHeader();
        }

        std::string binaryHeader = ssHeader.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryHeader);
        return true;
    }

    case RetFormat::HEX: {
        CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
        for (const CBlockIndex *pindex : headers) {
            ssHeader << pindex->GetBlockHeader();
        }

        std::string strHex = HexStr(ssHeader) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }
    case RetFormat::JSON: {
        const LLMQContext* llmq_ctx = GetLLMQContext(context, req);
        if (!llmq_ctx) return false;

        UniValue jsonHeaders(UniValue::VARR);
        for (const CBlockIndex *pindex : headers) {
            jsonHeaders.push_back(blockheaderToJSON(tip, pindex, *llmq_ctx->clhandler));
        }
        std::string strJSON = jsonHeaders.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static bool rest_block(const CoreContext& context,
                       HTTPRequest* req,
                       const std::string& strURIPart,
                       TxVerbosity tx_verbosity)
{
    if (!CheckWarmup(req))
        return false;
    std::string hashStr;
    const RetFormat rf = ParseDataFormat(hashStr, strURIPart);

    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    CBlock block;
    const CBlockIndex* pblockindex = nullptr;
    const CBlockIndex* tip = nullptr;
    ChainstateManager* maybe_chainman = GetChainman(context, req);
    if (!maybe_chainman) return false;
    ChainstateManager& chainman = *maybe_chainman;
    {
        LOCK(cs_main);
        tip = chainman.ActiveChain().Tip();
        pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pblockindex) {
            return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
        }

        if (chainman.m_blockman.IsBlockPruned(pblockindex))
            return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not available (pruned data)");
    }

    if (!ReadBlockFromDisk(block, pblockindex, Params().GetConsensus())) {
        return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string binaryBlock = ssBlock.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryBlock);
        return true;
    }

    case RetFormat::HEX: {
        CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
        ssBlock << block;
        std::string strHex = HexStr(ssBlock) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }

    case RetFormat::JSON: {
        const LLMQContext* llmq_ctx = GetLLMQContext(context, req);
        if (!llmq_ctx) return false;

        UniValue objBlock = blockToJSON(chainman.m_blockman, block, tip, pblockindex, *llmq_ctx->clhandler, *llmq_ctx->isman, tx_verbosity);
        std::string strJSON = objBlock.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }

    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static bool rest_block_extended(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    return rest_block(context, req, strURIPart, TxVerbosity::SHOW_DETAILS_AND_PREVOUT);
}

static bool rest_block_notxdetails(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    return rest_block(context, req, strURIPart, TxVerbosity::SHOW_TXID);
}

static bool rest_filter_header(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req)) return false;

    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    std::vector<std::string> uri_parts = SplitString(param, '/');
    if (uri_parts.size() != 3) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid URI format. Expected /rest/blockfilterheaders/<filtertype>/<count>/<blockhash>");
    }

    uint256 block_hash;
    if (!ParseHashStr(uri_parts[2], block_hash)) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + uri_parts[2]);
    }

    BlockFilterType filtertype;
    if (!BlockFilterTypeByName(uri_parts[0], filtertype)) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Unknown filtertype " + uri_parts[0]);
    }

    BlockFilterIndex* index = GetBlockFilterIndex(filtertype);
    if (!index) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Index is not enabled for filtertype " + uri_parts[0]);
    }

    const auto parsed_count{ToIntegral<size_t>(uri_parts[1])};
    if (!parsed_count.has_value() || *parsed_count < 1 || *parsed_count > MAX_REST_HEADERS_RESULTS) {
        return RESTERR(req, HTTP_BAD_REQUEST, strprintf("Header count is invalid or out of acceptable range (1-%u): %s", MAX_REST_HEADERS_RESULTS, uri_parts[1]));
    }

    std::vector<const CBlockIndex*> headers;
    headers.reserve(*parsed_count);
    {
        ChainstateManager* maybe_chainman = GetChainman(context, req);
        if (!maybe_chainman) return false;
        ChainstateManager& chainman = *maybe_chainman;
        LOCK(cs_main);
        CChain& active_chain = chainman.ActiveChain();
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(block_hash);
        while (pindex != nullptr && active_chain.Contains(pindex)) {
            headers.push_back(pindex);
            if (headers.size() == *parsed_count)
                break;
            pindex = active_chain.Next(pindex);
        }
    }

    bool index_ready = index->BlockUntilSyncedToCurrentChain();

    std::vector<uint256> filter_headers;
    filter_headers.reserve(*parsed_count);
    for (const CBlockIndex* pindex : headers) {
        uint256 filter_header;
        if (!index->LookupFilterHeader(pindex, filter_header)) {
            std::string errmsg = "Filter not found.";

            if (!index_ready) {
                errmsg += " Block filters are still in the process of being indexed.";
            } else {
                errmsg += " This error is unexpected and indicates index corruption.";
            }

            return RESTERR(req, HTTP_NOT_FOUND, errmsg);
        }
        filter_headers.push_back(filter_header);
    }

    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ssHeader{SER_NETWORK, PROTOCOL_VERSION};
        for (const uint256& header : filter_headers) {
            ssHeader << header;
        }

        std::string binaryHeader = ssHeader.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryHeader);
        return true;
    }
    case RetFormat::HEX: {
        CDataStream ssHeader{SER_NETWORK, PROTOCOL_VERSION};
        for (const uint256& header : filter_headers) {
            ssHeader << header;
        }

        std::string strHex = HexStr(ssHeader) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }
    case RetFormat::JSON: {
        UniValue jsonHeaders(UniValue::VARR);
        for (const uint256& header : filter_headers) {
            jsonHeaders.push_back(header.GetHex());
        }

        std::string strJSON = jsonHeaders.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static bool rest_block_filter(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req)) return false;

    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    // request is sent over URI scheme /rest/blockfilter/filtertype/blockhash
    std::vector<std::string> uri_parts = SplitString(param, '/');
    if (uri_parts.size() != 2) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid URI format. Expected /rest/blockfilter/<filtertype>/<blockhash>");
    }

    uint256 block_hash;
    if (!ParseHashStr(uri_parts[1], block_hash)) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + uri_parts[1]);
    }

    BlockFilterType filtertype;
    if (!BlockFilterTypeByName(uri_parts[0], filtertype)) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Unknown filtertype " + uri_parts[0]);
    }

    BlockFilterIndex* index = GetBlockFilterIndex(filtertype);
    if (!index) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Index is not enabled for filtertype " + uri_parts[0]);
    }

    const CBlockIndex* block_index;
    bool block_was_connected;
    {
        ChainstateManager* maybe_chainman = GetChainman(context, req);
        if (!maybe_chainman) return false;
        ChainstateManager& chainman = *maybe_chainman;
        LOCK(cs_main);
        block_index = chainman.m_blockman.LookupBlockIndex(block_hash);
        if (!block_index) {
            return RESTERR(req, HTTP_NOT_FOUND, uri_parts[1] + " not found");
        }
        block_was_connected = block_index->IsValid(BLOCK_VALID_SCRIPTS);
    }

    bool index_ready = index->BlockUntilSyncedToCurrentChain();

    BlockFilter filter;
    if (!index->LookupFilter(block_index, filter)) {
        std::string errmsg = "Filter not found.";

        if (!block_was_connected) {
            errmsg += " Block was not connected to active chain.";
        } else if (!index_ready) {
            errmsg += " Block filters are still in the process of being indexed.";
        } else {
            errmsg += " This error is unexpected and indicates index corruption.";
        }

        return RESTERR(req, HTTP_NOT_FOUND, errmsg);
    }

    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ssResp{SER_NETWORK, PROTOCOL_VERSION};
        ssResp << filter;

        std::string binaryResp = ssResp.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryResp);
        return true;
    }
    case RetFormat::HEX: {
        CDataStream ssResp{SER_NETWORK, PROTOCOL_VERSION};
        ssResp << filter;

        std::string strHex = HexStr(ssResp) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }
    case RetFormat::JSON: {
        UniValue ret(UniValue::VOBJ);
        ret.pushKV("filter", HexStr(filter.GetEncodedFilter()));
        std::string strJSON = ret.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

// A bit of a hack - dependency on a function defined in rpc/blockchain.cpp
RPCHelpMan getblockchaininfo();

static bool rest_chaininfo(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf) {
    case RetFormat::JSON: {
        JSONRPCRequest jsonRequest;
        jsonRequest.context = context;
        jsonRequest.params = UniValue(UniValue::VARR);
        UniValue chainInfoObject = getblockchaininfo().HandleRequest(jsonRequest);
        std::string strJSON = chainInfoObject.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
    }
    }
}

static bool rest_mempool_info(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    const CTxMemPool* mempool = GetMemPool(context, req);
    if (!mempool) return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf) {
    case RetFormat::JSON: {
        const LLMQContext* llmq_ctx = GetLLMQContext(context, req);
        if (!llmq_ctx) return false;

        UniValue mempoolInfoObject = MempoolInfoToJSON(*mempool, *llmq_ctx->isman);

        std::string strJSON = mempoolInfoObject.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
    }
    }
}

static bool rest_mempool_contents(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req)) return false;
    const CTxMemPool* mempool = GetMemPool(context, req);
    if (!mempool) return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf) {
    case RetFormat::JSON: {
        const LLMQContext* llmq_ctx = GetLLMQContext(context, req);
        if (!llmq_ctx) return false;

        UniValue mempoolObject = MempoolToJSON(*mempool, llmq_ctx->isman.get(), true);

        std::string strJSON = mempoolObject.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
    }
    }
}

static bool rest_tx(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string hashStr;
    const RetFormat rf = ParseDataFormat(hashStr, strURIPart);

    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    const NodeContext* const node = GetNodeContext(context, req);
    if (!node) return false;
    uint256 hashBlock = uint256();
    const CTransactionRef tx = GetTransaction(/* block_index */ nullptr, node->mempool.get(), hash, Params().GetConsensus(), hashBlock);
    if (!tx) {
        return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << tx;

        std::string binaryTx = ssTx.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryTx);
        return true;
    }

    case RetFormat::HEX: {
        CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
        ssTx << tx;

        std::string strHex = HexStr(ssTx) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }

    case RetFormat::JSON: {
        UniValue objTx(UniValue::VOBJ);
        TxToUniv(*tx, /*block_hash=*/hashBlock, /*entry=*/objTx);
        std::string strJSON = objTx.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }

    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static bool rest_getutxos(const CoreContext& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RetFormat rf = ParseDataFormat(param, strURIPart);

    std::vector<std::string> uriParts;
    if (param.length() > 1)
    {
        std::string strUriParams = param.substr(1);
        uriParts = SplitString(strUriParams, '/');
    }

    // throw exception in case of an empty request
    std::string strRequestMutable = req->ReadBody();
    if (strRequestMutable.length() == 0 && uriParts.size() == 0)
        return RESTERR(req, HTTP_BAD_REQUEST, "Error: empty request");

    bool fInputParsed = false;
    bool fCheckMemPool = false;
    std::vector<COutPoint> vOutPoints;

    // parse/deserialize input
    // input-format = output-format, rest/getutxos/bin requires binary input, gives binary output, ...

    if (uriParts.size() > 0)
    {
        //inputs is sent over URI scheme (/rest/getutxos/checkmempool/txid1-n/txid2-n/...)
        if (uriParts[0] == "checkmempool") fCheckMemPool = true;

        for (size_t i = (fCheckMemPool) ? 1 : 0; i < uriParts.size(); i++)
        {
            uint256 txid;
            int32_t nOutput;
            std::string strTxid = uriParts[i].substr(0, uriParts[i].find('-'));
            std::string strOutput = uriParts[i].substr(uriParts[i].find('-')+1);

            if (!ParseInt32(strOutput, &nOutput) || !IsHex(strTxid))
                return RESTERR(req, HTTP_BAD_REQUEST, "Parse error");

            txid.SetHex(strTxid);
            vOutPoints.push_back(COutPoint(txid, (uint32_t)nOutput));
        }

        if (vOutPoints.size() > 0)
            fInputParsed = true;
        else
            return RESTERR(req, HTTP_BAD_REQUEST, "Error: empty request");
    }

    switch (rf) {
    case RetFormat::HEX: {
        // convert hex to bin, continue then with bin part
        std::vector<unsigned char> strRequestV = ParseHex(strRequestMutable);
        strRequestMutable.assign(strRequestV.begin(), strRequestV.end());
        [[fallthrough]];
    }

    case RetFormat::BINARY: {
        try {
            //deserialize only if user sent a request
            if (strRequestMutable.size() > 0)
            {
                if (fInputParsed) //don't allow sending input over URI and HTTP RAW DATA
                    return RESTERR(req, HTTP_BAD_REQUEST, "Combination of URI scheme inputs and raw post data is not allowed");

                CDataStream oss(SER_NETWORK, PROTOCOL_VERSION);
                oss << strRequestMutable;
                oss >> fCheckMemPool;
                oss >> vOutPoints;
            }
        } catch (const std::ios_base::failure&) {
            // abort in case of unreadable binary data
            return RESTERR(req, HTTP_BAD_REQUEST, "Parse error");
        }
        break;
    }

    case RetFormat::JSON: {
        if (!fInputParsed)
            return RESTERR(req, HTTP_BAD_REQUEST, "Error: empty request");
        break;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }

    // limit max outpoints
    if (vOutPoints.size() > MAX_GETUTXOS_OUTPOINTS)
        return RESTERR(req, HTTP_BAD_REQUEST, strprintf("Error: max outpoints exceeded (max: %d, tried: %d)", MAX_GETUTXOS_OUTPOINTS, vOutPoints.size()));

    // check spentness and form a bitmap (as well as a JSON capable human-readable string representation)
    std::vector<unsigned char> bitmap;
    std::vector<CCoin> outs;
    std::string bitmapStringRepresentation;
    std::vector<bool> hits;
    bitmap.resize((vOutPoints.size() + 7) / 8);
    ChainstateManager* maybe_chainman = GetChainman(context, req);
    if (!maybe_chainman) return false;
    ChainstateManager& chainman = *maybe_chainman;
    {
        auto process_utxos = [&vOutPoints, &outs, &hits](const CCoinsView& view, const CTxMemPool* mempool) {
            for (const COutPoint& vOutPoint : vOutPoints) {
                Coin coin;
                bool hit = (!mempool || !mempool->isSpent(vOutPoint)) && view.GetCoin(vOutPoint, coin);
                hits.push_back(hit);
                if (hit) outs.emplace_back(std::move(coin));
            }
        };

        if (fCheckMemPool) {
            const CTxMemPool* mempool = GetMemPool(context, req);
            if (!mempool) return false;
            // use db+mempool as cache backend in case user likes to query mempool
            LOCK2(cs_main, mempool->cs);
            CCoinsViewCache& viewChain = chainman.ActiveChainstate().CoinsTip();
            CCoinsViewMemPool viewMempool(&viewChain, *mempool);
            process_utxos(viewMempool, mempool);
        } else {
            LOCK(cs_main);
            process_utxos(chainman.ActiveChainstate().CoinsTip(), nullptr);
        }

        for (size_t i = 0; i < hits.size(); ++i) {
            const bool hit = hits[i];
            bitmapStringRepresentation.append(hit ? "1" : "0"); // form a binary string representation (human-readable for json output)
            bitmap[i / 8] |= ((uint8_t)hit) << (i % 8);
        }
    }

    switch (rf) {
    case RetFormat::BINARY: {
        // serialize data
        // use exact same output as mentioned in Bip64
        CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
        ssGetUTXOResponse << chainman.ActiveChain().Height() << chainman.ActiveChain().Tip()->GetBlockHash() << bitmap << outs;
        std::string ssGetUTXOResponseString = ssGetUTXOResponse.str();

        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, ssGetUTXOResponseString);
        return true;
    }

    case RetFormat::HEX: {
        CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
        ssGetUTXOResponse << chainman.ActiveChain().Height() << chainman.ActiveChain().Tip()->GetBlockHash() << bitmap << outs;
        std::string strHex = HexStr(ssGetUTXOResponse) + "\n";

        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }

    case RetFormat::JSON: {
        UniValue objGetUTXOResponse(UniValue::VOBJ);

        // pack in some essentials
        // use more or less the same output as mentioned in Bip64
        objGetUTXOResponse.pushKV("chainHeight", chainman.ActiveChain().Height());
        objGetUTXOResponse.pushKV("chaintipHash", chainman.ActiveChain().Tip()->GetBlockHash().GetHex());
        objGetUTXOResponse.pushKV("bitmap", bitmapStringRepresentation);

        UniValue utxos(UniValue::VARR);
        for (const CCoin& coin : outs) {
            UniValue utxo(UniValue::VOBJ);
            utxo.pushKV("height", (int32_t)coin.nHeight);
            utxo.pushKV("value", ValueFromAmount(coin.out.nValue));

            // include the script in a json output
            UniValue o(UniValue::VOBJ);
            ScriptToUniv(coin.out.scriptPubKey, /*out=*/o, /*include_hex=*/true, /*include_address=*/true);
            utxo.pushKV("scriptPubKey", o);
            utxos.push_back(utxo);
        }
        objGetUTXOResponse.pushKV("utxos", utxos);

        // return json string
        std::string strJSON = objGetUTXOResponse.write() + "\n";
        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, strJSON);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static bool rest_blockhash_by_height(const CoreContext& context, HTTPRequest* req,
                       const std::string& str_uri_part)
{
    if (!CheckWarmup(req)) return false;
    std::string height_str;
    const RetFormat rf = ParseDataFormat(height_str, str_uri_part);

    int32_t blockheight = -1; // Initialization done only to prevent valgrind false positive, see https://github.com/bitcoin/bitcoin/pull/18785
    if (!ParseInt32(height_str, &blockheight) || blockheight < 0) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid height: " + SanitizeString(height_str));
    }

    CBlockIndex* pblockindex = nullptr;
    {
        ChainstateManager* maybe_chainman = GetChainman(context, req);
        if (!maybe_chainman) return false;
        ChainstateManager& chainman = *maybe_chainman;
        LOCK(cs_main);
        const CChain& active_chain = chainman.ActiveChain();
        if (blockheight > active_chain.Height()) {
            return RESTERR(req, HTTP_NOT_FOUND, "Block height out of range");
        }
        pblockindex = active_chain[blockheight];
    }
    switch (rf) {
    case RetFormat::BINARY: {
        CDataStream ss_blockhash(SER_NETWORK, PROTOCOL_VERSION);
        ss_blockhash << pblockindex->GetBlockHash();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, ss_blockhash.str());
        return true;
    }
    case RetFormat::HEX: {
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, pblockindex->GetBlockHash().GetHex() + "\n");
        return true;
    }
    case RetFormat::JSON: {
        req->WriteHeader("Content-Type", "application/json");
        UniValue resp = UniValue(UniValue::VOBJ);
        resp.pushKV("blockhash", pblockindex->GetBlockHash().GetHex());
        req->WriteReply(HTTP_OK, resp.write() + "\n");
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }
}

static const struct {
    const char* prefix;
    bool (*handler)(const CoreContext& context, HTTPRequest* req, const std::string& strReq);
} uri_prefixes[] = {
      {"/rest/tx/", rest_tx},
      {"/rest/block/notxdetails/", rest_block_notxdetails},
      {"/rest/block/", rest_block_extended},
      {"/rest/blockfilter/", rest_block_filter},
      {"/rest/blockfilterheaders/", rest_filter_header},
      {"/rest/chaininfo", rest_chaininfo},
      {"/rest/mempool/info", rest_mempool_info},
      {"/rest/mempool/contents", rest_mempool_contents},
      {"/rest/headers/", rest_headers},
      {"/rest/getutxos", rest_getutxos},
      {"/rest/blockhashbyheight/", rest_blockhash_by_height},
};

void StartREST(const CoreContext& context)
{
    for (const auto& up : uri_prefixes) {
        auto handler = [context, up](HTTPRequest* req, const std::string& prefix) { return up.handler(context, req, prefix); };
        RegisterHTTPHandler(up.prefix, false, handler);
    }
}

void InterruptREST()
{
}

void StopREST()
{
    for (const auto& up : uri_prefixes) {
        UnregisterHTTPHandler(up.prefix, false);
    }
}
