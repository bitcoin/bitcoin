// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rest.h>

#include <blockfilter.h>
#include <chain.h>
#include <chainparams.h>
#include <core_io.h>
#include <httpserver.h>
#include <index/blockfilterindex.h>
#include <index/txindex.h>
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
#include <util/system.h>
#include <validation.h>
#include <version.h>

#include <any>
#include <string>

#include <univalue.h>

using node::GetTransaction;
using node::NodeContext;
using node::ReadBlockFromDisk;

static const size_t MAX_GETUTXOS_OUTPOINTS = 15; //allow a max of 15 outpoints to be queried at once
static constexpr unsigned int MAX_REST_HEADERS_RESULTS = 2000;

static const struct {
    RESTResponseFormat rf;
    const char* name;
} rf_names[] = {
      {RESTResponseFormat::UNDEF, ""},
      {RESTResponseFormat::BINARY, "bin"},
      {RESTResponseFormat::HEX, "hex"},
      {RESTResponseFormat::JSON, "json"},
};

struct CCoin {
    uint32_t nHeight;
    // SYSCOIN
    CTxOutCoin out;

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
static NodeContext* GetNodeContext(const std::any& context, HTTPRequest* req)
{
    auto node_context = util::AnyPtr<NodeContext>(context);
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
static CTxMemPool* GetMemPool(const std::any& context, HTTPRequest* req)
{
    auto node_context = util::AnyPtr<NodeContext>(context);
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
static ChainstateManager* GetChainman(const std::any& context, HTTPRequest* req)
{
    auto node_context = util::AnyPtr<NodeContext>(context);
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

RESTResponseFormat ParseDataFormat(std::string& param, const std::string& strReq)
{
    // Remove query string (if any, separated with '?') as it should not interfere with
    // parsing param and data format
    param = strReq.substr(0, strReq.rfind('?'));
    const std::string::size_type pos_format{param.rfind('.')};

    // No format string is found
    if (pos_format == std::string::npos) {
        return rf_names[0].rf;
    }

    // Match format string to available formats
    const std::string suffix(param, pos_format + 1);
    for (const auto& rf_name : rf_names) {
        if (suffix == rf_name.name) {
            param.erase(pos_format);
            return rf_name.rf;
        }
    }

    // If no suffix is found, return RESTResponseFormat::UNDEF and original string without query string
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

static bool rest_headers(const std::any& context,
                         HTTPRequest* req,
                         const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RESTResponseFormat rf = ParseDataFormat(param, strURIPart);
    std::vector<std::string> path = SplitString(param, '/');

    std::string raw_count;
    std::string hashStr;
    if (path.size() == 2) {
        // deprecated path: /rest/headers/<count>/<hash>
        hashStr = path[1];
        raw_count = path[0];
    } else if (path.size() == 1) {
        // new path with query parameter: /rest/headers/<hash>?count=<count>
        hashStr = path[0];
        raw_count = req->GetQueryParameter("count").value_or("5");
    } else {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid URI format. Expected /rest/headers/<hash>.<ext>?count=<count>");
    }

    const auto parsed_count{ToIntegral<size_t>(raw_count)};
    if (!parsed_count.has_value() || *parsed_count < 1 || *parsed_count > MAX_REST_HEADERS_RESULTS) {
        return RESTERR(req, HTTP_BAD_REQUEST, strprintf("Header count is invalid or out of acceptable range (1-%u): %s", MAX_REST_HEADERS_RESULTS, raw_count));
    }

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
    case RESTResponseFormat::BINARY: {
        CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
        for (const CBlockIndex *pindex : headers) {
            ssHeader << pindex->GetBlockHeader(Params().GetConsensus());
        }

        std::string binaryHeader = ssHeader.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryHeader);
        return true;
    }

    case RESTResponseFormat::HEX: {
        CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
        for (const CBlockIndex *pindex : headers) {
            ssHeader << pindex->GetBlockHeader(Params().GetConsensus());
        }

        std::string strHex = HexStr(ssHeader) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }
    case RESTResponseFormat::JSON: {
        UniValue jsonHeaders(UniValue::VARR);
        for (const CBlockIndex *pindex : headers) {
            jsonHeaders.push_back(blockheaderToJSON(tip, pindex));
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

static bool rest_block(const std::any& context,
                       HTTPRequest* req,
                       const std::string& strURIPart,
                       TxVerbosity tx_verbosity)
{
    if (!CheckWarmup(req))
        return false;
    std::string hashStr;
    const RESTResponseFormat rf = ParseDataFormat(hashStr, strURIPart);

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

    if (!ReadBlockFromDisk(block, pblockindex, chainman.GetParams().GetConsensus())) {
        return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf) {
    case RESTResponseFormat::BINARY: {
        // SYSCOIN
        CDataStream ssBlock(SER_DISK, PROTOCOL_VERSION | RPCSerializationFlags());
        ssBlock << block;
        std::string binaryBlock = ssBlock.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryBlock);
        return true;
    }

    case RESTResponseFormat::HEX: {
        // SYSCOIN
        CDataStream ssBlock(SER_DISK, PROTOCOL_VERSION | RPCSerializationFlags());
        ssBlock << block;
        std::string strHex = HexStr(ssBlock) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }

    case RESTResponseFormat::JSON: {
        UniValue objBlock = blockToJSON(chainman.m_blockman, block, tip, pblockindex, tx_verbosity);
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

static bool rest_block_extended(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    return rest_block(context, req, strURIPart, TxVerbosity::SHOW_DETAILS_AND_PREVOUT);
}

static bool rest_block_notxdetails(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    return rest_block(context, req, strURIPart, TxVerbosity::SHOW_TXID);
}

static bool rest_filter_header(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req)) return false;

    std::string param;
    const RESTResponseFormat rf = ParseDataFormat(param, strURIPart);

    std::vector<std::string> uri_parts = SplitString(param, '/');
    std::string raw_count;
    std::string raw_blockhash;
    if (uri_parts.size() == 3) {
        // deprecated path: /rest/blockfilterheaders/<filtertype>/<count>/<blockhash>
        raw_blockhash = uri_parts[2];
        raw_count = uri_parts[1];
    } else if (uri_parts.size() == 2) {
        // new path with query parameter: /rest/blockfilterheaders/<filtertype>/<blockhash>?count=<count>
        raw_blockhash = uri_parts[1];
        raw_count = req->GetQueryParameter("count").value_or("5");
    } else {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid URI format. Expected /rest/blockfilterheaders/<filtertype>/<blockhash>.<ext>?count=<count>");
    }

    const auto parsed_count{ToIntegral<size_t>(raw_count)};
    if (!parsed_count.has_value() || *parsed_count < 1 || *parsed_count > MAX_REST_HEADERS_RESULTS) {
        return RESTERR(req, HTTP_BAD_REQUEST, strprintf("Header count is invalid or out of acceptable range (1-%u): %s", MAX_REST_HEADERS_RESULTS, raw_count));
    }

    uint256 block_hash;
    if (!ParseHashStr(raw_blockhash, block_hash)) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + raw_blockhash);
    }

    BlockFilterType filtertype;
    if (!BlockFilterTypeByName(uri_parts[0], filtertype)) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Unknown filtertype " + uri_parts[0]);
    }

    BlockFilterIndex* index = GetBlockFilterIndex(filtertype);
    if (!index) {
        return RESTERR(req, HTTP_BAD_REQUEST, "Index is not enabled for filtertype " + uri_parts[0]);
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
    case RESTResponseFormat::BINARY: {
        DataStream ssHeader{};
        for (const uint256& header : filter_headers) {
            ssHeader << header;
        }

        std::string binaryHeader = ssHeader.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryHeader);
        return true;
    }
    case RESTResponseFormat::HEX: {
        DataStream ssHeader{};
        for (const uint256& header : filter_headers) {
            ssHeader << header;
        }

        std::string strHex = HexStr(ssHeader) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }
    case RESTResponseFormat::JSON: {
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

static bool rest_block_filter(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req)) return false;

    std::string param;
    const RESTResponseFormat rf = ParseDataFormat(param, strURIPart);

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
    case RESTResponseFormat::BINARY: {
        DataStream ssResp{};
        ssResp << filter;

        std::string binaryResp = ssResp.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryResp);
        return true;
    }
    case RESTResponseFormat::HEX: {
        DataStream ssResp{};
        ssResp << filter;

        std::string strHex = HexStr(ssResp) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }
    case RESTResponseFormat::JSON: {
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

static bool rest_chaininfo(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RESTResponseFormat rf = ParseDataFormat(param, strURIPart);

    switch (rf) {
    case RESTResponseFormat::JSON: {
        node::JSONRPCRequest jsonRequest;
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


RPCHelpMan getdeploymentinfo();

static bool rest_deploymentinfo(const std::any& context, HTTPRequest* req, const std::string& str_uri_part)
{
    if (!CheckWarmup(req)) return false;

    std::string hash_str;
    const RESTResponseFormat rf = ParseDataFormat(hash_str, str_uri_part);

    switch (rf) {
    case RESTResponseFormat::JSON: {
        node::JSONRPCRequest jsonRequest;
        jsonRequest.context = context;
        jsonRequest.params = UniValue(UniValue::VARR);

        if (!hash_str.empty()) {
            uint256 hash;
            if (!ParseHashStr(hash_str, hash)) {
                return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hash_str);
            }

            const ChainstateManager* chainman = GetChainman(context, req);
            if (!chainman) return false;
            if (!WITH_LOCK(::cs_main, return chainman->m_blockman.LookupBlockIndex(ParseHashV(hash_str, "blockhash")))) {
                return RESTERR(req, HTTP_BAD_REQUEST, "Block not found");
            }

            jsonRequest.params.pushKV("blockhash", hash_str);
        }

        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, getdeploymentinfo().HandleRequest(jsonRequest).write() + "\n");
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
    }
    }

}

static bool rest_mempool(const std::any& context, HTTPRequest* req, const std::string& str_uri_part)
{
    if (!CheckWarmup(req))
        return false;

    std::string param;
    const RESTResponseFormat rf = ParseDataFormat(param, str_uri_part);
    if (param != "contents" && param != "info") {
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid URI format. Expected /rest/mempool/<info|contents>.json");
    }

    const CTxMemPool* mempool = GetMemPool(context, req);
    if (!mempool) return false;

    switch (rf) {
    case RESTResponseFormat::JSON: {
        std::string str_json;
        if (param == "contents") {
            const std::string raw_verbose{req->GetQueryParameter("verbose").value_or("true")};
            if (raw_verbose != "true" && raw_verbose != "false") {
                return RESTERR(req, HTTP_BAD_REQUEST, "The \"verbose\" query parameter must be either \"true\" or \"false\".");
            }
            const std::string raw_mempool_sequence{req->GetQueryParameter("mempool_sequence").value_or("false")};
            if (raw_mempool_sequence != "true" && raw_mempool_sequence != "false") {
                return RESTERR(req, HTTP_BAD_REQUEST, "The \"mempool_sequence\" query parameter must be either \"true\" or \"false\".");
            }
            const bool verbose{raw_verbose == "true"};
            const bool mempool_sequence{raw_mempool_sequence == "true"};
            if (verbose && mempool_sequence) {
                return RESTERR(req, HTTP_BAD_REQUEST, "Verbose results cannot contain mempool sequence values. (hint: set \"verbose=false\")");
            }
            str_json = MempoolToJSON(*mempool, verbose, mempool_sequence).write() + "\n";
        } else {
            str_json = MempoolInfoToJSON(*mempool).write() + "\n";
        }

        req->WriteHeader("Content-Type", "application/json");
        req->WriteReply(HTTP_OK, str_json);
        return true;
    }
    default: {
        return RESTERR(req, HTTP_NOT_FOUND, "output format not found (available: json)");
    }
    }
}

static bool rest_tx(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string hashStr;
    const RESTResponseFormat rf = ParseDataFormat(hashStr, strURIPart);
    CBlockIndex* blockindex = nullptr;
    const NodeContext* const node = GetNodeContext(context, req);
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        return RESTERR(req, HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }
    // SYSCOIN
    else{
        uint32_t nBlockHeight;
        if(pblockindexdb->ReadBlockHeight(hash, nBlockHeight)) {
            LOCK(cs_main);
            blockindex = node->chainman->ActiveChain()[nBlockHeight];
        }
    }

    if (!node) return false;
    uint256 hashBlock = uint256();
    // SYSCOIN
    const CTransactionRef tx = GetTransaction(blockindex, node->mempool.get(), hash, Params().GetConsensus(), hashBlock);
    if (!tx) {
        return RESTERR(req, HTTP_NOT_FOUND, hashStr + " not found");
    }

    switch (rf) {
    case RESTResponseFormat::BINARY: {
        // SYSCOIN
        CDataStream ssTx(SER_DISK, PROTOCOL_VERSION | RPCSerializationFlags());
        ssTx << tx;

        std::string binaryTx = ssTx.str();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, binaryTx);
        return true;
    }

    case RESTResponseFormat::HEX: {
        // SYSCOIN
        CDataStream ssTx(SER_DISK, PROTOCOL_VERSION | RPCSerializationFlags());
        ssTx << tx;

        std::string strHex = HexStr(ssTx) + "\n";
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }

    case RESTResponseFormat::JSON: {
        UniValue objTx(UniValue::VOBJ);
        TxToUniv(*tx, /*block_hash=*/hashBlock, /*entry=*/ objTx);
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

static bool rest_getutxos(const std::any& context, HTTPRequest* req, const std::string& strURIPart)
{
    if (!CheckWarmup(req))
        return false;
    std::string param;
    const RESTResponseFormat rf = ParseDataFormat(param, strURIPart);

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
    case RESTResponseFormat::HEX: {
        // convert hex to bin, continue then with bin part
        std::vector<unsigned char> strRequestV = ParseHex(strRequestMutable);
        strRequestMutable.assign(strRequestV.begin(), strRequestV.end());
        [[fallthrough]];
    }

    case RESTResponseFormat::BINARY: {
        try {
            //deserialize only if user sent a request
            if (strRequestMutable.size() > 0)
            {
                if (fInputParsed) //don't allow sending input over URI and HTTP RAW DATA
                    return RESTERR(req, HTTP_BAD_REQUEST, "Combination of URI scheme inputs and raw post data is not allowed");

                DataStream oss{};
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

    case RESTResponseFormat::JSON: {
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
    decltype(chainman.ActiveHeight()) active_height;
    uint256 active_hash;
    {
        auto process_utxos = [&vOutPoints, &outs, &hits, &active_height, &active_hash, &chainman](const CCoinsView& view, const CTxMemPool* mempool) EXCLUSIVE_LOCKS_REQUIRED(chainman.GetMutex()) {
            for (const COutPoint& vOutPoint : vOutPoints) {
                Coin coin;
                bool hit = (!mempool || !mempool->isSpent(vOutPoint)) && view.GetCoin(vOutPoint, coin);
                hits.push_back(hit);
                if (hit) outs.emplace_back(std::move(coin));
            }
            active_height = chainman.ActiveHeight();
            active_hash = chainman.ActiveTip()->GetBlockHash();
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
    case RESTResponseFormat::BINARY: {
        // serialize data
        // use exact same output as mentioned in Bip64
        DataStream ssGetUTXOResponse{};
        ssGetUTXOResponse << active_height << active_hash << bitmap << outs;
        std::string ssGetUTXOResponseString = ssGetUTXOResponse.str();

        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, ssGetUTXOResponseString);
        return true;
    }

    case RESTResponseFormat::HEX: {
        DataStream ssGetUTXOResponse{};
        ssGetUTXOResponse << active_height << active_hash << bitmap << outs;
        std::string strHex = HexStr(ssGetUTXOResponse) + "\n";

        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, strHex);
        return true;
    }

    case RESTResponseFormat::JSON: {
        UniValue objGetUTXOResponse(UniValue::VOBJ);

        // pack in some essentials
        // use more or less the same output as mentioned in Bip64
        objGetUTXOResponse.pushKV("chainHeight", active_height);
        objGetUTXOResponse.pushKV("chaintipHash", active_hash.GetHex());
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

static bool rest_blockhash_by_height(const std::any& context, HTTPRequest* req,
                       const std::string& str_uri_part)
{
    if (!CheckWarmup(req)) return false;
    std::string height_str;
    const RESTResponseFormat rf = ParseDataFormat(height_str, str_uri_part);

    int32_t blockheight = -1; // Initialization done only to prevent valgrind false positive, see https://github.com/syscoin/syscoin/pull/18785
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
    case RESTResponseFormat::BINARY: {
        DataStream ss_blockhash{};
        ss_blockhash << pblockindex->GetBlockHash();
        req->WriteHeader("Content-Type", "application/octet-stream");
        req->WriteReply(HTTP_OK, ss_blockhash.str());
        return true;
    }
    case RESTResponseFormat::HEX: {
        req->WriteHeader("Content-Type", "text/plain");
        req->WriteReply(HTTP_OK, pblockindex->GetBlockHash().GetHex() + "\n");
        return true;
    }
    case RESTResponseFormat::JSON: {
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
    bool (*handler)(const std::any& context, HTTPRequest* req, const std::string& strReq);
} uri_prefixes[] = {
      {"/rest/tx/", rest_tx},
      {"/rest/block/notxdetails/", rest_block_notxdetails},
      {"/rest/block/", rest_block_extended},
      {"/rest/blockfilter/", rest_block_filter},
      {"/rest/blockfilterheaders/", rest_filter_header},
      {"/rest/chaininfo", rest_chaininfo},
      {"/rest/mempool/", rest_mempool},
      {"/rest/headers/", rest_headers},
      {"/rest/getutxos", rest_getutxos},
      {"/rest/deploymentinfo/", rest_deploymentinfo},
      {"/rest/deploymentinfo", rest_deploymentinfo},
      {"/rest/blockhashbyheight/", rest_blockhash_by_height},
};

void StartREST(const std::any& context)
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
