// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "clientversion.h"
#include "chainparams.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "leakybucket.h"
#include "main.h"
#include "net.h"
#include "primitives/block.h"
#include "rpcserver.h"
#include "thinblock.h"
#include "tinyformat.h"
#include "txmempool.h"
#include "unlimited.h"
#include "utilstrencodings.h"
#include "util.h"
#include "validationinterface.h"
#include "version.h"
#include "stat.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <boost/thread.hpp>
#include <inttypes.h>

using namespace std;

extern CTxMemPool mempool; // from main.cpp

uint64_t maxGeneratedBlock = DEFAULT_MAX_GENERATED_BLOCK_SIZE;
unsigned int excessiveBlockSize = DEFAULT_EXCESSIVE_BLOCK_SIZE;
unsigned int excessiveAcceptDepth = DEFAULT_EXCESSIVE_ACCEPT_DEPTH;
unsigned int maxMessageSizeMultiplier = DEFAULT_MAX_MESSAGE_SIZE_MULTIPLIER;

std::vector<std::string> BUComments = std::vector<std::string>();
std::string minerComment;

// Variables for traffic shaping
CLeakyBucket receiveShaper(DEFAULT_MAX_RECV_BURST, DEFAULT_AVE_RECV);
CLeakyBucket sendShaper(DEFAULT_MAX_SEND_BURST, DEFAULT_AVE_SEND);
boost::chrono::steady_clock CLeakyBucket::clock;

// Variables for statistics tracking, must be before the "requester" singleton instantiation
const char* sampleNames[] = { "sec10", "min5", "hourly", "daily","monthly"};
int operateSampleCount[] = { 30,       12,   24,  30 };
int interruptIntervals[] = { 30,       30*12,   30*12*24,   30*12*24*30 };

boost::posix_time::milliseconds statMinInterval(10000);
boost::asio::io_service stat_io_service __attribute__((init_priority(101)));

CStatMap statistics __attribute__((init_priority(102)));

CStatHistory<unsigned int, MinValMax<unsigned int> > txAdded; //"memPool/txAdded");
CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize; // "memPool/size",STAT_OP_AVE);


void UnlimitedPushTxns(CNode* dest);

// BUIP010 Xtreme Thinblocks Variables
std::map<uint256, uint64_t> mapThinBlockTimer;

//! The largest block size that we have seen since startup
uint64_t nLargestBlockSeen=BLOCKSTREAM_CORE_MAX_BLOCK_SIZE; // BU - Xtreme Thinblocks

std::string UnlimitedCmdLineHelp()
{
    std::string strUsage;
    strUsage += HelpMessageGroup(_("Bitcoin Unlimited Options:"));
    strUsage += HelpMessageOpt("-excessiveblocksize=<n>", _("Blocks above this size in bytes are considered excessive"));
    strUsage += HelpMessageOpt("-excessiveacceptdepth=<n>", _("Excessive blocks are accepted anyway if this many blocks are mined on top of them"));
    strUsage += HelpMessageOpt("-receiveburst", _("The maximum rate that data can be received in kB/s.  If there has been a period of lower than average data rates, the client may receive extra data to bring the average back to '-receiveavg' but the data rate will not exceed this parameter."));
    strUsage += HelpMessageOpt("-sendburst", _("The maximum rate that data can be sent in kB/s.  If there has been a period of lower than average data rates, the client may send extra data to bring the average back to '-receiveavg' but the data rate will not exceed this parameter."));
    strUsage += HelpMessageOpt("-receiveavg", _("The average rate that data can be received in kB/s"));
    strUsage += HelpMessageOpt("-sendavg", _("The maximum rate that data can be sent in kB/s"));
    strUsage += HelpMessageOpt("-use-thinblocks=<n>", strprintf(_("Turn Thinblocks on or off (off: 0, on: 1, default: %d)"), 1));
    strUsage += HelpMessageOpt("-connect-thinblock=<ip:port>", _("Connect to a thinblock node(s). Blocks will only be downloaded from a thinblock peer.  If no connections are possible then regular blocks will then be downloaded form any other connected peers."));
    strUsage += HelpMessageOpt("-minlimitertxfee=<amt>", strprintf(_("Fees (in satoshi/byte) smaller than this are considered zero fee and subject to -limitfreerelay (default: %s)"), DEFAULT_MINLIMITERTXFEE));
    strUsage += HelpMessageOpt("-maxlimitertxfee=<amt>", strprintf(_("Fees (in satoshi/byte) larger than this are always relayed (default: %s)"), DEFAULT_MAXLIMITERTXFEE));
    return strUsage;
}

std::string FormatCoinbaseMessage(const std::vector<std::string>& comments,const std::string& customComment)
{
    std::ostringstream ss;
    if (!comments.empty())
    {
        std::vector<std::string>::const_iterator it(comments.begin());
        ss << "/" << *it;
        for(++it; it != comments.end(); ++it)
            ss << "/" << *it;
        ss << "/";
    }
    std::string ret = ss.str() + minerComment;
    return ret;
}

UniValue pushtx(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "pushtx \"node\"\n"
            "\nPush uncommitted transactions to a node.\n"
            "\nArguments:\n"
            "1. \"node\"     (string, required) The node (see getpeerinfo for nodes)\n"
            "\nExamples:\n" +
            HelpExampleCli("pushtx", "\"192.168.0.6:8333\" ") + HelpExampleRpc("pushtx", "\"192.168.0.6:8333\", "));

    string strNode = params[0].get_str();

    CNode* node = FindNode(strNode);
    if (!node) {
#if 0
    if (strCommand == "onetry") {
        CAddress addr;
        OpenNetworkConnection(addr, NULL, strNode.c_str());
        return NullUniValue;
    }
#endif
        throw runtime_error("Unknown node");
    }
    UnlimitedPushTxns(node);
    return NullUniValue;
}

void UnlimitedPushTxns(CNode* dest)
{
    //LOCK2(cs_main, pfrom->cs_filter);
    LOCK(dest->cs_filter);
    std::vector<uint256> vtxid;
    mempool.queryHashes(vtxid);
    vector<CInv> vInv;
    BOOST_FOREACH (uint256& hash, vtxid) {
        CInv inv(MSG_TX, hash);
        CTransaction tx;
        bool fInMemPool = mempool.lookup(hash, tx);
        if (!fInMemPool)
            continue; // another thread removed since queryHashes, maybe...
        if ((dest->pfilter && dest->pfilter->IsRelevantAndUpdate(tx)) ||
            (!dest->pfilter))
            vInv.push_back(inv);
        if (vInv.size() == MAX_INV_SZ) {
            dest->PushMessage("inv", vInv);
            vInv.clear();
        }
    }
    if (vInv.size() > 0)
        dest->PushMessage("inv", vInv);
}

void settingsToUserAgentString()
{
    BUComments.clear();

    double ebInMegaBytes = (double)excessiveBlockSize/1000000;
    std::stringstream ebss;
    ebss <<std::fixed << std::setprecision(1) << ebInMegaBytes;
    std::string eb =  ebss.str();
    std::string eb_formatted;
    eb_formatted = (eb.at(eb.size() - 1) == '0' ? eb.substr(0, eb.size() - 2) : eb); //strip zero decimal
    BUComments.push_back("EB" + eb_formatted);

    int ad_formatted;
    ad_formatted = (excessiveAcceptDepth >= 9999999 ? 9999999 : excessiveAcceptDepth);
    BUComments.push_back("AD" + boost::lexical_cast<std::string>(ad_formatted));
}

void UnlimitedSetup(void)
{
    maxGeneratedBlock = GetArg("-blockmaxsize", maxGeneratedBlock);
    excessiveBlockSize = GetArg("-excessiveblocksize", excessiveBlockSize);
    excessiveAcceptDepth = GetArg("-excessiveacceptdepth", excessiveAcceptDepth);
    settingsToUserAgentString();
    //  Init network shapers
    int64_t rb = GetArg("-receiveburst", DEFAULT_MAX_RECV_BURST);
    // parameter is in KBytes/sec, leaky bucket is in bytes/sec.  But if it is "off" then don't multiply
    if (rb != LONG_LONG_MAX)
        rb *= 1024;
    int64_t ra = GetArg("-receiveavg", DEFAULT_MAX_RECV_BURST);
    if (ra != LONG_LONG_MAX)
        ra *= 1024;
    int64_t sb = GetArg("-sendburst", DEFAULT_MAX_RECV_BURST);
    if (sb != LONG_LONG_MAX)
        sb *= 1024;
    int64_t sa = GetArg("-sendavg", DEFAULT_MAX_RECV_BURST);
    if (sa != LONG_LONG_MAX)
        sa *= 1024;

    receiveShaper.set(rb, ra);
    sendShaper.set(sb, sa);

    txAdded.init("memPool/txAdded");
    poolSize.init("memPool/size",STAT_OP_AVE | STAT_KEEP);
}


FILE* blockReceiptLog = NULL;

extern void UnlimitedLogBlock(const CBlock& block, const std::string& hash, uint64_t receiptTime)
{
#if 0  // remove block logging for official release
    if (!blockReceiptLog)
        blockReceiptLog = fopen("blockReceiptLog.txt", "a");
    if (blockReceiptLog) {
        long int byteLen = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
        CBlockHeader bh = block.GetBlockHeader();
        fprintf(blockReceiptLog, "%" PRIu64 ",%" PRIu64 ",%ld,%ld,%s\n", receiptTime, (uint64_t)bh.nTime, byteLen, block.vtx.size(), hash.c_str());
        fflush(blockReceiptLog);
    }
#endif    
}


std::string LicenseInfo()
{
    return FormatParagraph(strprintf(_("Copyright (C) 2009-%i The Bitcoin Unlimited Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin Core Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin XT Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           "\n" +
           FormatParagraph(_("This is experimental software.")) + "\n" +
           "\n" +
           FormatParagraph(_("Distributed under the MIT software license, see the accompanying file COPYING or <http://www.opensource.org/licenses/mit-license.php>.")) + "\n" +
           "\n" +
           FormatParagraph(_("This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit <https://www.openssl.org/> and cryptographic software written by Eric Young and UPnP software written by Thomas Bernard.")) +
           "\n";
}


int chainContainsExcessive(const CBlockIndex* blk, unsigned int goBack)
{
    if (goBack == 0)
        goBack = excessiveAcceptDepth+EXCESSIVE_BLOCK_CHAIN_RESET;
    for (unsigned int i = 0; i < goBack; i++, blk = blk->pprev) 
    {
        if (!blk)
	  break; // we hit the beginning
        if (blk->nStatus & BLOCK_EXCESSIVE)
	  return true;
    }
    return false;
}

int isChainExcessive(const CBlockIndex* blk, unsigned int goBack)
{
    if (goBack == 0)
        goBack = excessiveAcceptDepth;
    bool recentExcessive = false;
    bool oldExcessive = false;
    for (unsigned int i = 0; i < goBack; i++, blk = blk->pprev) {
        if (!blk)
	  break; // we hit the beginning
        if (blk->nStatus & BLOCK_EXCESSIVE)
	  recentExcessive = true;
    }
 
    // Once an excessive block is built upon the chain is not excessive even if more large blocks appear.
    // So look back to make sure that this is the "first" excessive block for a while
    for (unsigned int i = 0; i < EXCESSIVE_BLOCK_CHAIN_RESET; i++, blk = blk->pprev) {
        if (!blk)
	  break; // we hit the beginning
        if (blk->nStatus & BLOCK_EXCESSIVE)
	  oldExcessive = true;
    }

    return (recentExcessive && !oldExcessive);
}

bool CheckExcessive(const CBlock& block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx)
{
    if (blockSize > excessiveBlockSize) {
        LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d  :too many bytes\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps);
        return true;
    }

    LogPrintf("Acceptable block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps);
    return false;
}

extern UniValue getminercomment(const UniValue& params, bool fHelp)
{
  if (fHelp || params.size() != 0)
        throw runtime_error(
            "getminercomment\n"
            "\nReturn the comment that will be put into each mined block's coinbase\n transaction after the Bitcoin Unlimited parameters."
            "\nResult\n"
            "  minerComment (string) miner comment\n"
            "\nExamples:\n" +
            HelpExampleCli("getminercomment", "") + HelpExampleRpc("getminercomment", ""));
  
  return minerComment;
}

extern UniValue setminercomment(const UniValue& params, bool fHelp)
{
  if (fHelp || params.size() != 1)
        throw runtime_error(
            "setminercomment\n"
            "\nSet the comment that will be put into each mined block's coinbase\n transaction after the Bitcoin Unlimited parameters.\n Comments that are too long will be truncated."
            "\nExamples:\n" +
            HelpExampleCli("setminercomment", "\"bitcoin is fundamentally emergent consensus\"") + HelpExampleRpc("setminercomment", "\"bitcoin is fundamentally emergent consensus\""));

  minerComment = params[0].getValStr();
  return NullUniValue;
}

UniValue getexcessiveblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getexcessiveblock\n"
            "\nReturn the excessive block size and accept depth."
            "\nResult\n"
            "  excessiveBlockSize (integer) block size in bytes\n"
            "  excessiveAcceptDepth (integer) if the chain gets this much deeper than the excessive block, then accept the chain as active (if it has the most work)\n"
            "\nExamples:\n" +
            HelpExampleCli("getexcessiveblock", "") + HelpExampleRpc("getexcessiveblock", ""));

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("excessiveBlockSize", (uint64_t)excessiveBlockSize));
    ret.push_back(Pair("excessiveAcceptDepth", (uint64_t)excessiveAcceptDepth));
    return ret;
}

UniValue setexcessiveblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() >= 3)
        throw runtime_error(
            "setexcessiveblock blockSize acceptDepth\n"
            "\nSet the excessive block size and accept depth.  Excessive blocks will not be used in the active chain or relayed until they are several blocks deep in the blockchain.  This discourages the propagation of blocks that you consider excessively large.  However, if the mining majority of the network builds upon the block then you will eventually accept it, maintaining consensus."
            "\nResult\n"
            "  blockSize (integer) excessive block size in bytes\n"
            "  acceptDepth (integer) if the chain gets this much deeper than the excessive block, then accept the chain as active (if it has the most work)\n"
            "\nExamples:\n" +
            HelpExampleCli("getexcessiveblock", "") + HelpExampleRpc("getexcessiveblock", ""));

    if (params[0].isNum())
        excessiveBlockSize = params[0].get_int64();
    else {
        string temp = params[0].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        excessiveBlockSize = boost::lexical_cast<unsigned int>(temp);
    }

    if (params[1].isNum())
        excessiveAcceptDepth = params[1].get_int64();
    else {
        string temp = params[1].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        excessiveAcceptDepth = boost::lexical_cast<unsigned int>(temp);
    }

    settingsToUserAgentString();
    return NullUniValue;
}


UniValue getminingmaxblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getminingmaxblock\n"
            "\nReturn the max generated (mined) block size"
            "\nResult\n"
            "      (integer) maximum generated block size in bytes\n"
            "\nExamples:\n" +
            HelpExampleCli("getminingmaxblock", "") + HelpExampleRpc("getminingmaxblock", ""));

    return maxGeneratedBlock;
}


UniValue setminingmaxblock(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setminingmaxblock blocksize\n"
            "\nSet the maximum number of bytes to include in a generated (mined) block.  This command does not turn generation on/off.\n"
            "\nArguments:\n"
            "1. blocksize         (integer, required) the maximum number of bytes to include in a block.\n"
            "\nExamples:\n"
            "\nSet the generated block size limit to 8 MB\n" +
            HelpExampleCli("setminingmaxblock", "8000000") +
            "\nCheck the setting\n" + HelpExampleCli("getminingmaxblock", ""));

    uint64_t arg = 0;
    if (params[0].isNum())
        arg = params[0].get_int64();
    else {
        string temp = params[0].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        arg = boost::lexical_cast<uint64_t>(temp);
    }

    // I don't want to waste time testing edge conditions where no txns can fit in a block, so limit the minimum block size
    if (arg < 100000)
        throw runtime_error("max generated block size must be greater than 100KB");

    maxGeneratedBlock = arg;

    return NullUniValue;
}


UniValue gettrafficshaping(const UniValue& params, bool fHelp)
{
    string strCommand;
    if (params.size() == 1) {
        strCommand = params[0].get_str();
    }

    if (fHelp || (params.size() != 0))
        throw runtime_error(
            "gettrafficshaping"
            "\nReturns the current settings for the network send and receive bandwidth and burst in kilobytes per second.\n"
            "\nArguments: None\n"
            "\nResult:\n"
            "  {\n"
            "    \"sendBurst\" : 40,   (string) The maximum send bandwidth in Kbytes/sec\n"
            "    \"sendAve\" : 30,   (string) The average send bandwidth in Kbytes/sec\n"
            "    \"recvBurst\" : 20,   (string) The maximum receive bandwidth in Kbytes/sec\n"
            "    \"recvAve\" : 10,   (string) The average receive bandwidth in Kbytes/sec\n"
            "  }\n"
            "\n NOTE: if the send and/or recv parameters do not exist, shaping in that direction is disabled.\n"
            "\nExamples:\n" +
            HelpExampleCli("gettrafficshaping", "") + HelpExampleRpc("gettrafficshaping", ""));

    UniValue ret(UniValue::VOBJ);
    int64_t max, ave;
    sendShaper.get(&max, &ave);
    if (ave != LONG_MAX) {
        ret.push_back(Pair("sendBurst", max / 1024));
        ret.push_back(Pair("sendAve", ave / 1024));
    }
    receiveShaper.get(&max, &ave);
    if (ave != LONG_MAX) {
        ret.push_back(Pair("recvBurst", max / 1024));
        ret.push_back(Pair("recvAve", ave / 1024));
    }
    return ret;
}

UniValue settrafficshaping(const UniValue& params, bool fHelp)
{
    bool disable = false;
    bool badArg = false;
    string strCommand;
    CLeakyBucket* bucket = NULL;
    if (params.size() >= 2) {
        strCommand = params[0].get_str();
        if (strCommand == "send")
            bucket = &sendShaper;
        if (strCommand == "receive")
            bucket = &receiveShaper;
        if (strCommand == "recv")
            bucket = &receiveShaper;
    }
    if (params.size() == 2) {
        if (params[1].get_str() == "disable")
            disable = true;
        else
            badArg = true;
    } else if (params.size() != 3)
        badArg = true;

    if (fHelp || badArg || bucket == NULL)
        throw runtime_error(
            "settrafficshaping \"send|receive\" \"burstKB\" \"averageKB\""
            "\nSets the network send or receive bandwidth and burst in kilobytes per second.\n"
            "\nArguments:\n"
            "1. \"send|receive\"     (string, required) Are you setting the transmit or receive bandwidth\n"
            "2. \"burst\"  (integer, required) Specify the maximum burst size in Kbytes/sec (actual max will be 1 packet larger than this number)\n"
            "2. \"average\"  (integer, required) Specify the average throughput in Kbytes/sec\n"
            "\nExamples:\n" +
            HelpExampleCli("settrafficshaping", "\"receive\" 10000 1024") + HelpExampleCli("settrafficshaping", "\"receive\" disable") + HelpExampleRpc("settrafficshaping", "\"receive\" 10000 1024"));

    if (disable) {
        if (bucket)
            bucket->disable();
    } else {
        uint64_t burst;
        uint64_t ave;
        if (params[1].isNum())
            burst = params[1].get_int64();
        else {
            string temp = params[1].get_str();
            burst = boost::lexical_cast<uint64_t>(temp);
        }
        if (params[2].isNum())
            ave = params[2].get_int64();
        else {
            string temp = params[2].get_str();
            ave = boost::lexical_cast<uint64_t>(temp);
        }
        if (burst < ave) {
            throw runtime_error("Burst rate must be greater than the average rate"
                                "\nsettrafficshaping \"send|receive\" \"burst\" \"average\"");
        }

        bucket->set(burst * 1024, ave * 1024);
    }

    return NullUniValue;
}


/**
 *  BUIP010 Xtreme Thinblocks Section 
 */
bool HaveConnectThinblockNodes()
{
    // Strip the port from then list of all the current in and outbound ip addresses
    std::vector<std::string> vNodesIP;
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH (CNode* pnode, vNodes) {
           int pos = pnode->addrName.find(":");
           if (pos <= 0 )
               vNodesIP.push_back(pnode->addrName);
           else
               vNodesIP.push_back(pnode->addrName.substr(0, pos));
        }
    }

    // Create a set used to check for cross connected nodes.
    // A cross connected node is one where we have a connect-thinblock connection to
    // but we also have another inbound connection which is also using
    // connect-thinblock. In those cases we have created a dead-lock where no blocks 
    // can be downloaded unless we also have at least one additional connect-thinblock 
    // connection to a different node.
    std::set<std::string> nNotCrossConnected;
 
    int nConnectionsOpen = 0;
    BOOST_FOREACH(const std::string& strAddrNode, mapMultiArgs["-connect-thinblock"]) {
        std::string strThinblockNode;
        int pos = strAddrNode.find(":");
        if (pos <= 0 )
            strThinblockNode = strAddrNode;
        else
            strThinblockNode = strAddrNode.substr(0, pos);
        BOOST_FOREACH(std::string strAddr, vNodesIP) {
            if (strAddr == strThinblockNode) {
                nConnectionsOpen++;
                if (!nNotCrossConnected.count(strAddr))
                    nNotCrossConnected.insert(strAddr);
                else
                    nNotCrossConnected.erase(strAddr);
            }
        }
    }
    if (nNotCrossConnected.size() > 0)
        return true;
    else if (nConnectionsOpen > 0)
        LogPrint("thin", "You have a cross connected thinblock node - we may download regular blocks until you resolve the issue\n");
    return false; // Connections are either not open or they are cross connected.
} 


bool HaveThinblockNodes()
{
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH (CNode* pnode, vNodes)
            if (pnode->ThinBlockCapable())
                return true;
    }
    return false;
}

bool CheckThinblockTimer(uint256 hash)
{
    if (!mapThinBlockTimer.count(hash)) {
        mapThinBlockTimer[hash] = GetTimeMillis();
        LogPrint("thin", "Starting Preferential Thinblock timer\n");
    }
    else {
        // Check that we have not exceeded the 10 second limit.
        // If we have then we want to return false so that we can
        // proceed to download a regular block instead.
        uint64_t elapsed = GetTimeMillis() - mapThinBlockTimer[hash];
        if (elapsed > 10000) {
            LogPrint("thin", "Preferential Thinblock timer exceeded - downloading regular block instead\n");
            return false;
        }
    }
    return true;
}

void ClearThinBlockTimer(uint256 hash)
{
    if (mapThinBlockTimer.count(hash)) {
        mapThinBlockTimer.erase(hash);
        LogPrint("thin", "Clearing Preferential Thinblock timer\n");
    }
}

bool IsThinBlocksEnabled() 
{
    bool fThinblocksEnabled = GetBoolArg("-use-thinblocks", true);

    // Enabling the XTHIN service should really be in init.cpp but because
    // we want to avoid possile future merge conflicts with Core we can enable
    // it here as it has little performance impact.
    if (fThinblocksEnabled)
        nLocalServices |= NODE_XTHIN;
    return fThinblocksEnabled;
}

bool IsChainNearlySyncd() 
{
    LOCK(cs_main);
    if(chainActive.Height() < pindexBestHeader->nHeight - 2)
        return false;
    return true;
}

void BuildSeededBloomFilter(CBloomFilter& filterMemPool, std::vector<uint256>& vOrphanHashes)
{
    LogPrint("thin", "Starting creation of bloom filter\n");
    seed_insecure_rand();
    double nBloomPoolSize = (double)mempool.mapTx.size();
    if (nBloomPoolSize > MAX_BLOOM_FILTER_SIZE / 1.8)
        nBloomPoolSize = MAX_BLOOM_FILTER_SIZE / 1.8;
    double nBloomDecay = 1.5 - (nBloomPoolSize * 1.8 / MAX_BLOOM_FILTER_SIZE);  // We should never go below 0.5 as we will start seeing re-requests for tx's
    int nElements = std::max((int)(((int)mempool.mapTx.size() + (int)vOrphanHashes.size()) * nBloomDecay), 1); // Must make sure nElements is greater than zero or will assert
    double nFPRate = .001 + (((double)nElements * 1.8 / MAX_BLOOM_FILTER_SIZE) * .004); // The false positive rate in percent decays as the mempool grows
    filterMemPool = CBloomFilter(nElements, nFPRate, insecure_rand(), BLOOM_UPDATE_ALL);
    LogPrint("thin", "Bloom multiplier: %f FPrate: %f Num elements in bloom filter: %d num mempool entries: %d\n", nBloomDecay, nFPRate, nElements, (int)mempool.mapTx.size());

    // Seed the filter with the transactions in the memory pool
    LOCK(cs_main);
    std::vector<uint256> vMemPoolHashes;
    mempool.queryHashes(vMemPoolHashes);
    for (uint64_t i = 0; i < vMemPoolHashes.size(); i++)
         filterMemPool.insert(vMemPoolHashes[i]);
    for (uint64_t i = 0; i < vOrphanHashes.size(); i++)
         filterMemPool.insert(vOrphanHashes[i]);
    LogPrint("thin", "Created bloom filter: %d bytes\n",::GetSerializeSize(filterMemPool, SER_NETWORK, PROTOCOL_VERSION));
}

void LoadFilter(CNode *pfrom, CBloomFilter *filter)
{
    if (!filter->IsWithinSizeConstraints())
        // There is no excuse for sending a too-large filter
        Misbehaving(pfrom->GetId(), 100);
    else
    {
        LOCK(pfrom->cs_filter);
        delete pfrom->pThinBlockFilter;
        pfrom->pThinBlockFilter = new CBloomFilter(*filter);
        pfrom->pThinBlockFilter->UpdateEmptyFull();
    }
    uint64_t nSizeFilter = ::GetSerializeSize(*pfrom->pThinBlockFilter, SER_NETWORK, PROTOCOL_VERSION);
    LogPrint("thin", "Thinblock Bloom filter size: %d\n", nSizeFilter);
}

void HandleBlockMessage(CNode *pfrom, const string &strCommand, CBlock &block, const CInv &inv)
{
    int64_t startTime = GetTimeMicros();
    CValidationState state;
    uint64_t nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);

    // Process all blocks from whitelisted peers, even if not requested,
    // unless we're still syncing with the network.
    // Such an unrequested block may still be processed, subject to the
    // conditions in AcceptBlock().
    bool forceProcessing = pfrom->fWhitelisted && !IsInitialBlockDownload();
    const CChainParams& chainparams = Params();
    ProcessNewBlock(state, chainparams, pfrom, &block, forceProcessing, NULL);
    int nDoS;
    if (state.IsInvalid(nDoS)) {
        LogPrintf("Invalid block due to %s\n", state.GetRejectReason().c_str());
        pfrom->PushMessage("reject", strCommand, state.GetRejectCode(),
                           state.GetRejectReason().substr(0, MAX_REJECT_MESSAGE_LENGTH), inv.hash);
        if (nDoS > 0) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), nDoS);
        }
    }
    else 
        nLargestBlockSeen = std::max(nSizeBlock, nLargestBlockSeen);

    LogPrint("thin", "Processed Block %s in %.2f seconds\n", inv.hash.ToString(), (double)(GetTimeMicros() - startTime) / 1000000.0);
    
    // When we request a thinblock we may get back a regular block if it is smaller than a thinblock
    // Therefore we have to remove the thinblock in flight if it exists and we also need to check that 
    // the block didn't arrive from some other peer.  This code ALSO cleans up the thin block that
    // was passed to us (&block), so do not use it after this.
    {
        int nTotalThinBlocksInFlight = 0;
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes) {
            if (pnode->mapThinBlocksInFlight.count(inv.hash)) {
                pnode->mapThinBlocksInFlight.erase(inv.hash); 
                pnode->thinBlockWaitingForTxns = -1;
                pnode->thinBlock.SetNull();
            }
            if (pnode->mapThinBlocksInFlight.size() > 0)
                nTotalThinBlocksInFlight++;
        }

        // When we no longer have any thinblocks in flight then clear the set
        // just to make sure we don't somehow get growth over time.
        if (nTotalThinBlocksInFlight == 0) {
            setPreVerifiedTxHash.clear();
            setUnVerifiedOrphanTxHash.clear();
        }
    }

    // Clear the thinblock timer used for preferential download
    ClearThinBlockTimer(inv.hash);
}

bool ThinBlockMessageHandler(vector<CNode*>& vNodesCopy)
{
    bool sleep = true;
    CNodeSignals& signals = GetNodeSignals();
    BOOST_FOREACH (CNode* pnode, vNodesCopy)
    {
        if ((pnode->fDisconnect) || (!pnode->ThinBlockCapable()))
            continue;

        // Receive messages
        {
            TRY_LOCK(pnode->cs_vRecvMsg, lockRecv);
            if (lockRecv)
            {
                if (!signals.ProcessMessages(pnode))
                    pnode->CloseSocketDisconnect();

                if (pnode->nSendSize < SendBufferSize())
                {
                    if (!pnode->vRecvGetData.empty() || (!pnode->vRecvMsg.empty() && pnode->vRecvMsg[0].complete()))
                        sleep = false;
                }
            }
        }
        boost::this_thread::interruption_point();
        signals.SendMessages(pnode);
        boost::this_thread::interruption_point();
    }
    return sleep;
}

void ConnectToThinBlockNodes()
{
    // Connect to specific addresses
    if (mapArgs.count("-connect-thinblock") && mapMultiArgs["-connect-thinblock"].size() > 0)
    {
        BOOST_FOREACH(const std::string& strAddr, mapMultiArgs["-connect-thinblock"])
        {
            CAddress addr;
            OpenNetworkConnection(addr, NULL, strAddr.c_str());
            MilliSleep(500);
        }
    }
}

void CheckNodeSupportForThinBlocks()
{
    if(IsThinBlocksEnabled()) {
        // Check that a nodes pointed to with connect-thinblock actually supports thinblocks
        BOOST_FOREACH(string& strAddr, mapMultiArgs["-connect-thinblock"]) {
            if(CNode* pnode = FindNode(strAddr)) {
                if(!pnode->ThinBlockCapable()) {
                    LogPrintf("ERROR: You are trying to use connect-thinblocks but to a node that does not support it - Protocol Version: %d peer=%d\n", 
                               pnode->nVersion, pnode->id);
                }
            }
        }
    }
}

void SendXThinBlock(CBlock &block, CNode* pfrom, const CInv &inv)
{
    if (inv.type == MSG_XTHINBLOCK)
    {
        CXThinBlock xThinBlock(block, pfrom->pThinBlockFilter);
        int nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
        if (xThinBlock.collision == true) // If there is a cheapHash collision in this block then send a normal thinblock
        {
            CThinBlock thinBlock(block, *pfrom->pThinBlockFilter);
            int nSizeThinBlock = ::GetSerializeSize(xThinBlock, SER_NETWORK, PROTOCOL_VERSION);
            if (nSizeThinBlock < nSizeBlock) {
                pfrom->PushMessage(NetMsgType::THINBLOCK, thinBlock);
                LogPrint("thin", "TX HASH COLLISION: Sent thinblock - size: %d vs block size: %d => tx hashes: %d transactions: %d  peerid=%d\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->id);
            }
            else {
                pfrom->PushMessage(NetMsgType::BLOCK, block);
                LogPrint("thin", "Sent regular block instead - xthinblock size: %d vs block size: %d => tx hashes: %d transactions: %d  peerid=%d\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->id);
            }
        }
        else // Send an xThinblock
        {
            // Only send a thinblock if smaller than a regular block
            int nSizeThinBlock = ::GetSerializeSize(xThinBlock, SER_NETWORK, PROTOCOL_VERSION);
            if (nSizeThinBlock < nSizeBlock) {
                pfrom->PushMessage(NetMsgType::XTHINBLOCK, xThinBlock);
                LogPrint("thin", "Sent xthinblock - size: %d vs block size: %d => tx hashes: %d transactions: %d  peerid=%d\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->id);
            }
            else {
                pfrom->PushMessage(NetMsgType::BLOCK, block);
                LogPrint("thin", "Sent regular block instead - xthinblock size: %d vs block size: %d => tx hashes: %d transactions: %d  peerid=%d\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->id);
            }
        }
    }
    else if (inv.type == MSG_THINBLOCK)
    {
        CThinBlock thinBlock(block, *pfrom->pThinBlockFilter);
        int nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
        int nSizeThinBlock = ::GetSerializeSize(thinBlock, SER_NETWORK, PROTOCOL_VERSION);
        if (nSizeThinBlock < nSizeBlock) { // Only send a thinblock if smaller than a regular block
            pfrom->PushMessage(NetMsgType::THINBLOCK, thinBlock);
            LogPrint("thin", "Sent thinblock - size: %d vs block size: %d => tx hashes: %d transactions: %d  peerid=%d\n", nSizeThinBlock, nSizeBlock, thinBlock.vTxHashes.size(), thinBlock.vMissingTx.size(), pfrom->id);
        }
        else {
            pfrom->PushMessage(NetMsgType::BLOCK, block);
            LogPrint("thin", "Sent regular block instead - thinblock size: %d vs block size: %d => tx hashes: %d transactions: %d  peerid=%d\n", nSizeThinBlock, nSizeBlock, thinBlock.vTxHashes.size(), thinBlock.vMissingTx.size(), pfrom->id);
        }
    }
}

// Similar to TestBlockValidity but is very conservative in parameters (used in mining)
bool TestConservativeBlockValidity(CValidationState& state, const CChainParams& chainparams, const CBlock& block, CBlockIndex* pindexPrev, bool fCheckPOW, bool fCheckMerkleRoot)
{
    AssertLockHeld(cs_main);
    assert(pindexPrev && pindexPrev == chainActive.Tip());
    if (fCheckpointsEnabled && !CheckIndexAgainstCheckpoint(pindexPrev, state, chainparams, block.GetHash()))
        return error("%s: CheckIndexAgainstCheckpoint(): %s", __func__, state.GetRejectReason().c_str());

    CCoinsViewCache viewNew(pcoinsTip);
    CBlockIndex indexDummy(block);
    indexDummy.pprev = pindexPrev;
    indexDummy.nHeight = pindexPrev->nHeight + 1;

    // NOTE: CheckBlockHeader is called by CheckBlock
    if (!ContextualCheckBlockHeader(block, state, pindexPrev))
        return false;
    if (!CheckBlock(block, state, fCheckPOW, fCheckMerkleRoot, true))
        return false;
    if (!ContextualCheckBlock(block, state, pindexPrev))
        return false;
    if (!ConnectBlock(block, state, &indexDummy, viewNew, true))
        return false;
    assert(state.IsValid());

    return true;
}

// Statistics:

CStatBase* FindStatistic(const char* name)
{
  CStatMap::iterator item = statistics.find(name);
  if (item != statistics.end())
    return item->second;
  return NULL;
}

UniValue getstatlist(const UniValue& params, bool fHelp)
{
  if (fHelp || (params.size() != 0))
        throw runtime_error(
            "getstatlist"
            "\nReturns a list of all statistics available on this node.\n"
            "\nArguments: None\n"
            "\nResult:\n"
            "  {\n"
            "    \"name\" : (string) name of the statistic\n"
            "    ...\n"
            "  }\n"
            "\nExamples:\n" +
            HelpExampleCli("getstatlist", "") + HelpExampleRpc("getstatlist", ""));

  CStatMap::iterator it;

  UniValue ret(UniValue::VARR);
  for (it = statistics.begin(); it != statistics.end(); ++it)
    {
    ret.push_back(it->first);
    }

  return ret;
}

UniValue getstat(const UniValue& params, bool fHelp)
{
    string specificIssue;

    int count = 0;
    if (params.size() < 3) count = 1;  // if a count is not specified, give the latest sample
    else
      {
	if (!params[2].isNum()) 
	  {
          try
	    {
	      count =  boost::lexical_cast<int>(params[2].get_str());
	    }
          catch (const boost::bad_lexical_cast &)
	    {
            fHelp=true;
            specificIssue = "Invalid argument 3 \"count\" -- not a number";
  	    }
	  }
        else
	  {
	    count = params[2].get_int();
	  }
      }
    if (fHelp || (params.size() < 1))
        throw runtime_error(
            "getstat"
            "\nReturns the current settings for the network send and receive bandwidth and burst in kilobytes per second.\n"
            "\nArguments: \n"
            "1. \"statistic\"     (string, required) Specify what statistic you want\n"
            "2. \"series\"  (string, optional) Specify what data series you want.  Options are \"now\",\"all\", \"sec10\", \"min5\", \"hourly\", \"daily\",\"monthly\".  Default is all.\n"
            "3. \"count\"  (string, optional) Specify the number of samples you want.\n"

            "\nResult:\n"
            "  {\n"
            "    \"<statistic name>\"\n"
            "    {\n"
            "    \"<series name>\"\n"
            "      [\n"
            "      <data>, (any type) The data points in the series\n"
            "      ],\n"
            "    ...\n"
            "    },\n"
            "  ...\n"            
            "  }\n"
            "\nExamples:\n" +
            HelpExampleCli("getstat", "") + HelpExampleRpc("getstat", "")
            + "\n" + specificIssue);

    UniValue ret(UniValue::VARR);

    string seriesStr;
    if (params.size() < 2)
      seriesStr = "now";
    else seriesStr = params[1].get_str();
    //uint_t series = 0; 
    //if (series == "now") series |= 1;
    //if (series == "all") series = 0xfffffff;

    CStatBase* base = FindStatistic(params[0].get_str().c_str());
    if (base)
      {
        UniValue ustat(UniValue::VOBJ);
        if (seriesStr == "now")
          {
	    ustat.push_back(Pair("now", base->GetNow()));
	  }
        else
	  {
            UniValue series = base->GetSeries(seriesStr,count);
	    ustat.push_back(Pair(seriesStr,series));
	  }

        ret.push_back(ustat);  
      }

    return ret;
}
