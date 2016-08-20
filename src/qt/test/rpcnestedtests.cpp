// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "rpcnestedtests.h"

#include "chainparams.h"
#include "consensus/validation.h"
#include "main.h"
#include "rpc/register.h"
#include "rpc/server.h"
#include "rpcconsole.h"
#include "test/testutil.h"
#include "univalue.h"
#include "util.h"

#include <QDir>

#include <boost/filesystem.hpp>

void RPCNestedTests::rpcNestedTests()
{
    UniValue jsonRPCError;

    // do some test setup
    // could be moved to a more generic place when we add more tests on QT level
    const CChainParams& chainparams = Params();
    RegisterAllCoreRPCCommands(tableRPC);
    ClearDatadirCache();
    std::string path = QDir::tempPath().toStdString() + "/" + strprintf("test_bitcoin_qt_%lu_%i", (unsigned long)GetTime(), (int)(GetRand(100000)));
    QDir dir(QString::fromStdString(path));
    dir.mkpath(".");
    mapArgs["-datadir"] = path;
    //mempool.setSanityCheck(1.0);
    pblocktree = new CBlockTreeDB(1 << 20, true);
    pcoinsdbview = new CCoinsViewDB(1 << 23, true);
    pcoinsTip = new CCoinsViewCache(pcoinsdbview);
    InitBlockIndex(chainparams);
    {
        CValidationState state;
        bool ok = ActivateBestChain(state, chainparams);
        QVERIFY(ok);
    }

    SetRPCWarmupFinished();

    std::string result;
    std::string result2;
    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()[chain]"); //simple result filtering with path
    QVERIFY(result=="main");

    RPCConsole::RPCExecuteCommandLine(result, "getblock(getbestblockhash())"); //simple 2 level nesting
    RPCConsole::RPCExecuteCommandLine(result, "getblock(getblock(getbestblockhash())[hash], true)");

    RPCConsole::RPCExecuteCommandLine(result, "getblock( getblock( getblock(getbestblockhash())[hash] )[hash], true)"); //4 level nesting with whitespace, filtering path and boolean parameter

    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo");
    QVERIFY(result.substr(0,1) == "{");

    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()");
    QVERIFY(result.substr(0,1) == "{");

    RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo "); //whitespace at the end will be tolerated
    QVERIFY(result.substr(0,1) == "{");

#if QT_VERSION >= 0x050300
    // do the QVERIFY_EXCEPTION_THROWN checks only with Qt5.3 and higher (QVERIFY_EXCEPTION_THROWN was introduced in Qt5.3)
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo() .\n"), std::runtime_error); //invalid syntax
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo() getblockchaininfo()"), std::runtime_error); //invalid syntax
    (RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo(")); //tolerate non closing brackets if we have no arguments
    (RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()()()")); //tolerate non command brackts
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo(True)"), UniValue); //invalid argument
    QVERIFY_EXCEPTION_THROWN(RPCConsole::RPCExecuteCommandLine(result, "a(getblockchaininfo(True))"), UniValue); //method not found
#endif

    (RPCConsole::RPCExecuteCommandLine(result, "getblockchaininfo()[\"chain\"]")); //Quote path identifier are allowed, but look after a child contaning the quotes in the key
    QVERIFY(result == "null");

    (RPCConsole::RPCExecuteCommandLine(result, "createrawtransaction [] {} 0")); //parameter not in brackets are allowed
    (RPCConsole::RPCExecuteCommandLine(result2, "createrawtransaction([],{},0)")); //parameter in brackets are allowed
    QVERIFY(result == result2);
    (RPCConsole::RPCExecuteCommandLine(result2, "createrawtransaction( [],  {} , 0   )")); //whitespace between parametres is allowed
    QVERIFY(result == result2);

    RPCConsole::RPCExecuteCommandLine(result, "getblock(getbestblockhash())[tx][0]");
    QVERIFY(result == "4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");

    delete pcoinsTip;
    delete pcoinsdbview;
    delete pblocktree;

    boost::filesystem::remove_all(boost::filesystem::path(path));
}
