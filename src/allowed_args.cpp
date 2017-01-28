// Copyright (c) 2017 Stephen McCarthy
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "allowed_args.h"

#include "tinyformat.h"
#include "util.h"

#include <set>

const std::set<std::string> HELP_ARGS
{
  "?",
  "h",
  "help",
  "version"
};

const std::set<std::string> BITCOIND_ARGS
{
#ifdef ENABLE_WALLET
    "disablewallet",
    "fallbackfee",
    "keypool",
    "maxtxfee",
    "mintxfee",
    "paytxfee",
    "rescan",
    "salvagewallet",
    "sendfreetransactions",
    "spendzeroconfchange",
    "txconfirmtarget",
    "upgradewallet",
    "wallet",
    "walletbroadcast",
    "walletnotify",
    "zapwallettxes",
#endif
#if ENABLE_ZMQ
    "zmqpubhashblock",
    "zmqpubhashtx",
    "zmqpubrawblock",
    "zmqpubrawtx"
#endif
#ifdef USE_UPNP
    "upnp",
#endif
#ifndef WIN32
    "daemon",
    "pid",
#endif
    "acceptnonstdtxn",
    "addnode",
    "admincookiefile",
    "adminserver",
    "alertnotify",
    "banscore",
    "bantime",
    "bind",
    "blockmaxsize",
    "blockminsize",
    "blocknotify",
    "blockprioritysize",
    "blocksizeacceptlimit",
    "blocksonly",
    "blockversion",
    "bytespersigop",
    "checkblockindex",
    "checkblocks",
    "checklevel",
    "checkmempool",
    "checkpoints",
    "conf",
    "connect",
    "datacarrier",
    "datacarriersize",
    "datadir",
    "dbcache",
    "dblogsize",
    "debug",
    "disablesafemode",
    "discover",
    "dns",
    "dnsseed",
    "dropmessagestest",
    "enforcenodebloom",
    "excessiveblocksize",
    "externalip",
    "flextrans",
    "flushwallet",
    "forcednsseed",
    "fuzzmessagestest",
    "gen",
    "gencoinbase",
    "genproclimit",
    "help-debug",
    "limitancestorcount",
    "limitancestorsize",
    "limitdescendantcount",
    "limitdescendantsize",
    "limitfreerelay",
    "listen",
    "listenonion",
    "loadblock",
    "logips",
    "logtimemicros",
    "logtimestamps",
    "maxconnections",
    "maxmempool",
    "maxorphantx",
    "maxreceivebuffer",
    "maxsendbuffer",
    "maxsigcachesize",
    "maxuploadtarget",
    "mempoolexpiry",
    "minrelaytxfee",
    "mocktime",
    "onion",
    "onlynet",
    "par",
    "peerbloomfilters",
    "permitbaremultisig",
    "port",
    "printpriority",
    "printtoconsole",
    "privdb",
    "proxy",
    "proxyrandomize",
    "prune",
    "reindex",
    "relaypriority",
    "regtest",
    "rest",
    "rpcallowip",
    "rpcauth",
    "rpcbind",
    "rpccookiefile",
    "rpcpassword",
    "rpcport",
    "rpcservertimeout",
    "rpcthreads",
    "rpcuser",
    "rpcworkqueue",
    "seednode",
    "server",
    "shrinkdebugfile",
    "stopafterblockimport",
    "testnet",
    "testnet-ft",
    "testsafemode",
    "timeout",
    "torcontrol",
    "torpassword",
    "txindex",
    "uacomment",
    "use-thinblocks",
    "version",
    "whitebind",
    "whitelist",
    "whitelistforcerelay",
    "whitelistrelay"
};

const std::set<std::string> BITCOIN_QT_ARGS
{
    "allowselfsignedrootcertificates",
    "choosedatadir",
    "lang",
    "min",
    "resetguisettings",
    "rootcertificates",
    "splash",
    "uiplatform"
};

const std::set<std::string> BITCOIN_CLI_ARGS
{
    "conf",
    "datadir",
    "rpcclienttimeout",
    "rpcconnect",
    "rpcpassword",
    "rpcport",
    "rpcuser",
    "rpcwait"
};

const std::set<std::string> BITCOIN_TX_ARGS
{
    "create",
    "json",
    "txid"
};

void unrecognizedOption(const std::string& strArg)
{
    throw std::runtime_error(strprintf(_("unrecognized option '%s'"), strArg));
}

void AllowedArgs::BitcoinCli(const std::string& strArg)
{
    if (!HELP_ARGS.count(strArg) &&
        !BITCOIN_CLI_ARGS.count(strArg))
    {
        unrecognizedOption(strArg);
    }
}

void AllowedArgs::Bitcoind(const std::string& strArg)
{
    if (!HELP_ARGS.count(strArg) &&
        !BITCOIND_ARGS.count(strArg))
    {
        unrecognizedOption(strArg);
    }
}

void AllowedArgs::BitcoinQt(const std::string& strArg)
{
    if (!HELP_ARGS.count(strArg) &&
        !BITCOIND_ARGS.count(strArg) &&
        !BITCOIN_QT_ARGS.count(strArg))
    {
        unrecognizedOption(strArg);
    }
}

void AllowedArgs::BitcoinTx(const std::string& strArg)
{
    if (!HELP_ARGS.count(strArg) &&
        !BITCOIN_TX_ARGS.count(strArg))
    {
        unrecognizedOption(strArg);
    }
}

void AllowedArgs::ConfigFile(const std::string& strArg)
{
    if (!HELP_ARGS.count(strArg) &&
        !BITCOIND_ARGS.count(strArg) &&
        !BITCOIN_QT_ARGS.count(strArg) &&
        !BITCOIN_CLI_ARGS.count(strArg))
    {
        unrecognizedOption(strArg);
    }
}
