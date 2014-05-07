// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"

#include "base58.h"
#include "init.h"
#include "main.h"
#include "ui_interface.h"
#include "util.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/shared_ptr.hpp>
#include "json/json_spirit_writer_template.h"

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace json_spirit;

static std::string strRPCUserColonPass;

// These are created by StartRPCThreads, destroyed in StopRPCThreads
static asio::io_service* rpc_io_service = NULL;
static map<string, boost::shared_ptr<deadline_timer> > deadlineTimers;
static ssl::context* rpc_ssl_context = NULL;
static boost::thread_group* rpc_worker_group = NULL;
static boost::asio::io_service::work *rpc_dummy_work = NULL;
static std::vector<CSubNet> rpc_allow_subnets; //!< List of subnets to allow RPC connections from
static std::vector< boost::shared_ptr<ip::tcp::acceptor> > rpc_acceptors;

void RPCTypeCheck(const Array& params,
                  const list<Value_type>& typesExpected,
                  bool fAllowNull)
{
    unsigned int i = 0;
    BOOST_FOREACH(Value_type t, typesExpected)
    {
        if (params.size() <= i)
            break;

        const Value& v = params[i];
        if (!((v.type() == t) || (fAllowNull && (v.type() == null_type))))
        {
            string err = strprintf("Expected type %s, got %s",
                                   Value_type_name[t], Value_type_name[v.type()]);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
        i++;
    }
}

void RPCTypeCheck(const Object& o,
                  const map<string, Value_type>& typesExpected,
                  bool fAllowNull)
{
    BOOST_FOREACH(const PAIRTYPE(string, Value_type)& t, typesExpected)
    {
        const Value& v = find_value(o, t.first);
        if (!fAllowNull && v.type() == null_type)
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first));

        if (!((v.type() == t.second) || (fAllowNull && (v.type() == null_type))))
        {
            string err = strprintf("Expected type %s for %s, got %s",
                                   Value_type_name[t.second], t.first, Value_type_name[v.type()]);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
    }
}

int64_t AmountFromValue(const Value& value)
{
    double dAmount = value.get_real();
    if (dAmount <= 0.0 || dAmount > 21000000.0)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    int64_t nAmount = roundint64(dAmount * COIN);
    if (!MoneyRange(nAmount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    return nAmount;
}

Value ValueFromAmount(int64_t amount)
{
    return (double)amount / (double)COIN;
}

std::string HexBits(unsigned int nBits)
{
    union {
        int32_t nBits;
        char cBits[4];
    } uBits;
    uBits.nBits = htonl((int32_t)nBits);
    return HexStr(BEGIN(uBits.cBits), END(uBits.cBits));
}

uint256 ParseHashV(const Value& v, string strName)
{
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    uint256 result;
    result.SetHex(strHex);
    return result;
}
uint256 ParseHashO(const Object& o, string strKey)
{
    return ParseHashV(find_value(o, strKey), strKey);
}
vector<unsigned char> ParseHexV(const Value& v, string strName)
{
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return ParseHex(strHex);
}
vector<unsigned char> ParseHexO(const Object& o, string strKey)
{
    return ParseHexV(find_value(o, strKey), strKey);
}


///
/// Note: This interface may still be subject to change.
///

string CRPCTable::help(string strCommand) const
{
    string strRet;
    set<rpcfn_type> setDone;
    for (map<string, const CRPCCommand*>::const_iterator mi = mapCommands.begin(); mi != mapCommands.end(); ++mi)
    {
        const CRPCCommand *pcmd = mi->second;
        string strMethod = mi->first;
        // We already filter duplicates, but these deprecated screw up the sort order
        if (strMethod.find("label") != string::npos)
            continue;
        if (strCommand != "" && strMethod != strCommand)
            continue;
#ifdef ENABLE_WALLET
        if (pcmd->reqWallet && !pwalletMain)
            continue;
#endif

        try
        {
            Array params;
            rpcfn_type pfn = pcmd->actor;
            if (setDone.insert(pfn).second)
                (*pfn)(params, true);
        }
        catch (std::exception& e)
        {
            // Help text is returned in an exception
            string strHelp = string(e.what());
            if (strCommand == "")
                if (strHelp.find('\n') != string::npos)
                    strHelp = strHelp.substr(0, strHelp.find('\n'));
            strRet += strHelp + "\n";
        }
    }
    if (strRet == "")
        strRet = strprintf("help: unknown command: %s\n", strCommand);
    strRet = strRet.substr(0,strRet.size()-1);
    return strRet;
}

Value help(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "help ( \"command\" )\n"
            "\nList all commands, or get help for a specified command.\n"
            "\nArguments:\n"
            "1. \"command\"     (string, optional) The command to get help on\n"
            "\nResult:\n"
            "\"text\"     (string) The help text\n"
        );

    string strCommand;
    if (params.size() > 0)
        strCommand = params[0].get_str();

    return tableRPC.help(strCommand);
}


Value stop(const Array& params, bool fHelp)
{
    // Accept the deprecated and ignored 'detach' boolean argument
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "stop\n"
            "\nStop Bitcoin server.");
    // Shutdown will take long enough that the response should get back
    StartShutdown();
    return "Bitcoin server stopping";
}



//
// Call Table
//


static const CRPCCommand vRPCCommands[] =
{ //  name                      actor (function)         okSafeMode threadSafe reqWallet
  //  ------------------------  -----------------------  ---------- ---------- ---------
    /* Overall control/query calls */
    { "getinfo",                &getinfo,                true,      false,      false }, /* uses wallet if enabled */
    { "help",                   &help,                   true,      true,       false },
    { "stop",                   &stop,                   true,      true,       false },

    /* P2P networking */
    { "getnetworkinfo",         &getnetworkinfo,         true,      false,      false },
    { "addnode",                &addnode,                true,      true,       false },
    { "getaddednodeinfo",       &getaddednodeinfo,       true,      true,       false },
    { "getconnectioncount",     &getconnectioncount,     true,      false,      false },
    { "getnettotals",           &getnettotals,           true,      true,       false },
    { "getpeerinfo",            &getpeerinfo,            true,      false,      false },
    { "ping",                   &ping,                   true,      false,      false },

    /* Block chain and UTXO */
    { "getblockchaininfo",      &getblockchaininfo,      true,      false,      false },
    { "getbestblockhash",       &getbestblockhash,       true,      false,      false },
    { "getblockcount",          &getblockcount,          true,      false,      false },
    { "getblock",               &getblock,               false,     false,      false },
    { "getblockhash",           &getblockhash,           false,     false,      false },
    { "getdifficulty",          &getdifficulty,          true,      false,      false },
    { "getrawmempool",          &getrawmempool,          true,      false,      false },
    { "gettxout",               &gettxout,               true,      false,      false },
    { "gettxoutsetinfo",        &gettxoutsetinfo,        true,      false,      false },
    { "verifychain",            &verifychain,            true,      false,      false },

    /* Mining */
    { "getblocktemplate",       &getblocktemplate,       true,      false,      false },
    { "getmininginfo",          &getmininginfo,          true,      false,      false },
    { "getnetworkhashps",       &getnetworkhashps,       true,      false,      false },
    { "submitblock",            &submitblock,            false,     true,       false },

    /* Raw transactions */
    { "createrawtransaction",   &createrawtransaction,   false,     false,      false },
    { "decoderawtransaction",   &decoderawtransaction,   false,     false,      false },
    { "decodescript",           &decodescript,           false,     false,      false },
    { "getrawtransaction",      &getrawtransaction,      false,     false,      false },
    { "sendrawtransaction",     &sendrawtransaction,     false,     false,      false },
    { "signrawtransaction",     &signrawtransaction,     false,     false,      false }, /* uses wallet if enabled */

    /* Utility functions */
    { "createmultisig",         &createmultisig,         true,      true ,      false },
    { "validateaddress",        &validateaddress,        true,      false,      false }, /* uses wallet if enabled */
    { "verifymessage",          &verifymessage,          false,     false,      false },
    { "estimatefee",            &estimatefee,            true,      true,       false },
    { "estimatepriority",       &estimatepriority,       true,      true,       false },

#ifdef ENABLE_WALLET
    /* Wallet */
    { "addmultisigaddress",     &addmultisigaddress,     false,     false,      true },
    { "backupwallet",           &backupwallet,           true,      false,      true },
    { "dumpprivkey",            &dumpprivkey,            true,      false,      true },
    { "dumpwallet",             &dumpwallet,             true,      false,      true },
    { "encryptwallet",          &encryptwallet,          false,     false,      true },
    { "getaccountaddress",      &getaccountaddress,      true,      false,      true },
    { "getaccount",             &getaccount,             false,     false,      true },
    { "getaddressesbyaccount",  &getaddressesbyaccount,  true,      false,      true },
    { "getbalance",             &getbalance,             false,     false,      true },
    { "getnewaddress",          &getnewaddress,          true,      false,      true },
    { "getrawchangeaddress",    &getrawchangeaddress,    true,      false,      true },
    { "getreceivedbyaccount",   &getreceivedbyaccount,   false,     false,      true },
    { "getreceivedbyaddress",   &getreceivedbyaddress,   false,     false,      true },
    { "gettransaction",         &gettransaction,         false,     false,      true },
    { "getunconfirmedbalance",  &getunconfirmedbalance,  false,     false,      true },
    { "getwalletinfo",          &getwalletinfo,          true,      false,      true },
    { "importprivkey",          &importprivkey,          false,     false,      true },
    { "importwallet",           &importwallet,           false,     false,      true },
    { "keypoolrefill",          &keypoolrefill,          true,      false,      true },
    { "listaccounts",           &listaccounts,           false,     false,      true },
    { "listaddressgroupings",   &listaddressgroupings,   false,     false,      true },
    { "listlockunspent",        &listlockunspent,        false,     false,      true },
    { "listreceivedbyaccount",  &listreceivedbyaccount,  false,     false,      true },
    { "listreceivedbyaddress",  &listreceivedbyaddress,  false,     false,      true },
    { "listsinceblock",         &listsinceblock,         false,     false,      true },
    { "listtransactions",       &listtransactions,       false,     false,      true },
    { "listunspent",            &listunspent,            false,     false,      true },
    { "lockunspent",            &lockunspent,            false,     false,      true },
    { "move",                   &movecmd,                false,     false,      true },
    { "sendfrom",               &sendfrom,               false,     false,      true },
    { "sendmany",               &sendmany,               false,     false,      true },
    { "sendtoaddress",          &sendtoaddress,          false,     false,      true },
    { "setaccount",             &setaccount,             true,      false,      true },
    { "settxfee",               &settxfee,               false,     false,      true },
    { "signmessage",            &signmessage,            false,     false,      true },
    { "walletlock",             &walletlock,             true,      false,      true },
    { "walletpassphrasechange", &walletpassphrasechange, false,     false,      true },
    { "walletpassphrase",       &walletpassphrase,       true,      false,      true },

    /* Wallet-enabled mining */
    { "getgenerate",            &getgenerate,            true,      false,      false },
    { "gethashespersec",        &gethashespersec,        true,      false,      false },
    { "getwork",                &getwork,                true,      false,      true  },
    { "setgenerate",            &setgenerate,            true,      true,       false },
#endif // ENABLE_WALLET
};

CRPCTable::CRPCTable()
{
    unsigned int vcidx;
    for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++)
    {
        const CRPCCommand *pcmd;

        pcmd = &vRPCCommands[vcidx];
        mapCommands[pcmd->name] = pcmd;
    }
}

const CRPCCommand *CRPCTable::operator[](string name) const
{
    map<string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
    if (it == mapCommands.end())
        return NULL;
    return (*it).second;
}


bool HTTPAuthorized(map<string, string>& mapHeaders)
{
    string strAuth = mapHeaders["authorization"];
    if (strAuth.substr(0,6) != "Basic ")
        return false;
    string strUserPass64 = strAuth.substr(6); boost::trim(strUserPass64);
    string strUserPass = DecodeBase64(strUserPass64);
    return TimingResistantEqual(strUserPass, strRPCUserColonPass);
}

void ErrorReply(std::ostream& stream, const Object& objError, const Value& id)
{
    // Send error reply from json-rpc error object
    int nStatus = HTTP_INTERNAL_SERVER_ERROR;
    int code = find_value(objError, "code").get_int();
    if (code == RPC_INVALID_REQUEST) nStatus = HTTP_BAD_REQUEST;
    else if (code == RPC_METHOD_NOT_FOUND) nStatus = HTTP_NOT_FOUND;
    string strReply = JSONRPCReply(Value::null, objError, id);
    stream << HTTPReply(nStatus, strReply, false) << std::flush;
}

CNetAddr BoostAsioToCNetAddr(boost::asio::ip::address address)
{
    CNetAddr netaddr;
    // Make sure that IPv4-compatible and IPv4-mapped IPv6 addresses are treated as IPv4 addresses
    if (address.is_v6()
     && (address.to_v6().is_v4_compatible()
      || address.to_v6().is_v4_mapped()))
        address = address.to_v6().to_v4();

    if(address.is_v4())
    {
        boost::asio::ip::address_v4::bytes_type bytes = address.to_v4().to_bytes();
        netaddr.SetRaw(NET_IPV4, &bytes[0]);
    }
    else
    {
        boost::asio::ip::address_v6::bytes_type bytes = address.to_v6().to_bytes();
        netaddr.SetRaw(NET_IPV6, &bytes[0]);
    }
    return netaddr;
}

bool ClientAllowed(const boost::asio::ip::address& address)
{
    CNetAddr netaddr = BoostAsioToCNetAddr(address);
    BOOST_FOREACH(const CSubNet &subnet, rpc_allow_subnets)
        if (subnet.Match(netaddr))
            return true;
    return false;
}

class AcceptedConnection
{
public:
    virtual ~AcceptedConnection() {}

    virtual std::iostream& stream() = 0;
    virtual std::string peer_address_to_string() const = 0;
    virtual void close() = 0;
};

template <typename Protocol>
class AcceptedConnectionImpl : public AcceptedConnection
{
public:
    AcceptedConnectionImpl(
            asio::io_service& io_service,
            ssl::context &context,
            bool fUseSSL) :
        sslStream(io_service, context),
        _d(sslStream, fUseSSL),
        _stream(_d)
    {
    }

    virtual std::iostream& stream()
    {
        return _stream;
    }

    virtual std::string peer_address_to_string() const
    {
        return peer.address().to_string();
    }

    virtual void close()
    {
        _stream.close();
    }

    typename Protocol::endpoint peer;
    asio::ssl::stream<typename Protocol::socket> sslStream;

private:
    SSLIOStreamDevice<Protocol> _d;
    iostreams::stream< SSLIOStreamDevice<Protocol> > _stream;
};

void ServiceConnection(AcceptedConnection *conn);

// Forward declaration required for RPCListen
template <typename Protocol, typename SocketAcceptorService>
static void RPCAcceptHandler(boost::shared_ptr< basic_socket_acceptor<Protocol, SocketAcceptorService> > acceptor,
                             ssl::context& context,
                             bool fUseSSL,
                             boost::shared_ptr< AcceptedConnection > conn,
                             const boost::system::error_code& error);

/**
 * Sets up I/O resources to accept and handle a new connection.
 */
template <typename Protocol, typename SocketAcceptorService>
static void RPCListen(boost::shared_ptr< basic_socket_acceptor<Protocol, SocketAcceptorService> > acceptor,
                   ssl::context& context,
                   const bool fUseSSL)
{
    // Accept connection
    boost::shared_ptr< AcceptedConnectionImpl<Protocol> > conn(new AcceptedConnectionImpl<Protocol>(acceptor->get_io_service(), context, fUseSSL));

    acceptor->async_accept(
            conn->sslStream.lowest_layer(),
            conn->peer,
            boost::bind(&RPCAcceptHandler<Protocol, SocketAcceptorService>,
                acceptor,
                boost::ref(context),
                fUseSSL,
                conn,
                _1));
}


/**
 * Accept and handle incoming connection.
 */
template <typename Protocol, typename SocketAcceptorService>
static void RPCAcceptHandler(boost::shared_ptr< basic_socket_acceptor<Protocol, SocketAcceptorService> > acceptor,
                             ssl::context& context,
                             const bool fUseSSL,
                             boost::shared_ptr< AcceptedConnection > conn,
                             const boost::system::error_code& error)
{
    // Immediately start accepting new connections, except when we're cancelled or our socket is closed.
    if (error != asio::error::operation_aborted && acceptor->is_open())
        RPCListen(acceptor, context, fUseSSL);

    AcceptedConnectionImpl<ip::tcp>* tcp_conn = dynamic_cast< AcceptedConnectionImpl<ip::tcp>* >(conn.get());

    if (error)
    {
        // TODO: Actually handle errors
        LogPrintf("%s: Error: %s\n", __func__, error.message());
    }
    // Restrict callers by IP.  It is important to
    // do this before starting client thread, to filter out
    // certain DoS and misbehaving clients.
    else if (tcp_conn && !ClientAllowed(tcp_conn->peer.address()))
    {
        // Only send a 403 if we're not using SSL to prevent a DoS during the SSL handshake.
        if (!fUseSSL)
            conn->stream() << HTTPReply(HTTP_FORBIDDEN, "", false) << std::flush;
        conn->close();
    }
    else {
        ServiceConnection(conn.get());
        conn->close();
    }
}

static ip::tcp::endpoint ParseEndpoint(const std::string &strEndpoint, int defaultPort)
{
    std::string addr;
    int port = defaultPort;
    SplitHostPort(strEndpoint, port, addr);
    return ip::tcp::endpoint(asio::ip::address::from_string(addr), port);
}

void StartRPCThreads()
{
    rpc_allow_subnets.clear();
    rpc_allow_subnets.push_back(CSubNet("127.0.0.0/8")); // always allow IPv4 local subnet
    rpc_allow_subnets.push_back(CSubNet("::1")); // always allow IPv6 localhost
    if (mapMultiArgs.count("-rpcallowip"))
    {
        const vector<string>& vAllow = mapMultiArgs["-rpcallowip"];
        BOOST_FOREACH(string strAllow, vAllow)
        {
            CSubNet subnet(strAllow);
            if(!subnet.IsValid())
            {
                uiInterface.ThreadSafeMessageBox(
                    strprintf("Invalid -rpcallowip subnet specification: %s", strAllow),
                    "", CClientUIInterface::MSG_ERROR);
                StartShutdown();
                return;
            }
            rpc_allow_subnets.push_back(subnet);
        }
    }
    std::string strAllowed;
    BOOST_FOREACH(const CSubNet &subnet, rpc_allow_subnets)
        strAllowed += subnet.ToString() + " ";
    LogPrint("rpc", "Allowing RPC connections from: %s\n", strAllowed);

    strRPCUserColonPass = mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"];
    if (((mapArgs["-rpcpassword"] == "") ||
         (mapArgs["-rpcuser"] == mapArgs["-rpcpassword"])) && Params().RequireRPCPassword())
    {
        unsigned char rand_pwd[32];
        RAND_bytes(rand_pwd, 32);
        string strWhatAmI = "To use bitcoind";
        if (mapArgs.count("-server"))
            strWhatAmI = strprintf(_("To use the %s option"), "\"-server\"");
        else if (mapArgs.count("-daemon"))
            strWhatAmI = strprintf(_("To use the %s option"), "\"-daemon\"");
        uiInterface.ThreadSafeMessageBox(strprintf(
            _("%s, you must set a rpcpassword in the configuration file:\n"
              "%s\n"
              "It is recommended you use the following random password:\n"
              "rpcuser=bitcoinrpc\n"
              "rpcpassword=%s\n"
              "(you do not need to remember this password)\n"
              "The username and password MUST NOT be the same.\n"
              "If the file does not exist, create it with owner-readable-only file permissions.\n"
              "It is also recommended to set alertnotify so you are notified of problems;\n"
              "for example: alertnotify=echo %%s | mail -s \"Bitcoin Alert\" admin@foo.com\n"),
                strWhatAmI,
                GetConfigFile().string(),
                EncodeBase58(&rand_pwd[0],&rand_pwd[0]+32)),
                "", CClientUIInterface::MSG_ERROR);
        StartShutdown();
        return;
    }

    assert(rpc_io_service == NULL);
    rpc_io_service = new asio::io_service();
    rpc_ssl_context = new ssl::context(*rpc_io_service, ssl::context::sslv23);

    const bool fUseSSL = GetBoolArg("-rpcssl", false);

    if (fUseSSL)
    {
        rpc_ssl_context->set_options(ssl::context::no_sslv2);

        filesystem::path pathCertFile(GetArg("-rpcsslcertificatechainfile", "server.cert"));
        if (!pathCertFile.is_complete()) pathCertFile = filesystem::path(GetDataDir()) / pathCertFile;
        if (filesystem::exists(pathCertFile)) rpc_ssl_context->use_certificate_chain_file(pathCertFile.string());
        else LogPrintf("ThreadRPCServer ERROR: missing server certificate file %s\n", pathCertFile.string());

        filesystem::path pathPKFile(GetArg("-rpcsslprivatekeyfile", "server.pem"));
        if (!pathPKFile.is_complete()) pathPKFile = filesystem::path(GetDataDir()) / pathPKFile;
        if (filesystem::exists(pathPKFile)) rpc_ssl_context->use_private_key_file(pathPKFile.string(), ssl::context::pem);
        else LogPrintf("ThreadRPCServer ERROR: missing server private key file %s\n", pathPKFile.string());

        string strCiphers = GetArg("-rpcsslciphers", "TLSv1.2+HIGH:TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!3DES:@STRENGTH");
        SSL_CTX_set_cipher_list(rpc_ssl_context->impl(), strCiphers.c_str());
    }

    std::vector<ip::tcp::endpoint> vEndpoints;
    bool bBindAny = false;
    int defaultPort = GetArg("-rpcport", Params().RPCPort());
    if (!mapArgs.count("-rpcallowip")) // Default to loopback if not allowing external IPs
    {
        vEndpoints.push_back(ip::tcp::endpoint(asio::ip::address_v6::loopback(), defaultPort));
        vEndpoints.push_back(ip::tcp::endpoint(asio::ip::address_v4::loopback(), defaultPort));
        if (mapArgs.count("-rpcbind"))
        {
            LogPrintf("WARNING: option -rpcbind was ignored because -rpcallowip was not specified, refusing to allow everyone to connect\n");
        }
    } else if (mapArgs.count("-rpcbind")) // Specific bind address
    {
        BOOST_FOREACH(const std::string &addr, mapMultiArgs["-rpcbind"])
        {
            try {
                vEndpoints.push_back(ParseEndpoint(addr, defaultPort));
            }
            catch(boost::system::system_error &e)
            {
                uiInterface.ThreadSafeMessageBox(
                    strprintf(_("Could not parse -rpcbind value %s as network address"), addr),
                    "", CClientUIInterface::MSG_ERROR);
                StartShutdown();
                return;
            }
        }
    } else { // No specific bind address specified, bind to any
        vEndpoints.push_back(ip::tcp::endpoint(asio::ip::address_v6::any(), defaultPort));
        vEndpoints.push_back(ip::tcp::endpoint(asio::ip::address_v4::any(), defaultPort));
        // Prefer making the socket dual IPv6/IPv4 instead of binding
        // to both addresses seperately.
        bBindAny = true;
    }

    bool fListening = false;
    std::string strerr;
    BOOST_FOREACH(const ip::tcp::endpoint &endpoint, vEndpoints)
    {
        asio::ip::address bindAddress = endpoint.address();
        LogPrintf("Binding RPC on address %s port %i (IPv4+IPv6 bind any: %i)\n", bindAddress.to_string(), endpoint.port(), bBindAny);
        boost::system::error_code v6_only_error;
        boost::shared_ptr<ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(*rpc_io_service));
        rpc_acceptors.push_back(acceptor);

        try {
            acceptor->open(endpoint.protocol());
            acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

            // Try making the socket dual IPv6/IPv4 when listening on the IPv6 "any" address
            acceptor->set_option(boost::asio::ip::v6_only(
                !bBindAny || bindAddress != asio::ip::address_v6::any()), v6_only_error);

            acceptor->bind(endpoint);
            acceptor->listen(socket_base::max_connections);

            RPCListen(acceptor, *rpc_ssl_context, fUseSSL);

            fListening = true;
            // If dual IPv6/IPv4 bind succesful, skip binding to IPv4 separately
            if(bBindAny && bindAddress == asio::ip::address_v6::any() && !v6_only_error)
                break;
        }
        catch(boost::system::system_error &e)
        {
            LogPrintf("ERROR: Binding RPC on address %s port %i failed: %s\n", bindAddress.to_string(), endpoint.port(), e.what());
            strerr = strprintf(_("An error occurred while setting up the RPC address %s port %u for listening: %s"), bindAddress.to_string(), endpoint.port(), e.what());
        }
    }

    if (!fListening) {
        uiInterface.ThreadSafeMessageBox(strerr, "", CClientUIInterface::MSG_ERROR);
        StartShutdown();
        return;
    }

    rpc_worker_group = new boost::thread_group();
    for (int i = 0; i < GetArg("-rpcthreads", 4); i++)
        rpc_worker_group->create_thread(boost::bind(&asio::io_service::run, rpc_io_service));
}

void StartDummyRPCThread()
{
    if(rpc_io_service == NULL)
    {
        rpc_io_service = new asio::io_service();
        /* Create dummy "work" to keep the thread from exiting when no timeouts active,
         * see http://www.boost.org/doc/libs/1_51_0/doc/html/boost_asio/reference/io_service.html#boost_asio.reference.io_service.stopping_the_io_service_from_running_out_of_work */
        rpc_dummy_work = new asio::io_service::work(*rpc_io_service);
        rpc_worker_group = new boost::thread_group();
        rpc_worker_group->create_thread(boost::bind(&asio::io_service::run, rpc_io_service));
    }
}

void StopRPCThreads()
{
    if (rpc_io_service == NULL) return;

    // First, cancel all timers and acceptors
    // This is not done automatically by ->stop(), and in some cases the destructor of
    // asio::io_service can hang if this is skipped.
    BOOST_FOREACH(const boost::shared_ptr<ip::tcp::acceptor> &acceptor, rpc_acceptors)
        acceptor->cancel();
    rpc_acceptors.clear();
    BOOST_FOREACH(const PAIRTYPE(std::string, boost::shared_ptr<deadline_timer>) &timer, deadlineTimers)
        timer.second->cancel();
    deadlineTimers.clear();

    rpc_io_service->stop();
    if (rpc_worker_group != NULL)
        rpc_worker_group->join_all();
    delete rpc_dummy_work; rpc_dummy_work = NULL;
    delete rpc_worker_group; rpc_worker_group = NULL;
    delete rpc_ssl_context; rpc_ssl_context = NULL;
    delete rpc_io_service; rpc_io_service = NULL;
}

void RPCRunHandler(const boost::system::error_code& err, boost::function<void(void)> func)
{
    if (!err)
        func();
}

void RPCRunLater(const std::string& name, boost::function<void(void)> func, int64_t nSeconds)
{
    assert(rpc_io_service != NULL);

    if (deadlineTimers.count(name) == 0)
    {
        deadlineTimers.insert(make_pair(name,
                                        boost::shared_ptr<deadline_timer>(new deadline_timer(*rpc_io_service))));
    }
    deadlineTimers[name]->expires_from_now(posix_time::seconds(nSeconds));
    deadlineTimers[name]->async_wait(boost::bind(RPCRunHandler, _1, func));
}

class JSONRequest
{
public:
    Value id;
    string strMethod;
    Array params;

    JSONRequest() { id = Value::null; }
    void parse(const Value& valRequest);
};

void JSONRequest::parse(const Value& valRequest)
{
    // Parse request
    if (valRequest.type() != obj_type)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Request object");
    const Object& request = valRequest.get_obj();

    // Parse id now so errors from here on will have the id
    id = find_value(request, "id");

    // Parse method
    Value valMethod = find_value(request, "method");
    if (valMethod.type() == null_type)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Missing method");
    if (valMethod.type() != str_type)
        throw JSONRPCError(RPC_INVALID_REQUEST, "Method must be a string");
    strMethod = valMethod.get_str();
    if (strMethod != "getwork" && strMethod != "getblocktemplate")
        LogPrint("rpc", "ThreadRPCServer method=%s\n", strMethod);

    // Parse params
    Value valParams = find_value(request, "params");
    if (valParams.type() == array_type)
        params = valParams.get_array();
    else if (valParams.type() == null_type)
        params = Array();
    else
        throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array");
}


static Object JSONRPCExecOne(const Value& req)
{
    Object rpc_result;

    JSONRequest jreq;
    try {
        jreq.parse(req);

        Value result = tableRPC.execute(jreq.strMethod, jreq.params);
        rpc_result = JSONRPCReplyObj(result, Value::null, jreq.id);
    }
    catch (Object& objError)
    {
        rpc_result = JSONRPCReplyObj(Value::null, objError, jreq.id);
    }
    catch (std::exception& e)
    {
        rpc_result = JSONRPCReplyObj(Value::null,
                                     JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
    }

    return rpc_result;
}

static string JSONRPCExecBatch(const Array& vReq)
{
    Array ret;
    for (unsigned int reqIdx = 0; reqIdx < vReq.size(); reqIdx++)
        ret.push_back(JSONRPCExecOne(vReq[reqIdx]));

    return write_string(Value(ret), false) + "\n";
}

void ServiceConnection(AcceptedConnection *conn)
{
    bool fRun = true;
    while (fRun && !ShutdownRequested())
    {
        int nProto = 0;
        map<string, string> mapHeaders;
        string strRequest, strMethod, strURI;

        // Read HTTP request line
        if (!ReadHTTPRequestLine(conn->stream(), nProto, strMethod, strURI))
            break;

        // Read HTTP message headers and body
        ReadHTTPMessage(conn->stream(), mapHeaders, strRequest, nProto);

        if (strURI != "/") {
            conn->stream() << HTTPReply(HTTP_NOT_FOUND, "", false) << std::flush;
            break;
        }

        // Check authorization
        if (mapHeaders.count("authorization") == 0)
        {
            conn->stream() << HTTPReply(HTTP_UNAUTHORIZED, "", false) << std::flush;
            break;
        }
        if (!HTTPAuthorized(mapHeaders))
        {
            LogPrintf("ThreadRPCServer incorrect password attempt from %s\n", conn->peer_address_to_string());
            /* Deter brute-forcing short passwords.
               If this results in a DoS the user really
               shouldn't have their RPC port exposed. */
            if (mapArgs["-rpcpassword"].size() < 20)
                MilliSleep(250);

            conn->stream() << HTTPReply(HTTP_UNAUTHORIZED, "", false) << std::flush;
            break;
        }
        if (mapHeaders["connection"] == "close")
            fRun = false;

        JSONRequest jreq;
        try
        {
            // Parse request
            Value valRequest;
            if (!read_string(strRequest, valRequest))
                throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");

            string strReply;

            // singleton request
            if (valRequest.type() == obj_type) {
                jreq.parse(valRequest);

                Value result = tableRPC.execute(jreq.strMethod, jreq.params);

                // Send reply
                strReply = JSONRPCReply(result, Value::null, jreq.id);

            // array of requests
            } else if (valRequest.type() == array_type)
                strReply = JSONRPCExecBatch(valRequest.get_array());
            else
                throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");

            conn->stream() << HTTPReply(HTTP_OK, strReply, fRun) << std::flush;
        }
        catch (Object& objError)
        {
            ErrorReply(conn->stream(), objError, jreq.id);
            break;
        }
        catch (std::exception& e)
        {
            ErrorReply(conn->stream(), JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
            break;
        }
    }
}

json_spirit::Value CRPCTable::execute(const std::string &strMethod, const json_spirit::Array &params) const
{
    // Find method
    const CRPCCommand *pcmd = tableRPC[strMethod];
    if (!pcmd)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");
#ifdef ENABLE_WALLET
    if (pcmd->reqWallet && !pwalletMain)
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found (disabled)");
#endif

    // Observe safe mode
    string strWarning = GetWarnings("rpc");
    if (strWarning != "" && !GetBoolArg("-disablesafemode", false) &&
        !pcmd->okSafeMode)
        throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, string("Safe mode: ") + strWarning);

    try
    {
        // Execute
        Value result;
        {
            if (pcmd->threadSafe)
                result = pcmd->actor(params, false);
#ifdef ENABLE_WALLET
            else if (!pwalletMain) {
                LOCK(cs_main);
                result = pcmd->actor(params, false);
            } else {
                LOCK2(cs_main, pwalletMain->cs_wallet);
                result = pcmd->actor(params, false);
            }
#else // ENABLE_WALLET
            else {
                LOCK(cs_main);
                result = pcmd->actor(params, false);
            }
#endif // !ENABLE_WALLET
        }
        return result;
    }
    catch (std::exception& e)
    {
        throw JSONRPCError(RPC_MISC_ERROR, e.what());
    }
}

std::string HelpExampleCli(string methodname, string args){
    return "> bitcoin-cli " + methodname + " " + args + "\n";
}

std::string HelpExampleRpc(string methodname, string args){
    return "> curl --user myusername --data-binary '{\"jsonrpc\": \"1.0\", \"id\":\"curltest\", "
        "\"method\": \"" + methodname + "\", \"params\": [" + args + "] }' -H 'content-type: text/plain;' http://127.0.0.1:8332/\n";
}

const CRPCTable tableRPC;
