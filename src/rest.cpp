// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"
#include "primitives/transaction.h"
#include "main.h"
#include "rpcserver.h"
#include "streams.h"
#include "sync.h"
#include "txmempool.h"
#include "utilstrencodings.h"
#include "version.h"

#include <boost/algorithm/string.hpp>
#include <boost/dynamic_bitset.hpp>

using namespace std;
using namespace json_spirit;

static const int MAX_GETUTXOS_OUTPOINTS = 15; //allow a max of 15 outpoints to be queried at once

enum RetFormat {
    RF_UNDEF,
    RF_BINARY,
    RF_HEX,
    RF_JSON,
};

static const struct {
    enum RetFormat rf;
    const char* name;
} rf_names[] = {
      {RF_UNDEF, ""},
      {RF_BINARY, "bin"},
      {RF_HEX, "hex"},
      {RF_JSON, "json"},
};

struct CCoin {
    uint32_t nTxVer; // Don't call this nVersion, that name has a special meaning inside IMPLEMENT_SERIALIZE
    uint32_t nHeight;
    CTxOut out;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion)
    {
        READWRITE(nTxVer);
        READWRITE(nHeight);
        READWRITE(out);
    }
};

class RestErr
{
public:
    enum HTTPStatusCode status;
    string message;
};

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, Object& entry);
extern Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool txDetails = false);
extern void ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out, bool fIncludeHex);

static RestErr RESTERR(enum HTTPStatusCode status, string message)
{
    RestErr re;
    re.status = status;
    re.message = message;
    return re;
}

static enum RetFormat ParseDataFormat(vector<string>& params, const string strReq)
{
    boost::split(params, strReq, boost::is_any_of("."));
    if (params.size() > 1) {
        for (unsigned int i = 0; i < ARRAYLEN(rf_names); i++)
            if (params[1] == rf_names[i].name)
                return rf_names[i].rf;
    }

    return rf_names[0].rf;
}

static string AvailableDataFormatsString()
{
    string formats = "";
    for (unsigned int i = 0; i < ARRAYLEN(rf_names); i++)
        if (strlen(rf_names[i].name) > 0) {
            formats.append(".");
            formats.append(rf_names[i].name);
            formats.append(", ");
        }

    if (formats.length() > 0)
        return formats.substr(0, formats.length() - 2);

    return formats;
}

static bool ParseHashStr(const string& strReq, uint256& v)
{
    if (!IsHex(strReq) || (strReq.size() != 64))
        return false;

    v.SetHex(strReq);
    return true;
}

static bool rest_headers(AcceptedConnection* conn,
                         const std::string& strURIPart,
                         const std::string& strRequest,
                         const std::map<std::string, std::string>& mapHeaders,
                         bool fRun)
{
    vector<string> params;
    const RetFormat rf = ParseDataFormat(params, strURIPart);
    vector<string> path;
    boost::split(path, params[0], boost::is_any_of("/"));

    if (path.size() != 2)
        throw RESTERR(HTTP_BAD_REQUEST, "No header count specified. Use /rest/headers/<count>/<hash>.<ext>.");

    long count = strtol(path[0].c_str(), NULL, 10);
    if (count < 1 || count > 2000)
        throw RESTERR(HTTP_BAD_REQUEST, "Header count out of range: " + path[0]);

    string hashStr = path[1];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        throw RESTERR(HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    std::vector<CBlockHeader> headers;
    headers.reserve(count);
    {
        LOCK(cs_main);
        BlockMap::const_iterator it = mapBlockIndex.find(hash);
        const CBlockIndex *pindex = (it != mapBlockIndex.end()) ? it->second : NULL;
        while (pindex != NULL && chainActive.Contains(pindex)) {
            headers.push_back(pindex->GetBlockHeader());
            if (headers.size() == (unsigned long)count)
                break;
            pindex = chainActive.Next(pindex);
        }
    }

    CDataStream ssHeader(SER_NETWORK, PROTOCOL_VERSION);
    BOOST_FOREACH(const CBlockHeader &header, headers) {
        ssHeader << header;
    }

    switch (rf) {
    case RF_BINARY: {
        string binaryHeader = ssHeader.str();
        conn->stream() << HTTPReplyHeader(HTTP_OK, fRun, binaryHeader.size(), "application/octet-stream") << binaryHeader << std::flush;
        return true;
    }

    case RF_HEX: {
        string strHex = HexStr(ssHeader.begin(), ssHeader.end()) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strHex, fRun, false, "text/plain") << std::flush;
        return true;
    }

    default: {
        throw RESTERR(HTTP_NOT_FOUND, "output format not found (available: .bin, .hex)");
    }
    }

    // not reached
    return true; // continue to process further HTTP reqs on this cxn
}

static bool rest_block(AcceptedConnection* conn,
                       const std::string& strURIPart,
                       const std::string& strRequest,
                       const std::map<std::string, std::string>& mapHeaders,
                       bool fRun,
                       bool showTxDetails)
{
    vector<string> params;
    const RetFormat rf = ParseDataFormat(params, strURIPart);

    string hashStr = params[0];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        throw RESTERR(HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    CBlock block;
    CBlockIndex* pblockindex = NULL;
    {
        LOCK(cs_main);
        if (mapBlockIndex.count(hash) == 0)
            throw RESTERR(HTTP_NOT_FOUND, hashStr + " not found");

        pblockindex = mapBlockIndex[hash];
        if (fHavePruned && !(pblockindex->nStatus & BLOCK_HAVE_DATA) && pblockindex->nTx > 0)
            throw RESTERR(HTTP_NOT_FOUND, hashStr + " not available (pruned data)");

        if (!ReadBlockFromDisk(block, pblockindex))
            throw RESTERR(HTTP_NOT_FOUND, hashStr + " not found");
    }

    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << block;

    switch (rf) {
    case RF_BINARY: {
        string binaryBlock = ssBlock.str();
        conn->stream() << HTTPReplyHeader(HTTP_OK, fRun, binaryBlock.size(), "application/octet-stream") << binaryBlock << std::flush;
        return true;
    }

    case RF_HEX: {
        string strHex = HexStr(ssBlock.begin(), ssBlock.end()) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strHex, fRun, false, "text/plain") << std::flush;
        return true;
    }

    case RF_JSON: {
        Object objBlock = blockToJSON(block, pblockindex, showTxDetails);
        string strJSON = write_string(Value(objBlock), false) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strJSON, fRun) << std::flush;
        return true;
    }

    default: {
        throw RESTERR(HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }

    // not reached
    return true; // continue to process further HTTP reqs on this cxn
}

static bool rest_block_extended(AcceptedConnection* conn,
                       const std::string& strURIPart,
                       const std::string& strRequest,
                       const std::map<std::string, std::string>& mapHeaders,
                       bool fRun)
{
    return rest_block(conn, strURIPart, strRequest, mapHeaders, fRun, true);
}

static bool rest_block_notxdetails(AcceptedConnection* conn,
                       const std::string& strURIPart,
                       const std::string& strRequest,
                       const std::map<std::string, std::string>& mapHeaders,
                       bool fRun)
{
    return rest_block(conn, strURIPart, strRequest, mapHeaders, fRun, false);
}

static bool rest_chaininfo(AcceptedConnection* conn,
                           const std::string& strURIPart,
                           const std::string& strRequest,
                           const std::map<std::string, std::string>& mapHeaders,
                           bool fRun)
{
    vector<string> params;
    const RetFormat rf = ParseDataFormat(params, strURIPart);

    switch (rf) {
    case RF_JSON: {
        Array rpcParams;
        Value chainInfoObject = getblockchaininfo(rpcParams, false);

        string strJSON = write_string(chainInfoObject, false) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strJSON, fRun) << std::flush;
        return true;
    }
    default: {
        throw RESTERR(HTTP_NOT_FOUND, "output format not found (available: json)");
    }
    }

    // not reached
    return true; // continue to process further HTTP reqs on this cxn
}

static bool rest_tx(AcceptedConnection* conn,
                    const std::string& strURIPart,
                    const std::string& strRequest,
                    const std::map<std::string, std::string>& mapHeaders,
                    bool fRun)
{
    vector<string> params;
    const RetFormat rf = ParseDataFormat(params, strURIPart);

    string hashStr = params[0];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        throw RESTERR(HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    CTransaction tx;
    uint256 hashBlock = uint256();
    if (!GetTransaction(hash, tx, hashBlock, true))
        throw RESTERR(HTTP_NOT_FOUND, hashStr + " not found");

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    switch (rf) {
    case RF_BINARY: {
        string binaryTx = ssTx.str();
        conn->stream() << HTTPReplyHeader(HTTP_OK, fRun, binaryTx.size(), "application/octet-stream") << binaryTx << std::flush;
        return true;
    }

    case RF_HEX: {
        string strHex = HexStr(ssTx.begin(), ssTx.end()) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strHex, fRun, false, "text/plain") << std::flush;
        return true;
    }

    case RF_JSON: {
        Object objTx;
        TxToJSON(tx, hashBlock, objTx);
        string strJSON = write_string(Value(objTx), false) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strJSON, fRun) << std::flush;
        return true;
    }

    default: {
        throw RESTERR(HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }

    // not reached
    return true; // continue to process further HTTP reqs on this cxn
}

static bool rest_getutxos(AcceptedConnection* conn,
                          const std::string& strURIPart,
                          const std::string& strRequest,
                          const std::map<std::string, std::string>& mapHeaders,
                          bool fRun)
{
    vector<string> params;
    enum RetFormat rf = ParseDataFormat(params, strURIPart);

    vector<string> uriParts;
    if (params.size() > 0 && params[0].length() > 1)
    {
        std::string strUriParams = params[0].substr(1);
        boost::split(uriParts, strUriParams, boost::is_any_of("/"));
    }

    // throw exception in case of a empty request
    if (strRequest.length() == 0 && uriParts.size() == 0)
        throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, "Error: empty request");

    bool fInputParsed = false;
    bool fCheckMemPool = false;
    vector<COutPoint> vOutPoints;

    // parse/deserialize input
    // input-format = output-format, rest/getutxos/bin requires binary input, gives binary output, ...

    if (uriParts.size() > 0)
    {

        //inputs is sent over URI scheme (/rest/getutxos/checkmempool/txid1-n/txid2-n/...)
        if (uriParts.size() > 0 && uriParts[0] == "checkmempool")
            fCheckMemPool = true;

        for (size_t i = (fCheckMemPool) ? 1 : 0; i < uriParts.size(); i++)
        {
            uint256 txid;
            int32_t nOutput;
            std::string strTxid = uriParts[i].substr(0, uriParts[i].find("-"));
            std::string strOutput = uriParts[i].substr(uriParts[i].find("-")+1);

            if (!ParseInt32(strOutput, &nOutput) || !IsHex(strTxid))
                throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, "Parse error");

            txid.SetHex(strTxid);
            vOutPoints.push_back(COutPoint(txid, (uint32_t)nOutput));
        }

        if (vOutPoints.size() > 0)
            fInputParsed = true;
        else
            throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, "Error: empty request");
    }

    string strRequestMutable = strRequest; //convert const string to string for allowing hex to bin converting

    switch (rf) {
    case RF_HEX: {
        // convert hex to bin, continue then with bin part
        std::vector<unsigned char> strRequestV = ParseHex(strRequest);
        strRequestMutable.assign(strRequestV.begin(), strRequestV.end());
    }

    case RF_BINARY: {
        try {
            //deserialize only if user sent a request
            if (strRequestMutable.size() > 0)
            {
                if (fInputParsed) //don't allow sending input over URI and HTTP RAW DATA
                    throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, "Combination of URI scheme inputs and raw post data is not allowed");

                CDataStream oss(SER_NETWORK, PROTOCOL_VERSION);
                oss << strRequestMutable;
                oss >> fCheckMemPool;
                oss >> vOutPoints;
            }
        } catch (const std::ios_base::failure& e) {
            // abort in case of unreadable binary data
            throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, "Parse error");
        }
        break;
    }

    case RF_JSON: {
        if (!fInputParsed)
            throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, "Error: empty request");
        break;
    }
    default: {
        throw RESTERR(HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }

    // limit max outpoints
    if (vOutPoints.size() > MAX_GETUTXOS_OUTPOINTS)
        throw RESTERR(HTTP_INTERNAL_SERVER_ERROR, strprintf("Error: max outpoints exceeded (max: %d, tried: %d)", MAX_GETUTXOS_OUTPOINTS, vOutPoints.size()));

    // check spentness and form a bitmap (as well as a JSON capable human-readble string representation)
    vector<unsigned char> bitmap;
    vector<CCoin> outs;
    std::string bitmapStringRepresentation;
    boost::dynamic_bitset<unsigned char> hits(vOutPoints.size());
    {
        LOCK2(cs_main, mempool.cs);

        CCoinsView viewDummy;
        CCoinsViewCache view(&viewDummy);

        CCoinsViewCache& viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);

        if (fCheckMemPool)
            view.SetBackend(viewMempool); // switch cache backend to db+mempool in case user likes to query mempool

        for (size_t i = 0; i < vOutPoints.size(); i++) {
            CCoins coins;
            uint256 hash = vOutPoints[i].hash;
            if (view.GetCoins(hash, coins)) {
                mempool.pruneSpent(hash, coins);
                if (coins.IsAvailable(vOutPoints[i].n)) {
                    hits[i] = true;
                    // Safe to index into vout here because IsAvailable checked if it's off the end of the array, or if
                    // n is valid but points to an already spent output (IsNull).
                    CCoin coin;
                    coin.nTxVer = coins.nVersion;
                    coin.nHeight = coins.nHeight;
                    coin.out = coins.vout.at(vOutPoints[i].n);
                    assert(!coin.out.IsNull());
                    outs.push_back(coin);
                }
            }

            bitmapStringRepresentation.append(hits[i] ? "1" : "0"); // form a binary string representation (human-readable for json output)
        }
    }
    boost::to_block_range(hits, std::back_inserter(bitmap));

    switch (rf) {
    case RF_BINARY: {
        // serialize data
        // use exact same output as mentioned in Bip64
        CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
        ssGetUTXOResponse << chainActive.Height() << chainActive.Tip()->GetBlockHash() << bitmap << outs;
        string ssGetUTXOResponseString = ssGetUTXOResponse.str();

        conn->stream() << HTTPReplyHeader(HTTP_OK, fRun, ssGetUTXOResponseString.size(), "application/octet-stream") << ssGetUTXOResponseString << std::flush;
        return true;
    }

    case RF_HEX: {
        CDataStream ssGetUTXOResponse(SER_NETWORK, PROTOCOL_VERSION);
        ssGetUTXOResponse << chainActive.Height() << chainActive.Tip()->GetBlockHash() << bitmap << outs;
        string strHex = HexStr(ssGetUTXOResponse.begin(), ssGetUTXOResponse.end()) + "\n";

        conn->stream() << HTTPReply(HTTP_OK, strHex, fRun, false, "text/plain") << std::flush;
        return true;
    }

    case RF_JSON: {
        Object objGetUTXOResponse;

        // pack in some essentials
        // use more or less the same output as mentioned in Bip64
        objGetUTXOResponse.push_back(Pair("chainHeight", chainActive.Height()));
        objGetUTXOResponse.push_back(Pair("chaintipHash", chainActive.Tip()->GetBlockHash().GetHex()));
        objGetUTXOResponse.push_back(Pair("bitmap", bitmapStringRepresentation));

        Array utxos;
        BOOST_FOREACH (const CCoin& coin, outs) {
            Object utxo;
            utxo.push_back(Pair("txvers", (int32_t)coin.nTxVer));
            utxo.push_back(Pair("height", (int32_t)coin.nHeight));
            utxo.push_back(Pair("value", ValueFromAmount(coin.out.nValue)));

            // include the script in a json output
            Object o;
            ScriptPubKeyToJSON(coin.out.scriptPubKey, o, true);
            utxo.push_back(Pair("scriptPubKey", o));
            utxos.push_back(utxo);
        }
        objGetUTXOResponse.push_back(Pair("utxos", utxos));

        // return json string
        string strJSON = write_string(Value(objGetUTXOResponse), false) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strJSON, fRun) << std::flush;
        return true;
    }
    default: {
        throw RESTERR(HTTP_NOT_FOUND, "output format not found (available: " + AvailableDataFormatsString() + ")");
    }
    }

    // not reached
    return true; // continue to process further HTTP reqs on this cxn
}

static const struct {
    const char* prefix;
    bool (*handler)(AcceptedConnection* conn,
                    const std::string& strURIPart,
                    const std::string& strRequest,
                    const std::map<std::string, std::string>& mapHeaders,
                    bool fRun);
} uri_prefixes[] = {
      {"/rest/tx/", rest_tx},
      {"/rest/block/notxdetails/", rest_block_notxdetails},
      {"/rest/block/", rest_block_extended},
      {"/rest/chaininfo", rest_chaininfo},
      {"/rest/headers/", rest_headers},
      {"/rest/getutxos", rest_getutxos},
};

bool HTTPReq_REST(AcceptedConnection* conn,
                  const std::string& strURI,
                  const string& strRequest,
                  const std::map<std::string, std::string>& mapHeaders,
                  bool fRun)
{
    try {
        std::string statusmessage;
        if (RPCIsInWarmup(&statusmessage))
            throw RESTERR(HTTP_SERVICE_UNAVAILABLE, "Service temporarily unavailable: " + statusmessage);

        for (unsigned int i = 0; i < ARRAYLEN(uri_prefixes); i++) {
            unsigned int plen = strlen(uri_prefixes[i].prefix);
            if (strURI.substr(0, plen) == uri_prefixes[i].prefix) {
                string strURIPart = strURI.substr(plen);
                return uri_prefixes[i].handler(conn, strURIPart, strRequest, mapHeaders, fRun);
            }
        }
    } catch (const RestErr& re) {
        conn->stream() << HTTPReply(re.status, re.message + "\r\n", false, false, "text/plain") << std::flush;
        return false;
    }

    conn->stream() << HTTPError(HTTP_NOT_FOUND, false) << std::flush;
    return false;
}
