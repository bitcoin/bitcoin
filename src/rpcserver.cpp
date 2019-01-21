// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2011-2019 The Peercoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcserver.h"

#include "base58.h"
#include "init.h"
#include "checkpoints.h"
#include "util.h"
#include "sync.h"
#include "ui_interface.h"
#include "base58.h"
#include "db.h"
#include "kernelrecord.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/v6_only.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

using namespace std;
using namespace boost;
using namespace boost::asio;
using namespace json_spirit;

// Key used by getwork/getblocktemplate miners.
// Allocated in StartRPCThreads, free'd in StopRPCThreads
CReserveKey* pMiningKey = NULL;

static std::string strRPCUserColonPass;

// These are created by StartRPCThreads, destroyed in StopRPCThreads
static asio::io_service* rpc_io_service = NULL;
static map<string, boost::shared_ptr<deadline_timer> > deadlineTimers;
static ssl::context* rpc_ssl_context = NULL;
static boost::thread_group* rpc_worker_group = NULL;

static inline unsigned short GetDefaultRPCPort()
{
    return GetBoolArg("-testnet", false) ? TESTNET_RPC_PORT : RPC_PORT;
}

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
            throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Missing %s", t.first.c_str()));

        if (!((v.type() == t.second) || (fAllowNull && (v.type() == null_type))))
        {
            string err = strprintf("Expected type %s for %s, got %s",
                                   Value_type_name[t.second], t.first.c_str(), Value_type_name[v.type()]);
            throw JSONRPCError(RPC_TYPE_ERROR, err);
        }
    }
}

int64 AmountFromValue(const Value& value)
{
    double dAmount = value.get_real();
    if (dAmount <= 0.0 || dAmount > MAX_MONEY)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    int64 nAmount = roundint64(dAmount * COIN);
    if (!MoneyRange(nAmount))
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");
    return nAmount;
}

Value ValueFromAmount(int64 amount)
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



///
/// Note: This interface may still be subject to change.
///

// ppcoin: remove transaction from mempool
Value removetransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "removetransaction txid\n"
            "Removes transaction id from memory pool.");

    uint256 hash;
    hash.SetHex(params[0].get_str());

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock))
        throw JSONRPCError(-5, "No information available about transaction in mempool");

    mempool.remove(tx);
    return tx.GetHash().GetHex();
}

// ppcoin: reserve balance from being staked for network protection
Value reservebalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "reservebalance [<reserve> [amount]]\n"
            "<reserve> is true or false to turn balance reserve on or off.\n"
            "<amount> is a real and rounded to cent.\n"
            "Set reserve amount not participating in network protection.\n"
            "If no parameters provided current setting is printed.\n");

    if (params.size() > 0)
    {
        bool fReserve = params[0].get_bool();
        if (fReserve)
        {
            if (params.size() == 1)
                throw runtime_error("must provide amount to reserve balance.\n");
            int64 nAmount = AmountFromValue(params[1]);
            nAmount = (nAmount / CENT) * CENT;  // round to cent
            if (nAmount < 0)
                throw runtime_error("amount cannot be negative.\n");
            mapArgs["-reservebalance"] = FormatMoney(nAmount).c_str();
        }
        else
        {
            if (params.size() > 1)
                throw runtime_error("cannot specify amount to turn off reserve.\n");
            mapArgs["-reservebalance"] = "0";
        }
    }

    Object result;
    int64 nReserveBalance = 0;
    if (mapArgs.count("-reservebalance") && !ParseMoney(mapArgs["-reservebalance"], nReserveBalance))
        throw runtime_error("invalid reserve balance amount\n");
    result.push_back(Pair("reserve", (nReserveBalance > 0)));
    result.push_back(Pair("amount", ValueFromAmount(nReserveBalance)));
    return result;
}


// ppcoin: check wallet integrity
Value checkwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "checkwallet\n"
            "Check wallet for integrity.\n");

    int nMismatchSpent;
    int64 nBalanceInQuestion;
    pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion, true);
    Object result;
    if (nMismatchSpent == 0)
        result.push_back(Pair("wallet check passed", true));
    else
    {
        result.push_back(Pair("mismatched spent coins", nMismatchSpent));
        result.push_back(Pair("amount in question", ValueFromAmount(nBalanceInQuestion)));
    }
    return result;
}


// ppcoin: repair wallet
Value repairwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "repairwallet\n"
            "Repair wallet if checkwallet reports any problem.\n");

    int nMismatchSpent;
    int64 nBalanceInQuestion;
    pwalletMain->FixSpentCoins(nMismatchSpent, nBalanceInQuestion);
    Object result;
    if (nMismatchSpent == 0)
        result.push_back(Pair("wallet check passed", true));
    else
    {
        result.push_back(Pair("mismatched spent coins", nMismatchSpent));
        result.push_back(Pair("amount affected by repair", ValueFromAmount(nBalanceInQuestion)));
    }
    return result;
}

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
        strRet = strprintf("help: unknown command: %s\n", strCommand.c_str());
    strRet = strRet.substr(0,strRet.size()-1);
    return strRet;
}

Value help(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "help [command]\n"
            "List commands, or get help for a command.");

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
            "Stop Peercoin server.");
    // Shutdown will take long enough that the response should get back
    StartShutdown();
    return "Peercoin server stopping";
}



#ifdef TESTING

Value generatestake(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "generatestake\n"
            "generate a single proof of stake block"
            );

    if (GetBoolArg("-stakegen", true))
        throw JSONRPCError(-3, "Stake generation enabled. Won't start another generation.");

    BitcoinMiner(pwalletMain, true, true);
    return hashSingleStakeBlock.ToString();
}


extern bool fRequestShutdown;
Value shutdown(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "shutdown\n"
            "close the program"
            );

    fRequestShutdown = true;

    return "";
}


Value timetravel(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "timetravel <seconds>\n"
            "change relative time"
            );

    nTimeShift += params[0].get_int();

    return "";
}


unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake);

Value duplicateblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "duplicateblock <original hash> [<parent hash>]\n"
            "propagate a new block with the same stake as block <original hash> on top of <parent hash> (default: same parent)"
            );

    uint256 originalHash;
    originalHash.SetHex(params[0].get_str());
    if (!mapBlockIndex.count(originalHash))
        throw JSONRPCError(-3, "Original hash not in main chain");
    CBlockIndex *original = mapBlockIndex[originalHash];

    CBlockIndex *pindexPrev;
    if (params.size() > 1)
    {
        uint256 parentHash;
        parentHash.SetHex(params[1].get_str());
        if (!mapBlockIndex.count(parentHash))
            throw JSONRPCError(-3, "Parent hash not in main chain");
        pindexPrev = mapBlockIndex[parentHash];
    }
    else
        pindexPrev = original->pprev;

    CBlock block;
    block.ReadFromDisk(original);

    // Update parent block
    block.hashPrevBlock  = pindexPrev->GetBlockHash();
    block.nBits = GetNextTargetRequired(pindexPrev, block.IsProofOfStake());

    // Increment nonce to make sure the hash changes
    block.nNonce++;

    block.SignBlock(*pwalletMain); // We ignore errors to be able to test duplicate blocks with invalid signature

    // Process this block the same as if we had received it from another node
    // But do not check for errors as this is expected to fail
    CValidationState state;
    ProcessBlock(state, NULL, &block);

    return block.GetHash().ToString();
}

Value ignorenextblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "ignorenextblock"
            );

    nBlocksToIgnore++;

    return "";
}

#endif


//
// Call Table
//


static const CRPCCommand vRPCCommands[] =
{ //  name                      actor (function)         okSafeMode threadSafe
  //  ------------------------  -----------------------  ---------- ----------
    { "help",                   &help,                   true,      true },
    { "stop",                   &stop,                   true,      true },
    { "getblockcount",          &getblockcount,          true,      false },
    { "getconnectioncount",     &getconnectioncount,     true,      false },
    { "getpeerinfo",            &getpeerinfo,            true,      false },
    { "addnode",                &addnode,                true,      true },
    { "getaddednodeinfo",       &getaddednodeinfo,       true,      true },
    { "getdifficulty",          &getdifficulty,          true,      false },
    { "getgenerate",            &getgenerate,            true,      false },
#ifndef DISABLE_MINING
    { "setgenerate",            &setgenerate,            true,      false },
    { "gethashespersec",        &gethashespersec,        true,      false },
#endif // DISABLE_MINING
    { "getnetworkghps",         &getnetworkghps,         true,      false },
    { "getinfo",                &getinfo,                true,      false },
    { "getmininginfo",          &getmininginfo,          true,      false },
    { "getnewaddress",          &getnewaddress,          true,      false },
    { "getaccountaddress",      &getaccountaddress,      true,      false },
    { "setaccount",             &setaccount,             true,      false },
    { "getaccount",             &getaccount,             false,     false },
    { "getaddressesbyaccount",  &getaddressesbyaccount,  true,      false },
    { "sendtoaddress",          &sendtoaddress,          false,     false },
    { "getreceivedbyaddress",   &getreceivedbyaddress,   false,     false },
    { "getreceivedbyaccount",   &getreceivedbyaccount,   false,     false },
    { "listminting",            &listminting,            false,     false },
    { "listreceivedbyaddress",  &listreceivedbyaddress,  false,     false },
    { "listreceivedbyaccount",  &listreceivedbyaccount,  false,     false },
    { "backupwallet",           &backupwallet,           true,      false },
    { "keypoolrefill",          &keypoolrefill,          true,      false },
    { "walletpassphrase",       &walletpassphrase,       true,      false },
    { "walletpassphrasechange", &walletpassphrasechange, false,     false },
    { "walletlock",             &walletlock,             true,      false },
    { "encryptwallet",          &encryptwallet,          false,     false },
    { "validateaddress",        &validateaddress,        true,      false },
    { "getbalance",             &getbalance,             false,     false },
    { "move",                   &movecmd,                false,     false },
    { "sendfrom",               &sendfrom,               false,     false },
    { "sendmany",               &sendmany,               false,     false },
    { "addmultisigaddress",     &addmultisigaddress,     false,     false },
    { "createmultisig",         &createmultisig,         true,      true  },
    { "getrawmempool",          &getrawmempool,          true,      false },
    { "getblock",               &getblock,               false,     false },
    { "getblockhash",           &getblockhash,           false,     false },
    { "getbestblockhash",       &getbestblockhash,       false,     false },
    { "gettransaction",         &gettransaction,         false,     false },
    { "listtransactions",       &listtransactions,       false,     false },
    { "listaddressgroupings",   &listaddressgroupings,   false,     false },
    { "signmessage",            &signmessage,            false,     false },
    { "verifymessage",          &verifymessage,          false,     false },
#ifndef DISABLE_MINING
    { "getwork",                &getwork,                true,      false },
#endif // DISABLE_MINING
    { "listaccounts",           &listaccounts,           false,     false },
    { "settxfee",               &settxfee,               false,     false },
#ifndef DISABLE_MINING
    { "getblocktemplate",       &getblocktemplate,       true,      false },
#endif // DISABLE_MINING
    { "submitblock",            &submitblock,            false,     false },
    { "listsinceblock",         &listsinceblock,         false,     false },
    { "dumpprivkey",            &dumpprivkey,            true,      false },
    { "importprivkey",          &importprivkey,          false,     false },
    { "getcheckpoint",          &getcheckpoint,          true,      false },
    { "sendcheckpoint",         &sendcheckpoint,         true,      false },
    { "enforcecheckpoint",      &enforcecheckpoint,      true,      false },
    { "reservebalance",         &reservebalance,         false,     false },
    { "checkwallet",            &checkwallet,            false,     false },
    { "repairwallet",           &repairwallet,           false,     false },
    { "makekeypair",            &makekeypair,            false,     false },
    { "showkeypair",            &showkeypair,            false,     false },
    { "sendalert",              &sendalert,              false,     false },
    { "listunspent",            &listunspent,            false,     false },
    { "getrawtransaction",      &getrawtransaction,      false,     false },
    { "createrawtransaction",   &createrawtransaction,   false,     false },
    { "decoderawtransaction",   &decoderawtransaction,   false,     false },
    { "signrawtransaction",     &signrawtransaction,     false,     false },
    { "sendrawtransaction",     &sendrawtransaction,     false,     false },
    { "gettxoutsetinfo",        &gettxoutsetinfo,        true,      false },
    { "gettxout",               &gettxout,               true,      false },
    { "lockunspent",            &lockunspent,            false,     false },
    { "listlockunspent",        &listlockunspent,        false,     false },
    { "getbestblockhash",       &getbestblockhash,       false,     false },
    { "removetransaction",      &removetransaction,      false,     false },
#ifdef TESTING
    { "generatestake",          &generatestake,          true,      false },
    { "duplicateblock",         &duplicateblock,         true,      false },
    { "ignorenextblock",        &ignorenextblock,        true,      false },
    { "shutdown",               &shutdown,               true,      false },
    { "timetravel",             &timetravel,             true,      false },
#endif
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

bool ClientAllowed(const boost::asio::ip::address& address)
{
    // Make sure that IPv4-compatible and IPv4-mapped IPv6 addresses are treated as IPv4 addresses
    if (address.is_v6()
     && (address.to_v6().is_v4_compatible()
      || address.to_v6().is_v4_mapped()))
        return ClientAllowed(address.to_v6().to_v4());

    if (address == asio::ip::address_v4::loopback()
     || address == asio::ip::address_v6::loopback()
     || (address.is_v4()
         // Check whether IPv4 addresses match 127.0.0.0/8 (loopback subnet)
      && (address.to_v4().to_ulong() & 0xff000000) == 0x7f000000))
        return true;

    const string strAddress = address.to_string();
    const vector<string>& vAllow = mapMultiArgs["-rpcallowip"];
    BOOST_FOREACH(string strAllow, vAllow)
        if (WildcardMatch(strAddress, strAllow))
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
template <typename SocketAcceptor>
static void RPCAcceptHandler(boost::shared_ptr<SocketAcceptor> acceptor,
                             ssl::context& context,
                             bool fUseSSL,
                             AcceptedConnection* conn,
                             const boost::system::error_code& error);

/**
 * Sets up I/O resources to accept and handle a new connection.
 */
template <typename SocketAcceptor>
static void RPCListen(boost::shared_ptr<SocketAcceptor> acceptor,
                   ssl::context& context,
                   const bool fUseSSL)
{
    // Accept connection
    AcceptedConnectionImpl<typename SocketAcceptor::protocol_type>* conn = new AcceptedConnectionImpl<typename SocketAcceptor::protocol_type>(acceptor->get_io_service(), context, fUseSSL);

    acceptor->async_accept(
            conn->sslStream.lowest_layer(),
            conn->peer,
            boost::bind(&RPCAcceptHandler<SocketAcceptor>,
                acceptor,
                boost::ref(context),
                fUseSSL,
                conn,
                boost::asio::placeholders::error));
}


/**
 * Accept and handle incoming connection.
 */
template <typename SocketAcceptor>
static void RPCAcceptHandler(boost::shared_ptr<SocketAcceptor> acceptor,
                             ssl::context& context,
                             const bool fUseSSL,
                             AcceptedConnection* conn,
                             const boost::system::error_code& error)
{
    // Immediately start accepting new connections, except when we're cancelled or our socket is closed.
    if (error != asio::error::operation_aborted && acceptor->is_open())
        RPCListen(acceptor, context, fUseSSL);

    AcceptedConnectionImpl<ip::tcp>* tcp_conn = dynamic_cast< AcceptedConnectionImpl<ip::tcp>* >(conn);

    // TODO: Actually handle errors
    if (error)
    {
        delete conn;
    }

    // Restrict callers by IP.  It is important to
    // do this before starting client thread, to filter out
    // certain DoS and misbehaving clients.
    else if (tcp_conn && !ClientAllowed(tcp_conn->peer.address()))
    {
        // Only send a 403 if we're not using SSL to prevent a DoS during the SSL handshake.
        if (!fUseSSL)
            conn->stream() << HTTPReply(HTTP_FORBIDDEN, "", false) << std::flush;
        delete conn;
    }
    else {
        ServiceConnection(conn);
        conn->close();
        delete conn;
    }
}

void StartRPCThreads()
{
    // getwork/getblocktemplate mining rewards paid here:
    pMiningKey = new CReserveKey(pwalletMain);

    strRPCUserColonPass = mapArgs["-rpcuser"] + ":" + mapArgs["-rpcpassword"];
    if ((mapArgs["-rpcpassword"] == "") ||
        (mapArgs["-rpcuser"] == mapArgs["-rpcpassword"]))
    {
        unsigned char rand_pwd[32];
        RAND_bytes(rand_pwd, 32);
        string strWhatAmI = "To use peercoind";
        if (mapArgs.count("-server"))
            strWhatAmI = strprintf(_("To use the %s option"), "\"-server\"");
        else if (mapArgs.count("-daemon"))
            strWhatAmI = strprintf(_("To use the %s option"), "\"-daemon\"");
        uiInterface.ThreadSafeMessageBox(strprintf(
            _("%s, you must set a rpcpassword in the configuration file:\n"
              "%s\n"
              "It is recommended you use the following random password:\n"
              "rpcuser=peercoinrpc\n"
              "rpcpassword=%s\n"
              "(you do not need to remember this password)\n"
              "The username and password MUST NOT be the same.\n"
              "If the file does not exist, create it with owner-readable-only file permissions.\n"
              "It is also recommended to set alertnotify so you are notified of problems;\n"
              "for example: alertnotify=echo %%s | mail -s \"Peercoin Alert\" admin@foo.com\n"),
                strWhatAmI.c_str(),
                GetConfigFile().string().c_str(),
                EncodeBase58(&rand_pwd[0],&rand_pwd[0]+32).c_str()),
                "", CClientUIInterface::MSG_ERROR);
        StartShutdown();
        return;
    }

    assert(rpc_io_service == NULL);
    rpc_io_service = new asio::io_service();
    rpc_ssl_context = new ssl::context(ssl::context::sslv23);

    const bool fUseSSL = GetBoolArg("-rpcssl");

    if (fUseSSL)
    {
        rpc_ssl_context->set_options(ssl::context::no_sslv2);

        filesystem::path pathCertFile(GetArg("-rpcsslcertificatechainfile", "server.cert"));
        if (!pathCertFile.is_complete()) pathCertFile = filesystem::path(GetDataDir()) / pathCertFile;
        if (filesystem::exists(pathCertFile)) rpc_ssl_context->use_certificate_chain_file(pathCertFile.string());
        else printf("ThreadRPCServer ERROR: missing server certificate file %s\n", pathCertFile.string().c_str());

        filesystem::path pathPKFile(GetArg("-rpcsslprivatekeyfile", "server.pem"));
        if (!pathPKFile.is_complete()) pathPKFile = filesystem::path(GetDataDir()) / pathPKFile;
        if (filesystem::exists(pathPKFile)) rpc_ssl_context->use_private_key_file(pathPKFile.string(), ssl::context::pem);
        else printf("ThreadRPCServer ERROR: missing server private key file %s\n", pathPKFile.string().c_str());

        string strCiphers = GetArg("-rpcsslciphers", "TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!AH:!3DES:@STRENGTH");
        SSL_CTX_set_cipher_list(rpc_ssl_context->native_handle(), strCiphers.c_str());
    }

    // Try a dual IPv6/IPv4 socket, falling back to separate IPv4 and IPv6 sockets
    const bool loopback = !mapArgs.count("-rpcallowip");
    asio::ip::address bindAddress = loopback ? asio::ip::address_v6::loopback() : asio::ip::address_v6::any();
    ip::tcp::endpoint endpoint(bindAddress, GetArg("-rpcport", GetDefaultRPCPort()));
    boost::system::error_code v6_only_error;
    boost::shared_ptr<ip::tcp::acceptor> acceptor(new ip::tcp::acceptor(*rpc_io_service));

    bool fListening = false;
    std::string strerr;
    try
    {
        acceptor->open(endpoint.protocol());
        acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));

        // Try making the socket dual IPv6/IPv4 (if listening on the "any" address)
        acceptor->set_option(boost::asio::ip::v6_only(loopback), v6_only_error);

        acceptor->bind(endpoint);
        acceptor->listen(socket_base::max_connections);

        RPCListen(acceptor, *rpc_ssl_context, fUseSSL);

        fListening = true;
    }
    catch(boost::system::system_error &e)
    {
        strerr = strprintf(_("An error occurred while setting up the RPC port %u for listening on IPv6, falling back to IPv4: %s"), endpoint.port(), e.what());
    }

    try {
        // If dual IPv6/IPv4 failed (or we're opening loopback interfaces only), open IPv4 separately
        if (!fListening || loopback || v6_only_error)
        {
            bindAddress = loopback ? asio::ip::address_v4::loopback() : asio::ip::address_v4::any();
            endpoint.address(bindAddress);

            acceptor.reset(new ip::tcp::acceptor(*rpc_io_service));
            acceptor->open(endpoint.protocol());
            acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor->bind(endpoint);
            acceptor->listen(socket_base::max_connections);

            RPCListen(acceptor, *rpc_ssl_context, fUseSSL);

            fListening = true;
        }
    }
    catch(boost::system::system_error &e)
    {
        strerr = strprintf(_("An error occurred while setting up the RPC port %u for listening on IPv4: %s"), endpoint.port(), e.what());
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

void StopRPCThreads()
{
    delete pMiningKey; pMiningKey = NULL;

    if (rpc_io_service == NULL) return;

    deadlineTimers.clear();
    rpc_io_service->stop();
    rpc_worker_group->join_all();
    delete rpc_worker_group; rpc_worker_group = NULL;
    delete rpc_ssl_context; rpc_ssl_context = NULL;
    delete rpc_io_service; rpc_io_service = NULL;
}

void RPCRunHandler(const boost::system::error_code& err, boost::function<void(void)> func)
{
    if (!err)
        func();
}

void RPCRunLater(const std::string& name, boost::function<void(void)> func, int64 nSeconds)
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
        printf("ThreadRPCServer method=%s\n", strMethod.c_str());

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
    while (fRun)
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
            printf("ThreadRPCServer incorrect password attempt from %s\n", conn->peer_address_to_string().c_str());
            /* Deter brute-forcing short passwords.
               If this results in a DOS the user really
               shouldn't have their RPC port exposed.*/
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

    // Observe safe mode
    string strWarning = GetWarnings("rpc");
    if (strWarning != "" && !GetBoolArg("-disablesafemode") &&
        !pcmd->okSafeMode)
        throw JSONRPCError(RPC_FORBIDDEN_BY_SAFE_MODE, string("Safe mode: ") + strWarning);

    try
    {
        // Execute
        Value result;
        {
            if (pcmd->threadSafe)
                result = pcmd->actor(params, false);
            else {
                LOCK2(cs_main, pwalletMain->cs_wallet);
                result = pcmd->actor(params, false);
            }
        }
        return result;
    }
    catch (std::exception& e)
    {
        throw JSONRPCError(RPC_MISC_ERROR, e.what());
    }
}

const CRPCTable tableRPC;
