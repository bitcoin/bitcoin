// Copyright (c) 2011 Bitcoin Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "init.h" // for pwalletMain
#include "rpc.h"
#undef printf

#include <boost/asio.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#ifdef USE_SSL
#include <boost/asio/ssl.hpp> 
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLStream;
#endif
#include <boost/xpressive/xpressive_dynamic.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#define printf OutputDebugStringF

using namespace boost::asio;
using namespace json_spirit;
using namespace std;

extern string HTTPPost(const string& host, const string& path, const string& strMsg,
                       const map<string,string>& mapRequestHeaders);
extern string JSONRPCReply(const Value& result, const Value& error, const Value& id);
extern string JSONRPCRequest(const string& strMethod, const Array& params, const Value& id);
extern int ReadHTTP(std::basic_istream<char>& stream, map<string, string>& mapHeadersRet, string& strMessageRet);
extern double GetDifficulty(const CBlockIndex* blockindex = pindexBest);
extern void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret);


void ThreadHTTPPOST2(void* parg);

class CPOSTRequest
{
public:
    CPOSTRequest(const string &_url, const string& _body) : url(_url), body(_body)
    {
    }

    virtual bool POST()
    {
        using namespace boost::xpressive;
        // This regex is wrong for IPv6 urls; see http://www.ietf.org/rfc/rfc2732.txt
        //  (they're weird; e.g  "http://[::FFFF:129.144.52.38]:80/index.html" )
        // I can live with non-raw-IPv6 urls for now...
        static sregex url_regex = sregex::compile("^(http|https)://([^:/]+)(:[0-9]{1,5})?(.*)$");

        boost::xpressive::smatch urlparts;
        if (!regex_match(url, urlparts, url_regex))
        {
            printf("URL PARSING FAILED: %s\n", url.c_str());
            return true;
        }
        string protocol = urlparts[1];
        string host = urlparts[2];
        string s_port = urlparts[3];  // Note: includes colon, e.g. ":8080"
        bool fSSL = (protocol == "https" ? true : false);
        int port = (fSSL ? 443 : 80);
        if (s_port.size() > 1) { port = atoi(s_port.c_str()+1); }
        string path = urlparts[4];
        map<string, string> headers;

#ifdef USE_SSL
        io_service io_service;
        ssl::context context(io_service, ssl::context::sslv23);
        context.set_options(ssl::context::no_sslv2);
        SSLStream sslStream(io_service, context);
        SSLIOStreamDevice d(sslStream, fSSL);
        boost::iostreams::stream<SSLIOStreamDevice> stream(d);
        if (!d.connect(host, boost::lexical_cast<string>(port)))
        {
            printf("POST: Couldn't connect to %s:%d", host.c_str(), port);
            return false;
        }
#else
        if (fSSL)
        {
            printf("Cannot POST to SSL server, bitcoin compiled without full openssl libraries.");
            return false;
        }
        ip::tcp::iostream stream(host, boost::lexical_cast<string>(port));
#endif

        stream << HTTPPost(host, path, body, headers) << std::flush;
        map<string, string> mapResponseHeaders;
        string strReply;
        int status = ReadHTTP(stream, mapResponseHeaders, strReply);
//        printf(" HTTP response %d: %s\n", status, strReply.c_str());

        return (status < 300);
    }

protected:
    string url;
    string body;
};

static vector<boost::shared_ptr<CPOSTRequest> > vPOSTQueue;
static CCriticalSection cs_vPOSTQueue;


Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    result.push_back(Pair("blockcount", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("time", (boost::int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (boost::uint64_t)block.nNonce));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));
    Array txhashes;
    BOOST_FOREACH (const CTransaction&tx, block.vtx)
        txhashes.push_back(tx.GetHash().GetHex());
    result.push_back(Pair("tx", txhashes));

    if (blockindex->pprev)
        result.push_back(Pair("hashprevious", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("hashnext", blockindex->pnext->GetBlockHash().GetHex()));
    return result;
}

void monitorBlock(const CBlock& block, const CBlockIndex* pblockindex)
{
    Array params; // JSON-RPC requests are always "params" : [ ... ]
    params.push_back(blockToJSON(block, pblockindex));

    string postBody = JSONRPCRequest("monitorblock", params, Value());

    CRITICAL_BLOCK(cs_mapMonitored)
    CRITICAL_BLOCK(cs_vPOSTQueue)
    {
        BOOST_FOREACH (const string& url, setMonitorBlocks)
        {
            boost::shared_ptr<CPOSTRequest> postRequest(new CPOSTRequest(url, postBody));
            vPOSTQueue.push_back(postRequest);
        }
    }
}

void monitorTx(const CWalletTx& wtx)
{
    Array params; // JSON-RPC requests are always "params" : [ ... ]
    ListTransactions(wtx, "*", 0, true, params);
    if (params.empty())
        return; // Not our transaction

    string postBody = JSONRPCRequest("monitortx", params, Value());

    CRITICAL_BLOCK(cs_mapMonitored)
    CRITICAL_BLOCK(cs_vPOSTQueue)
    {
        BOOST_FOREACH (const string& url, setMonitorTx)
        {
            boost::shared_ptr<CPOSTRequest> postRequest(new CPOSTRequest(url, postBody));
            vPOSTQueue.push_back(postRequest);
        }
    }
}

Value listmonitored(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "listmonitored\n"
            "Returns list of urls that receive notification when new blocks are accepted.");

    Array ret;
    CRITICAL_BLOCK(cs_mapMonitored)
    {
        BOOST_FOREACH (const string& url, setMonitorBlocks)
        {
            Object item;
            item.push_back(Pair("category", "block"));
            item.push_back(Pair("url", url));
            ret.push_back(item);
        }
        BOOST_FOREACH (const string& url, setMonitorTx)
        {
            Object item;
            item.push_back(Pair("category", "tx"));
            item.push_back(Pair("url", url));
            ret.push_back(item);
        }
    }
    return ret;
}

Value monitortx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "monitortx <url> [monitor=true]\n"
            "POST transaction information to <url> as wallet transactions are sent/received.\n"
            "[monitor] true will start monitoring, false will stop.");
    string url = params[0].get_str();
    bool fMonitor = true;
    if (params.size() > 1)
        fMonitor = params[1].get_bool();

    CRITICAL_BLOCK(cs_mapMonitored)
    {
        if (!fMonitor)
            setMonitorTx.erase(url);
        else
            setMonitorTx.insert(url);
        WriteSetting("monitor_tx", setMonitorTx);
    }
    return Value::null;
}

Value monitorblocks(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "monitorblocks <url> [monitor=true]\n"
            "POST block information to <url> as blocks are added to the block chain.\n"
            "[monitor] true will start monitoring, false will stop.");
    string url = params[0].get_str();
    bool fMonitor = true;
    if (params.size() > 1)
        fMonitor = params[1].get_bool();

    CRITICAL_BLOCK(cs_mapMonitored)
    {
        if (!fMonitor)
            setMonitorBlocks.erase(url);
        else
            setMonitorBlocks.insert(url);
        WriteSetting("monitor_block", setMonitorBlocks);
    }
    return Value::null;
}

void ThreadHTTPPOST(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadHTTPPOST(parg));
    try
    {
        vnThreadsRunning[6]++;
        ThreadHTTPPOST2(parg);
        vnThreadsRunning[6]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[6]--;
        PrintException(&e, "ThreadHTTPPOST()");
    } catch (...) {
        vnThreadsRunning[6]--;
        PrintException(NULL, "ThreadHTTPPOST()");
    }
    printf("ThreadHTTPPOST exiting\n");
}

void ThreadHTTPPOST2(void* parg)
{
    printf("ThreadHTTPPOST started\n");

    loop
    {
        if (fShutdown)
            return;

        vector<boost::shared_ptr<CPOSTRequest> > work;
        CRITICAL_BLOCK(cs_vPOSTQueue)
        {
            work = vPOSTQueue;
            vPOSTQueue.clear();
        }
        BOOST_FOREACH (boost::shared_ptr<CPOSTRequest> r, work)
            r->POST();

        if (vPOSTQueue.empty())
            Sleep(100); // 100ms (1/10 second)
    }
}
