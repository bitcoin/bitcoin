// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string.hpp>
#include "rpcserver.h"
#include "streams.h"
#include "utilstrencodings.h"
#include "core/block.h"
#include "core/transaction.h"
#include "version.h"
#include "main.h"

using namespace std;
using namespace json_spirit;

enum RetFormat {
    RF_BINARY,
    RF_HEX,
    RF_JSON,
};

static const struct {
    enum RetFormat rf;
    const char *name;
} rf_names[] = {
    { RF_BINARY, "binary" },            // default, if match not found
    { RF_HEX, "hex" },
    { RF_JSON, "json" },
};

class RestErr {
public:
    enum HTTPStatusCode status;
    string message;
};

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, Object& entry);
extern Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex);

static RestErr RESTERR(enum HTTPStatusCode status, string message)
{
    RestErr re;
    re.status = status;
    re.message = message;
    return re;
}

static enum RetFormat ParseDataFormat(const string& format)
{
    for (unsigned int i = 0; i < ARRAYLEN(rf_names); i++)
        if (format == rf_names[i].name)
            return rf_names[i].rf;

    return rf_names[0].rf;
}

static bool ParseHashStr(string& strReq, uint256& v)
{
    if (!IsHex(strReq) || (strReq.size() != 64))
        return false;

    v.SetHex(strReq);
    return true;
}

static bool rest_block(AcceptedConnection *conn,
                       string& strReq,
                       map<string, string>& mapHeaders,
                       bool fRun)
{
    vector<string> params;
    boost::split(params, strReq, boost::is_any_of("/"));

    enum RetFormat rf = ParseDataFormat(params.size() > 1 ? params[1] : string(""));

    string hashStr = params[0];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        throw RESTERR(HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    if (mapBlockIndex.count(hash) == 0)
        throw RESTERR(HTTP_NOT_FOUND, hashStr + " not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    if (!ReadBlockFromDisk(block, pblockindex))
        throw RESTERR(HTTP_NOT_FOUND, hashStr + " not found");

    CDataStream ssBlock(SER_NETWORK, PROTOCOL_VERSION);
    ssBlock << block;

    switch (rf) {
    case RF_BINARY: {
        string binaryBlock = ssBlock.str();
        conn->stream() << HTTPReply(HTTP_OK, binaryBlock, fRun, true, "application/octet-stream") << binaryBlock << std::flush;
        return true;
    }

    case RF_HEX: {
        string strHex = HexStr(ssBlock.begin(), ssBlock.end()) + "\n";;
        conn->stream() << HTTPReply(HTTP_OK, strHex, fRun, false, "text/plain") << std::flush;
        return true;
    }

    case RF_JSON: {
        Object objBlock = blockToJSON(block, pblockindex);
        string strJSON = write_string(Value(objBlock), false) + "\n";
        conn->stream() << HTTPReply(HTTP_OK, strJSON, fRun) << std::flush;
        return true;
     }
    }

    // not reached
    return true;     // continue to process further HTTP reqs on this cxn
}

static bool rest_tx(AcceptedConnection *conn,
                    string& strReq,
                    map<string, string>& mapHeaders,
                    bool fRun)
{
    vector<string> params;
    boost::split(params, strReq, boost::is_any_of("/"));

    enum RetFormat rf = ParseDataFormat(params.size() > 1 ? params[1] : string(""));

    string hashStr = params[0];
    uint256 hash;
    if (!ParseHashStr(hashStr, hash))
        throw RESTERR(HTTP_BAD_REQUEST, "Invalid hash: " + hashStr);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock, true))
        throw RESTERR(HTTP_NOT_FOUND, hashStr + " not found");

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;

    switch (rf) {
    case RF_BINARY: {
        string binaryTx = ssTx.str();
        conn->stream() << HTTPReply(HTTP_OK, binaryTx, fRun, true, "application/octet-stream") << binaryTx << std::flush;
        return true;
    }

    case RF_HEX: {
        string strHex = HexStr(ssTx.begin(), ssTx.end()) + "\n";;
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
    }

    // not reached
    return true;     // continue to process further HTTP reqs on this cxn
}

static const struct {
    const char *prefix;
    bool (*handler)(AcceptedConnection *conn,
                    string& strURI,
                    map<string, string>& mapHeaders,
                    bool fRun);
} uri_prefixes[] = {
    { "/rest/tx/", rest_tx },
    { "/rest/block/", rest_block },
};

bool HTTPReq_REST(AcceptedConnection *conn,
                  string& strURI,
                  map<string, string>& mapHeaders,
                  bool fRun)
{
    try {
        for (unsigned int i = 0; i < ARRAYLEN(uri_prefixes); i++) {
            unsigned int plen = strlen(uri_prefixes[i].prefix);
            if (strURI.substr(0, plen) == uri_prefixes[i].prefix) {
                string strReq = strURI.substr(plen);
                return uri_prefixes[i].handler(conn, strReq, mapHeaders, fRun);
            }
        }
    }
    catch (RestErr& re) {
        conn->stream() << HTTPReply(re.status, re.message + "\r\n", false, false, "text/plain") << std::flush;
        return false;
    }

    conn->stream() << HTTPReply(HTTP_NOT_FOUND, "", false) << std::flush;
    return false;
}

