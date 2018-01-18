// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcnestedtests.h"

#include "chainparams.h"
#include "consensus/validation.h"
#include "fs.h"
#include "validation.h"
#include "rpc/register.h"
#include "rpc/server.h"
#include "rpcconsole.h"
#include "test/test_bitcoin.h"
#include "univalue.h"
#include "util.h"

#include <QDir>
#include <QtGlobal>

static UniValue rpcNestedTest_rpc(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        return "help message";
    }
    return request.params.write(0, 0);
}

static const CRPCCommand vRPCCommands[] =
{
    { "test", "rpcNestedTest", &rpcNestedTest_rpc, true, {} },
};

void RPCNestedTests::rpcNestedTests()
{
    // do some test setup
    // could be moved to a more generic place when we add more tests on QT level
    tableRPC.appendCommand("rpcNestedTest", &vRPCCommands[0]);
    //mempool.setSanityCheck(1.0);

    TestingSetup test;

    SetRPCWarmupFinished();

    std::string result;
    std::string result2;
    std::string filtered;
    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()[chain]", &filtered); //simple result filtering with path
    QVERIFY(result=="main");
    QVERIFY(filtered == "getblockchaininfo()[chain]");

    RPCConsole::RPCExecuteCommandLine(result, "getblock(getbestblockhash())"); //simple 2 level nesting
    RPCConsole::RPCExecuteCommandLine(result, "getblock(getblock(getbestblockhash())[hash], true)");

    RPCConsole::RPCExecuteCommandLine(result, "getblock( getblock( getblock(getbestblockhash())[hash] )[hash], true)"); //4 level nesting with whitespace, filtering path and boolean parameter

    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo");
    QVERIFY(result.substr(0,1) == "{");

    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()");
    QVERIFY(result.substr(0,1) == "{");

    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo "); //whitespace at the end will be tolerated
    QVERIFY(result.substr(0,1) == "{");

    (RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()[\"chain\"]")); //Quote path identifier are allowed, but look after a child contaning the quotes in the key
    QVERIFY(result == "null");

    (RPCConsole::RPCExecuteCommandLine(result, "createrawtransaction [] {} 0")); //parameter not in brackets are allowed
    (RPCConsole::RPCExecuteCommandLine(result2, "createrawtransaction([],{},0)")); //parameter in brackets are allowed
    QVERIFY(result == result2);
    (RPCConsole::RPCExecuteCommandLine(result2, "createrawtransaction( [],  {} , 0   )")); //whitespace between parametres is allowed
    QVERIFY(result == result2);

    RPCConsole::RPCExecuteCommandLine(result, "getblock(getbestblockhash())[tx][0]", &filtered);
    QVERIFY(result == "97ddfbbae6be97fd6cdf3e7ca13232a3afff2353e29badfab7f73011edd4ced9");
    QVERIFY(filtered == "getblock(getbestblockhash())[tx][0]");

    RPCConsole::RPCParseCommandLine(result, "importprivkey", false, &filtered);
    QVERIFY(filtered == "importprivkey(…)");
    RPCConsole::RPCParseCommandLine(result, "signmessagewithprivkey abc", false, &filtered);
    QVERIFY(filtered == "signmessagewithprivkey(…)");
    RPCConsole::RPCParseCommandLine(result, "signmessagewithprivkey abc,def", false, &filtered);
    QVERIFY(filtered == "signmessagewithprivkey(…)");
    RPCConsole::RPCParseCommandLine(result, "signrawtransaction(abc)", false, &filtered);
    QVERIFY(filtered == "signrawtransaction(…)");
    RPCConsole::RPCParseCommandLine(result, "walletpassphrase(help())", false, &filtered);
    QVERIFY(filtered == "walletpassphrase(…)");
    RPCConsole::RPCParseCommandLine(result, "walletpassphrasechange(help(walletpassphrasechange(abc)))", false, &filtered);
    QVERIFY(filtered == "walletpassphrasechange(…)");
    RPCConsole::RPCParseCommandLine(result, "help(encryptwallet(abc, def))", false, &filtered);
    QVERIFY(filtered == "help(encryptwallet(…))");
    RPCConsole::RPCParseCommandLine(result, "help(importprivkey())", false, &filtered);
    QVERIFY(filtered == "help(importprivkey(…))");
    RPCConsole::RPCParseCommandLine(result, "help(importprivkey(help()))", false, &filtered);
    QVERIFY(filtered == "help(importprivkey(…))");
    RPCConsole::RPCParseCommandLine(result, "help(importprivkey(abc), walletpassphrase(def))", false, &filtered);
    QVERIFY(filtered == "help(importprivkey(…), walletpassphrase(…))");

    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest");
    QVERIFY(result == "[]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest ''");
    QVERIFY(result == "[\"\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest \"\"");
    QVERIFY(result == "[\"\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest '' abc");
    QVERIFY(result == "[\"\",\"abc\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest abc '' abc");
    QVERIFY(result == "[\"abc\",\"\",\"abc\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest abc  abc");
    QVERIFY(result == "[\"abc\",\"abc\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest abc\t\tabc");
    QVERIFY(result == "[\"abc\",\"abc\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest(abc )");
    QVERIFY(result == "[\"abc\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest( abc )");
    QVERIFY(result == "[\"abc\"]");
    RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest(   abc   ,   cba )");
    QVERIFY(result == "[\"abc\",\"cba\"]");

#if QT_VERSION >= 0x050300
    // do the QVERIFY_EXCEPTION_THROWN checks only with Qt5.3 and higher (QVERIFY_EXCEPTION_THROWN was introduced in Qt5.3)
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo() .\n"), std::runtime_error); //invalid syntax
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo() getblockchaininfo()"), std::runtime_error); //invalid syntax
    (RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo(")); //tolerate non closing brackets if we have no arguments
    (RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()()()")); //tolerate non command brackts
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo(True)"), UniValue); //invalid argument
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "a(getblockchaininfo(True))"), UniValue); //method not found
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest abc,,abc"), std::runtime_error); //don't tollerate empty arguments when using ,
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest(abc,,abc)"), std::runtime_error); //don't tollerate empty arguments when using ,
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "rpcNestedTest(abc,,)"), std::runtime_error); //don't tollerate empty arguments when using ,
#endif
}
