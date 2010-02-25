// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#undef printf
#include <boost/asio.hpp>
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"
#define printf OutputDebugStringF
// MinGW 3.4.5 gets "fatal error: had to relocate PCH" if the json headers are
// precompiled in headers.h.  The problem might be when the pch file goes over
// a certain size around 145MB.  If we need access to json_spirit outside this
// file, we could use the compiled json_spirit option.

using boost::asio::ip::tcp;
using namespace json_spirit;

void ThreadRPCServer2(void* parg);







///
/// Note: I'm not finished designing this interface, it's still subject to change.
///



Value stop(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "stop (no parameters)\n"
            "Stop bitcoin server.");

    // Shutdown will take long enough that the response should get back
    CreateThread(Shutdown, NULL);
    return "bitcoin server stopping";
}


Value getblockcount(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getblockcount (no parameters)\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight + 1;
}


Value getblocknumber(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getblocknumber (no parameters)\n"
            "Returns the block number of the latest block in the longest block chain.");

    return nBestHeight;
}


Value getconnectioncount(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getconnectioncount (no parameters)\n"
            "Returns the number of connections to other nodes.");

    return (int)vNodes.size();
}


Value getdifficulty(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getdifficulty (no parameters)\n"
            "Returns the proof-of-work difficulty as a multiple of the minimum difficulty.");

    if (pindexBest == NULL)
        throw runtime_error("block chain not loaded");

    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    int nShift = 256 - 32 - 31; // to fit in a uint
    double dMinimum = (CBigNum().SetCompact(bnProofOfWorkLimit.GetCompact()) >> nShift).getuint();
    double dCurrently = (CBigNum().SetCompact(pindexBest->nBits) >> nShift).getuint();
    return dMinimum / dCurrently;
}


Value getbalance(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getbalance (no parameters)\n"
            "Returns the server's available balance.");

    return ((double)GetBalance() / (double)COIN);
}


Value getgenerate(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getgenerate (no parameters)\n"
            "Returns true or false.");

    return (bool)fGenerateBitcoins;
}


Value setgenerate(const Array& params)
{
    if (params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setgenerate <generate> [genproclimit]\n"
            "<generate> is true or false to turn generation on or off.\n"
            "Generation is limited to [genproclimit] processors, -1 is unlimited.");

    bool fGenerate = true;
    if (params.size() > 0)
        fGenerate = params[0].get_bool();

    if (params.size() > 1)
    {
        int nGenProcLimit = params[1].get_int();
        fLimitProcessors = (nGenProcLimit != -1);
        CWalletDB().WriteSetting("fLimitProcessors", fLimitProcessors);
        if (nGenProcLimit != -1)
            CWalletDB().WriteSetting("nLimitProcessors", nLimitProcessors = nGenProcLimit);
    }

    GenerateBitcoins(fGenerate);
    return Value::null;
}


Value getinfo(const Array& params)
{
    if (params.size() != 0)
        throw runtime_error(
            "getinfo (no parameters)");

    Object obj;
    obj.push_back(Pair("balance",       (double)GetBalance() / (double)COIN));
    obj.push_back(Pair("blocks",        (int)nBestHeight + 1));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("proxy",         (fUseProxy ? addrProxy.ToStringIPPort() : string())));
    obj.push_back(Pair("generate",      (bool)fGenerateBitcoins));
    obj.push_back(Pair("genproclimit",  (int)(fLimitProcessors ? nLimitProcessors : -1)));
    return obj;
}


Value getnewaddress(const Array& params)
{
    if (params.size() > 1)
        throw runtime_error(
            "getnewaddress [label]\n"
            "Returns a new bitcoin address for receiving payments.  "
            "If [label] is specified (recommended), it is added to the address book "
            "so payments received with the address will be labeled.");

    // Parse the label first so we don't generate a key if there's an error
    string strLabel;
    if (params.size() > 0)
        strLabel = params[0].get_str();

    // Generate a new key that is added to wallet
    string strAddress = PubKeyToAddress(GenerateNewKey());

    SetAddressBookName(strAddress, strLabel);
    return strAddress;
}


Value sendtoaddress(const Array& params)
{
    if (params.size() < 2 || params.size() > 4)
        throw runtime_error(
            "sendtoaddress <bitcoinaddress> <amount> [comment] [comment-to]\n"
            "<amount> is a real and is rounded to the nearest 0.01");

    string strAddress = params[0].get_str();

    // Amount
    if (params[1].get_real() <= 0.0 || params[1].get_real() > 21000000.0)
        throw runtime_error("Invalid amount");
    int64 nAmount = roundint64(params[1].get_real() * 100.00) * CENT;

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 2 && params[2].type() != null_type && !params[2].get_str().empty())
        wtx.mapValue["message"] = params[2].get_str();
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["to"]      = params[3].get_str();

    string strError = SendMoneyToBitcoinAddress(strAddress, nAmount, wtx);
    if (strError != "")
        throw runtime_error(strError);
    return "sent";
}


Value listtransactions(const Array& params)
{
    if (params.size() > 2)
        throw runtime_error(
            "listtransactions [count=10] [includegenerated=false]\n"
            "Returns up to [count] most recent transactions.");

    int64 nCount = 10;
    if (params.size() > 0)
        nCount = params[0].get_int64();
    bool fGenerated = false;
    if (params.size() > 1)
        fGenerated = params[1].get_bool();

    Array ret;
    //// not finished
    ret.push_back("not implemented yet");
    return ret;
}


Value getamountreceived(const Array& params)
{
    if (params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getamountreceived <bitcoinaddress> [minconf=1]\n"
            "Returns the total amount received by <bitcoinaddress> in transactions with at least [minconf] confirmations.");

    // Bitcoin address
    string strAddress = params[0].get_str();
    CScript scriptPubKey;
    if (!scriptPubKey.SetBitcoinAddress(strAddress))
        throw runtime_error("Invalid bitcoin address");

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    int64 nAmount = 0;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (wtx.IsCoinBase() || !wtx.IsFinal())
                continue;

            foreach(const CTxOut& txout, wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    if (wtx.GetDepthInMainChain() >= nMinDepth)
                        nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)COIN;
}


struct tallyitem
{
    int64 nAmount;
    int nConf;
    tallyitem()
    {
        nAmount = 0;
        nConf = INT_MAX;
    }
};

Value getallreceived(const Array& params)
{
    if (params.size() > 1)
        throw runtime_error(
            "getallreceived [minconf=1]\n"
            "[minconf] is the minimum number of confirmations before payments are included.\n"
            "Returns an array of objects containing:\n"
            "  \"address\" : receiving address\n"
            "  \"amount\" : total amount received by the address\n"
            "  \"confirmations\" : number of confirmations of the most recent transaction included\n"
            "  \"label\" : the label of the receiving address");

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Tally
    map<uint160, tallyitem> mapTally;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (wtx.IsCoinBase() || !wtx.IsFinal())
                continue;

            int nDepth = wtx.GetDepthInMainChain();
            if (nDepth < nMinDepth)
                continue;

            // Filter out debits and payments to self, which may have change return
            // we don't want to count.
            int64 nCredit = wtx.GetCredit(true);
            int64 nDebit = wtx.GetDebit();
            int64 nNet = nCredit - nDebit;
            if (nNet <= 0)
                continue;

            foreach(const CTxOut& txout, wtx.vout)
            {
                uint160 hash160 = txout.scriptPubKey.GetBitcoinAddressHash160();
                if (hash160 == 0 || !mapPubKeys.count(hash160))
                    continue;

                tallyitem& item = mapTally[hash160];
                item.nAmount += txout.nValue;
                item.nConf = min(item.nConf, nDepth);
            }
        }
    }

    // Reply
    Array ret;
    CRITICAL_BLOCK(cs_mapAddressBook)
    {
        for (map<uint160, tallyitem>::iterator it = mapTally.begin(); it != mapTally.end(); ++it)
        {
            string strAddress = Hash160ToAddress((*it).first);
            string strLabel;
            map<string, string>::iterator mi = mapAddressBook.find(strAddress);
            if (mi != mapAddressBook.end())
                strLabel = (*mi).second;

            Object obj;
            obj.push_back(Pair("address",       strAddress));
            obj.push_back(Pair("amount",        (double)(*it).second.nAmount / (double)COIN));
            obj.push_back(Pair("confirmations", (*it).second.nConf));
            obj.push_back(Pair("label",         strLabel));
            ret.push_back(obj);
        }
    }
    return ret;
}







//
// Call Table
//

typedef Value(*rpcfn_type)(const Array& params);
pair<string, rpcfn_type> pCallTable[] =
{
    make_pair("stop",               &stop),
    make_pair("getblockcount",      &getblockcount),
    make_pair("getblocknumber",     &getblocknumber),
    make_pair("getconnectioncount", &getconnectioncount),
    make_pair("getdifficulty",      &getdifficulty),
    make_pair("getbalance",         &getbalance),
    make_pair("getgenerate",        &getgenerate),
    make_pair("setgenerate",        &setgenerate),
    make_pair("getinfo",            &getinfo),
    make_pair("getnewaddress",      &getnewaddress),
    make_pair("sendtoaddress",      &sendtoaddress),
    make_pair("listtransactions",   &listtransactions),
    make_pair("getamountreceived",  &getamountreceived),
    make_pair("getallreceived",     &getallreceived),
};
map<string, rpcfn_type> mapCallTable(pCallTable, pCallTable + sizeof(pCallTable)/sizeof(pCallTable[0]));




//
// HTTP protocol
//
// This ain't Apache.  We're just using HTTP header for the length field
// and to be compatible with other JSON-RPC implementations.
//

string HTTPPost(const string& strMsg)
{
    return strprintf(
            "POST / HTTP/1.1\r\n"
            "User-Agent: json-rpc/1.0\r\n"
            "Host: 127.0.0.1\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: %d\r\n"
            "Accept: application/json\r\n"
            "\r\n"
            "%s",
        strMsg.size(),
        strMsg.c_str());
}

string HTTPReply(const string& strMsg, int nStatus=200)
{
    string strStatus;
    if (nStatus == 200) strStatus = "OK";
    if (nStatus == 500) strStatus = "Internal Server Error";
    return strprintf(
            "HTTP/1.1 %d %s\r\n"
            "Connection: close\r\n"
            "Content-Length: %d\r\n"
            "Content-Type: application/json\r\n"
            "Date: Sat, 08 Jul 2006 12:04:08 GMT\r\n"
            "Server: json-rpc/1.0\r\n"
            "\r\n"
            "%s",
        nStatus,
        strStatus.c_str(),
        strMsg.size(),
        strMsg.c_str());
}

int ReadHTTPHeader(tcp::iostream& stream)
{
    int nLen = 0;
    loop
    {
        string str;
        std::getline(stream, str);
        if (str.empty() || str == "\r")
            break;
        if (str.substr(0,15) == "Content-Length:")
            nLen = atoi(str.substr(15));
    }
    return nLen;
}

inline string ReadHTTP(tcp::iostream& stream)
{
    // Read header
    int nLen = ReadHTTPHeader(stream);
    if (nLen <= 0)
        return string();

    // Read message
    vector<char> vch(nLen);
    stream.read(&vch[0], nLen);
    return string(vch.begin(), vch.end());
}



//
// JSON-RPC protocol
//
// http://json-rpc.org/wiki/specification
// http://www.codeproject.com/KB/recipes/JSON_Spirit.aspx
//

string JSONRPCRequest(const string& strMethod, const Array& params, const Value& id)
{
    Object request;
    request.push_back(Pair("method", strMethod));
    request.push_back(Pair("params", params));
    request.push_back(Pair("id", id));
    return write_string(Value(request), false) + "\n";
}

string JSONRPCReply(const Value& result, const Value& error, const Value& id)
{
    Object reply;
    if (error.type() != null_type)
        reply.push_back(Pair("result", Value::null));
    else
        reply.push_back(Pair("result", result));
    reply.push_back(Pair("error", error));
    reply.push_back(Pair("id", id));
    return write_string(Value(reply), false) + "\n";
}




void ThreadRPCServer(void* parg)
{
    IMPLEMENT_RANDOMIZE_STACK(ThreadRPCServer(parg));
    try
    {
        vnThreadsRunning[4]++;
        ThreadRPCServer2(parg);
        vnThreadsRunning[4]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[4]--;
        PrintException(&e, "ThreadRPCServer()");
    } catch (...) {
        vnThreadsRunning[4]--;
        PrintException(NULL, "ThreadRPCServer()");
    }
    printf("ThreadRPCServer exiting\n");
}

void ThreadRPCServer2(void* parg)
{
    printf("ThreadRPCServer started\n");

    // Bind to loopback 127.0.0.1 so the socket can only be accessed locally
    boost::asio::io_service io_service;
    tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 8332);
    tcp::acceptor acceptor(io_service, endpoint);

    loop
    {
        // Accept connection
        tcp::iostream stream;
        tcp::endpoint peer;
        vnThreadsRunning[4]--;
        acceptor.accept(*stream.rdbuf(), peer);
        vnThreadsRunning[4]++;
        if (fShutdown)
            return;

        // Shouldn't be possible for anyone else to connect, but just in case
        if (peer.address().to_string() != "127.0.0.1")
            continue;

        // Receive request
        string strRequest = ReadHTTP(stream);
        printf("ThreadRPCServer request=%s", strRequest.c_str());

        // Handle multiple invocations per request
        string::iterator begin = strRequest.begin();
        while (skipspaces(begin), begin != strRequest.end())
        {
            string::iterator prev = begin;
            Value id;
            try
            {
                // Parse request
                Value valRequest;
                if (!read_range(begin, strRequest.end(), valRequest))
                    throw runtime_error("Parse error.");
                const Object& request = valRequest.get_obj();
                if (find_value(request, "method").type() != str_type ||
                    find_value(request, "params").type() != array_type)
                    throw runtime_error("Invalid request.");

                string strMethod    = find_value(request, "method").get_str();
                const Array& params = find_value(request, "params").get_array();
                id                  = find_value(request, "id");

                // Execute
                map<string, rpcfn_type>::iterator mi = mapCallTable.find(strMethod);
                if (mi == mapCallTable.end())
                    throw runtime_error("Method not found.");
                Value result = (*(*mi).second)(params);

                // Send reply
                string strReply = JSONRPCReply(result, Value::null, id);
                stream << HTTPReply(strReply, 200) << std::flush;
            }
            catch (std::exception& e)
            {
                // Send error reply
                string strReply = JSONRPCReply(Value::null, e.what(), id);
                stream << HTTPReply(strReply, 500) << std::flush;
            }
            if (begin == prev)
                break;
        }
    }
}




Value CallRPC(const string& strMethod, const Array& params)
{
    // Connect to localhost
    tcp::iostream stream("127.0.0.1", "8332");
    if (stream.fail())
        throw runtime_error("couldn't connect to server");

    // Send request
    string strRequest = JSONRPCRequest(strMethod, params, 1);
    stream << HTTPPost(strRequest) << std::flush;

    // Receive reply
    string strReply = ReadHTTP(stream);
    if (strReply.empty())
        throw runtime_error("no response from server");

    // Parse reply
    Value valReply;
    if (!read_string(strReply, valReply))
        throw runtime_error("couldn't parse reply from server");
    const Object& reply = valReply.get_obj();
    if (reply.empty())
        throw runtime_error("expected reply to have result, error and id properties");

    const Value& result = find_value(reply, "result");
    const Value& error  = find_value(reply, "error");
    const Value& id     = find_value(reply, "id");

    if (error.type() == str_type)
        throw runtime_error(error.get_str());
    else if (error.type() != null_type)
        throw runtime_error(write_string(error, false));
    return result;
}




template<typename T>
void ConvertTo(Value& value)
{
    if (value.type() == str_type)
    {
        // reinterpret string as unquoted json value
        Value value2;
        if (!read_string(value.get_str(), value2))
            throw runtime_error("type mismatch");
        value = value2.get_value<T>();
    }
    else
    {
        value = value.get_value<T>();
    }
}

int CommandLineRPC(int argc, char *argv[])
{
    try
    {
        // Check that method exists
        if (argc < 2)
            throw runtime_error("too few parameters");
        string strMethod = argv[1];
        if (!mapCallTable.count(strMethod))
            throw runtime_error(strprintf("unknown command: %s", strMethod.c_str()));

        // Parameters default to strings
        Array params;
        for (int i = 2; i < argc; i++)
            params.push_back(argv[i]);
        int n = params.size();

        //
        // Special case other types
        //
        if (strMethod == "setgenerate"       && n > 0) ConvertTo<bool>(params[0]);
        if (strMethod == "setgenerate"       && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "sendtoaddress"     && n > 1) ConvertTo<double>(params[1]);
        if (strMethod == "listtransactions"  && n > 0) ConvertTo<boost::int64_t>(params[0]);
        if (strMethod == "listtransactions"  && n > 1) ConvertTo<bool>(params[1]);
        if (strMethod == "getamountreceived" && n > 1) ConvertTo<boost::int64_t>(params[1]);
        if (strMethod == "getallreceived"    && n > 0) ConvertTo<boost::int64_t>(params[0]);

        // Execute
        Value result = CallRPC(strMethod, params);

        // Print result
        string strResult = (result.type() == str_type ? result.get_str() : write_string(result, true));
        if (result.type() != null_type)
        {
            if (fWindows && fGUI)
                // Windows GUI apps can't print to command line,
                // so settle for a message box yuck
                MyMessageBox(strResult.c_str(), "Bitcoin", wxOK);
            else
                fprintf(stdout, "%s\n", strResult.c_str());
        }
        return 0;
    }
    catch (std::exception& e) {
        if (fWindows && fGUI)
            MyMessageBox(strprintf("error: %s\n", e.what()).c_str(), "Bitcoin", wxOK);
        else
            fprintf(stderr, "error: %s\n", e.what());
    } catch (...) {
        PrintException(NULL, "CommandLineRPC()");
    }
    return 1;
}




#ifdef TEST
int main(int argc, char *argv[])
{
#ifdef _MSC_VER
    // Turn off microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFile("NUL", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0));
#endif
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    try
    {
        if (argc >= 2 && string(argv[1]) == "-server")
        {
            printf("server ready\n");
            ThreadRPCServer(NULL);
        }
        else
        {
            return CommandLineRPC(argc, argv);
        }
    }
    catch (std::exception& e) {
        PrintException(&e, "main()");
    } catch (...) {
        PrintException(NULL, "main()");
    }
    return 0;
}
#endif
