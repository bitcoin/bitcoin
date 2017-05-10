// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "clientversion.h"
#include "chainparams.h"
#include "miner.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "expedited.h"
#include "hash.h"
#include "leakybucket.h"
#include "main.h"
#include "net.h"
#include "policy/policy.h"
#include "primitives/block.h"
#include "rpcserver.h"
#include "thinblock.h"
#include "timedata.h"
#include "tinyformat.h"
#include "txmempool.h"
#include "unlimited.h"
#include "utilstrencodings.h"
#include "ui_interface.h"
#include "util.h"
#include "validationinterface.h"
#include "version.h"
#include "stat.h"
#include "tweak.h"

// just for size reporting
#include "alert.h"

#include <boost/atomic.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <iomanip>
#include <boost/thread.hpp>
#include <inttypes.h>
#include <queue>

using namespace std;

extern CTxMemPool mempool; // from main.cpp
static boost::atomic<uint64_t> nLargestBlockSeen(BLOCKSTREAM_CORE_MAX_BLOCK_SIZE); // track the largest block we've seen
static boost::atomic<bool> fIsChainNearlySyncd(false);
extern CTweakRef<uint64_t> miningBlockSize;
extern CTweakRef<unsigned int> ebTweak;
extern std::vector<CNode *> xpeditedBlk; // Who requested expedited blocks from us
extern std::vector<CNode *> xpeditedBlkUp; // Who we requested expedited blocks from
extern std::vector<CNode *> xpeditedTxn;

extern CCriticalSection cs_previousblock;  // Protects the vPreviousBlock array
// This vector holds hashes of the transactions in the previous block.  When a new block comes in it is used to
// flush the orphan pool of any transactions that may have trickled in AFTER the block that mines them has already
// been processed.  In this case a transaction may look like an orphan because its parents have already been removed
// from the UTXO set.
vector<uint256> vPreviousBlock;

bool IsTrafficShapingEnabled();

bool MiningAndExcessiveBlockValidatorRule(const unsigned int newExcessiveBlockSize, const unsigned int newMiningBlockSize)
{
    // The mined block size must be less then or equal too the excessive block size.
    return ( newMiningBlockSize <= newExcessiveBlockSize );
}

std::string ExcessiveBlockValidator(const unsigned int& value,unsigned int* item,bool validate)
{
  if (validate)
    {
      if (!MiningAndExcessiveBlockValidatorRule(value, maxGeneratedBlock))
	{
        std::ostringstream ret;
        ret << "Sorry, your maximum mined block (" << maxGeneratedBlock << ") is larger than your proposed excessive size (" << value << ").  This would cause you to orphan your own blocks.";    
        return ret.str();
	}
    }
  else  // Do anything to "take" the new value
    {
      // nothing needed
    }
  return std::string();
}

std::string MiningBlockSizeValidator(const uint64_t& value,uint64_t* item,bool validate)
{
  if (validate)
    {
      if (!MiningAndExcessiveBlockValidatorRule(excessiveBlockSize, value))
	{
        std::ostringstream ret;
        ret << "Sorry, your excessive block size (" << excessiveBlockSize << ") is smaller than your proposed mined block size (" << value << ").  This would cause you to orphan your own blocks.";    
        return ret.str();
	}
    }
  else  // Do anything to "take" the new value
    {
      // nothing needed
    }
  return std::string();
}

std::string OutboundConnectionValidator(const int& value,int* item,bool validate)
{
  if (validate)
    {
      if ((value < 0)||(value > 10000))  // sanity check
	{
        return "Invalid Value";
	}
      if (value < *item)
	{
	  return "This field cannot be reduced at run time since that would kick out existing connections";
	}
    }
  else  // Do anything to "take" the new value
    {
      if (value < *item)  // note that now value is the old value and *item has been set to the new.
        {
	  int diff = *item - value;
          if (semOutboundAddNode)  // Add the additional slots to the outbound semaphore
            for (int i=0; i<diff; i++)
              semOutboundAddNode->post();
	}
    }
  return std::string();
}

std::string SubverValidator(const std::string& value,std::string* item,bool validate)
{
  if (validate)
    {
    if (value.size() > MAX_SUBVERSION_LENGTH) 
    {
      return(std::string("Subversion string is too long")); 
    }
  }
  return std::string();
}


// Push all transactions in the mempool to another node
void UnlimitedPushTxns(CNode* dest);


int32_t UnlimitedComputeBlockVersion(const CBlockIndex* pindexPrev, const Consensus::Params& params,uint32_t nTime)
{
    if (blockVersion != 0)  // BU: allow override of block version
      {
        return blockVersion;
      }
    
    int32_t nVersion = ComputeBlockVersion(pindexPrev, params);

    // turn BIP109 off by default by commenting this out: 
    // if (nTime <= params.SizeForkExpiration())
    //	  nVersion |= FORK_BIT_2MB;
 
    return nVersion;
}


void UpdateSendStats(CNode* pfrom, const char* strCommand, int msgSize, int64_t nTime)
{
  sendAmt += msgSize;
  std::string name("net/send/msg/");
  name.append(strCommand);
  LOCK(cs_statMap);
  CStatMap::iterator obj = statistics.find(name);
  CStatMap::iterator end = statistics.end();
  if (obj != end)
    {
      CStatBase* base = obj->second;
      if (base)
	{
	  CStatHistory<uint64_t>* stat = dynamic_cast<CStatHistory<uint64_t>*> (base);
          if (stat)
            *stat << msgSize;
	}
    }
}

void UpdateRecvStats(CNode* pfrom, const std::string& strCommand, int msgSize, int64_t nTimeReceived)
{
  recvAmt += msgSize;
  std::string name = "net/recv/msg/" + strCommand;
  LOCK(cs_statMap);
  CStatMap::iterator obj = statistics.find(name);
  CStatMap::iterator end = statistics.end();
  if (obj != end)
    {
      CStatBase* base = obj->second;
      if (base)
	{
	  CStatHistory<uint64_t>* stat = dynamic_cast<CStatHistory<uint64_t>*> (base);
          if (stat)
            *stat << msgSize;
	}
    }
}


std::string UnlimitedCmdLineHelp()
{
    std::string strUsage;
    strUsage += HelpMessageGroup(_("Bitcoin Unlimited Options:"));
    strUsage += HelpMessageOpt("-blockversion=<n>", _("Generated block version number.  Value must be an integer"));
    strUsage += HelpMessageOpt("-excessiveblocksize=<n>", _("Blocks above this size in bytes are considered excessive"));
    strUsage += HelpMessageOpt("-excessiveacceptdepth=<n>", _("Excessive blocks are accepted anyway if this many blocks are mined on top of them"));
    strUsage += HelpMessageOpt("-receiveburst", _("The maximum rate that data can be received in kB/s.  If there has been a period of lower than average data rates, the client may receive extra data to bring the average back to '-receiveavg' but the data rate will not exceed this parameter."));
    strUsage += HelpMessageOpt("-sendburst", _("The maximum rate that data can be sent in kB/s.  If there has been a period of lower than average data rates, the client may send extra data to bring the average back to '-receiveavg' but the data rate will not exceed this parameter."));
    strUsage += HelpMessageOpt("-receiveavg", _("The average rate that data can be received in kB/s"));
    strUsage += HelpMessageOpt("-sendavg", _("The maximum rate that data can be sent in kB/s"));
    strUsage += HelpMessageOpt(
        "-use-thinblocks=<n>", strprintf(_("Turn Thinblocks on or off (off: 0, on: 1, default: %d)"), 1));
    strUsage += HelpMessageOpt("-connect-thinblock=<ip:port>",
        _("Connect to a thinblock node(s). Blocks will only be downloaded from a thinblock peer.  If no connections "
          "are possible then regular blocks will then be downloaded form any other connected peers."));
    strUsage +=
        HelpMessageOpt("-minlimitertxfee=<amt>", strprintf(_("Fees (in satoshi/byte) smaller than this are considered "
                                                             "zero fee and subject to -limitfreerelay (default: %s)"),
                                                     DEFAULT_MINLIMITERTXFEE));
    strUsage += HelpMessageOpt(
        "-min-xthin-nodes=<n>", strprintf(_("Minimum number of xthin nodes to automatically find and connect "
                                            "(default: %d)"),
                                    4));
    strUsage += HelpMessageOpt("-maxlimitertxfee=<amt>",
        strprintf(_("Fees (in satoshi/byte) larger than this are always relayed (default: %s)"),
                                   DEFAULT_MAXLIMITERTXFEE));
    strUsage += HelpMessageOpt(
        "-bitnodes", _("Query for peer addresses via Bitnodes API, if low on addresses (default: 1 unless -connect)"));
    strUsage += HelpMessageOpt("-forcebitnodes",
        strprintf(_("Always query for peer addresses via Bitnodes API (default: %u)"), DEFAULT_FORCEBITNODES));
    strUsage += HelpMessageOpt("-usednsseed=<host>", _("Add a custom DNS seed to use.  If at least one custom DNS seed "
                                                       "is set, the default DNS seeds will be ignored."));
    strUsage += HelpMessageOpt(
        "-expeditedblock=<host>", _("Request expedited blocks from this host whenever we are connected to it"));
    strUsage += HelpMessageOpt("-maxexpeditedblockrecipients=<n>",
        _("The maximum number of nodes this node will forward expedited blocks to"));
    strUsage += HelpMessageOpt("-maxexpeditedtxrecipients=<n>",
        _("The maximum number of nodes this node will forward expedited transactions to"));
    strUsage += HelpMessageOpt("-maxoutconnections=<n>",
        strprintf(_("Initiate at most <n> connections to peers (default: %u).  If this number is higher than "
                    "--maxconnections, it will be reduced to --maxconnections."),
                                   DEFAULT_MAX_OUTBOUND_CONNECTIONS));
    strUsage += HelpMessageOpt(
        "-parallel=<n>", strprintf(_("Turn Parallel Block Validation on or off (off: 0, on: 1, default: %d)"), 1));
    strUsage += HelpMessageOpt("-gen", strprintf(_("Generate coins (default: %u)"), DEFAULT_GENERATE));
    strUsage += HelpMessageOpt("-genproclimit=<n>",
        strprintf(_("Set the number of threads for coin generation if enabled (-1 = all cores, default: %d)"),
                                   DEFAULT_GENERATE_THREADS));
    strUsage += HelpMessageOpt("-ophanpoolexpiry=<n>", strprintf(_("Do not keep transactions in the orphanpool longer than <n> hours (default: %u)"),
                                   DEFAULT_ORPHANPOOL_EXPIRY));
    strUsage += TweakCmdLineHelp();
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

CNode* FindLikelyNode(const std::string& addrName)
{
    LOCK(cs_vNodes);
    bool wildcard = (addrName.find_first_of("*?") != std::string::npos);
    
    BOOST_FOREACH (CNode* pnode, vNodes)
      {
        if (wildcard) 
          {
            if (match(addrName.c_str(),pnode->addrName.c_str())) return (pnode);
          }
        else if (pnode->addrName.find(addrName) != std::string::npos)
            return (pnode);
      }
    return NULL;
}

UniValue expedited(const UniValue& params, bool fHelp)
{
    std::string strCommand;
    if (fHelp || params.size() < 2)
        throw runtime_error(
            "expedited block|tx \"node IP addr\" on|off\n"
            "\nRequest expedited forwarding of blocks and/or transactions from a node.\nExpedited forwarding sends blocks or transactions to a node before the node requests them.  This reduces latency, potentially at the expense of bandwidth.\n"
            "\nArguments:\n"
            "1. \"block | tx\"        (string, required) choose block to send expedited blocks, tx to send expedited transactions\n"
            "2. \"node ip addr\"     (string, required) The node's IP address or IP and port (see getpeerinfo for nodes)\n"
            "3. \"on | off\"     (string, required) Turn expedited service on or off\n"
            "\nExamples:\n" +
            HelpExampleCli("expedited", "block \"192.168.0.6:8333\" on") + HelpExampleRpc("expedited", "\"block\", \"192.168.0.6:8333\", \"on\""));

    std::string obj = params[0].get_str();
    std::string strNode = params[1].get_str();

    CNode* node = FindLikelyNode(strNode);
    if (!node) {
        throw runtime_error("Unknown node");
    }

    uint64_t flags=0;
    if (obj.find("block")!= std::string::npos) flags |= EXPEDITED_BLOCKS;
    if (obj.find("blk")!= std::string::npos) flags |= EXPEDITED_BLOCKS;
    if (obj.find("tx")!= std::string::npos) flags |= EXPEDITED_TXNS;
    if (obj.find("transaction")!= std::string::npos) flags |= EXPEDITED_TXNS;
    if ((flags & (EXPEDITED_BLOCKS|EXPEDITED_TXNS))==0)
      {
        throw runtime_error("Unknown object, give 'block' or 'transaction'");
      }

    if (params.size() >= 3)
    {
        std::string onoff = params[2].get_str();
        if (onoff == "off")
            flags |= EXPEDITED_STOP;
        if (onoff == "OFF")
            flags |= EXPEDITED_STOP;
    }

    // TODO: validate that the node can handle expedited blocks

    // Add or remove this node to our list of upstream nodes
    {
        LOCK(cs_xpedited);
        std::vector<CNode *>::iterator elem = std::find(xpeditedBlkUp.begin(), xpeditedBlkUp.end(), node);
        if ((flags & EXPEDITED_BLOCKS) && (flags & EXPEDITED_STOP))
        {
            if (elem != xpeditedBlkUp.end())
                xpeditedBlkUp.erase(elem);
        }
        else if (flags & EXPEDITED_BLOCKS)
        {
            if (elem == xpeditedBlkUp.end()) // don't add it twice
                xpeditedBlkUp.push_back(node);
        }
    }

    // Push the expedited message even if its a repeat to allow the operator to reissue the CLI command to trigger another message.
    node->PushMessage(NetMsgType::XPEDITEDREQUEST,flags);
    return NullUniValue;
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

    //BU: Add lock on cs_vNodes as FindNode now requries it to prevent potential use-after-free errors
    CNode* node = NULL;
    {
        LOCK(cs_vNodes);
        node = FindLikelyNode(strNode);

        if (!node) {
            throw runtime_error("Unknown node");
        }

        //BU: Since we are passing node to another function, add a ref to prevent use-after-free
        //    This allows us to release the lock on cs_vNodes earlier while still protecting node from deletion
        node->AddRef();
    }
    
    UnlimitedPushTxns(node);

    //BU: Remember to release the reference we took on node to protect from use-after-free
    node->Release();

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
    MIN_TX_REQUEST_RETRY_INTERVAL = GetArg("-txretryinterval", MIN_TX_REQUEST_RETRY_INTERVAL);
    MIN_BLK_REQUEST_RETRY_INTERVAL = GetArg("-blkretryinterval", MIN_BLK_REQUEST_RETRY_INTERVAL);
    maxGeneratedBlock = GetArg("-blockmaxsize", maxGeneratedBlock);
    blockVersion = GetArg("-blockversion", blockVersion);
    excessiveBlockSize = GetArg("-excessiveblocksize", excessiveBlockSize);
    excessiveAcceptDepth = GetArg("-excessiveacceptdepth", excessiveAcceptDepth);
    LoadTweaks();  // The above options are deprecated so the same parameter defined as a tweak will override them

    if (maxGeneratedBlock > excessiveBlockSize)
      {
        LogPrintf("Reducing the maximum mined block from the configured %d to your excessive block size %d.  Otherwise you would orphan your own blocks.\n", maxGeneratedBlock, excessiveBlockSize);
        maxGeneratedBlock = excessiveBlockSize;
      }
    
    settingsToUserAgentString();
    //  Init network shapers
    int64_t rb = GetArg("-receiveburst", DEFAULT_MAX_RECV_BURST);
    // parameter is in KBytes/sec, leaky bucket is in bytes/sec.  But if it is "off" then don't multiply
    if (rb != std::numeric_limits<long long>::max())
        rb *= 1024;
    int64_t ra = GetArg("-receiveavg", DEFAULT_AVE_RECV);
    if (ra != std::numeric_limits<long long>::max())
        ra *= 1024;
    int64_t sb = GetArg("-sendburst", DEFAULT_MAX_SEND_BURST);
    if (sb != std::numeric_limits<long long>::max())
        sb *= 1024;
    int64_t sa = GetArg("-sendavg", DEFAULT_AVE_SEND);
    if (sa != std::numeric_limits<long long>::max())
        sa *= 1024;

    receiveShaper.set(rb, ra);
    sendShaper.set(sb, sa);

    txAdded.init("memPool/txAdded");
    poolSize.init("memPool/size",STAT_OP_AVE | STAT_KEEP);
    recvAmt.init("net/recv/total");
    recvAmt.init("net/send/total");
    std::vector<std::string> msgTypes = getAllNetMessageTypes();
   
    for (std::vector<std::string>::const_iterator i=msgTypes.begin(); i!=msgTypes.end();++i)
    {
        mallocedStats.push_front(new CStatHistory<uint64_t >("net/recv/msg/" +  *i));
        mallocedStats.push_front(new CStatHistory<uint64_t >("net/send/msg/" +  *i));
    }

    xpeditedBlk.reserve(256); 
    xpeditedBlkUp.reserve(256);
    xpeditedTxn.reserve(256);  

    // make outbound conns modifiable by the user
    int nUserMaxOutConnections = GetArg("-maxoutconnections", DEFAULT_MAX_OUTBOUND_CONNECTIONS);
    nMaxOutConnections = std::max(nUserMaxOutConnections,0);

    if (nMaxConnections < nMaxOutConnections) {
      // uiInterface.ThreadSafeMessageBox((strprintf(_("Reducing -maxoutconnections from %d to %d, because this value is higher than max available connections."), nUserMaxOutConnections, nMaxConnections)),"", CClientUIInterface::MSG_WARNING);
      LogPrintf("Reducing -maxoutconnections from %d to %d, because this value is higher than max available connections.\n", nUserMaxOutConnections, nMaxConnections);
      nMaxOutConnections = nMaxConnections;
    }

}

void UnlimitedCleanup()
{
    CStatBase* obj = NULL;
    while (!mallocedStats.empty())
    {
        obj = mallocedStats.front();
        delete obj;
        mallocedStats.pop_front();
    }
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
    return FormatParagraph(strprintf(_("Copyright (C) 2015-%i The Bitcoin Unlimited Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2009-%i The Bitcoin Core Developers"), COPYRIGHT_YEAR)) + "\n\n" +
           FormatParagraph(strprintf(_("Portions Copyright (C) 2014-%i The Bitcoin XT Developers"), COPYRIGHT_YEAR)) + "\n\n" +
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

bool CheckExcessive(const CBlock& block, uint64_t blockSize, uint64_t nSigOps, uint64_t nTx, uint64_t largestTx)
{
    if (blockSize > excessiveBlockSize) 
    {
        LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d  :too many bytes\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps);
        return true;
    }

    if (blockSize > BLOCKSTREAM_CORE_MAX_BLOCK_SIZE)
      {
        // Check transaction size to limit sighash
        if (largestTx > maxTxSize.value)
          {
          LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " largest TX:%d  :tx too large.  Expected less than: %d\n", block.nVersion, block.nTime, blockSize, nTx, largestTx, maxTxSize.value);
          return true;
          }

        // check proportional sigops
        uint64_t blockMbSize = 1+((blockSize-1)/1000000);  // block size in megabytes rounded up. 1-1000000 -> 1, 1000001-2000000 -> 2, etc.
        if (nSigOps > blockSigopsPerMb.value*blockMbSize)
          {
            LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d  :too many sigops.  Expected less than: %d\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps, blockSigopsPerMb.value*blockMbSize);
            return true;
          }
      }
    else
      {
        // Within a 1MB block transactions can be 1MB, so nothing to check WRT transaction size

        // Check max sigops
        if (nSigOps > BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS)
          {
            LogPrintf("Excessive block: ver:%x time:%d size: %" PRIu64 " Tx:%" PRIu64 " Sig:%d  :too many sigops.  Expected < 1MB defined constant: %d\n", block.nVersion, block.nTime, blockSize, nTx, nSigOps, BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS);
            return true;
          }
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

    unsigned int ebs=0;
    if (params[0].isNum())
        ebs = params[0].get_int64();
    else {
        string temp = params[0].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        ebs = boost::lexical_cast<unsigned int>(temp);
    }

    std::string estr = ebTweak.Validate(ebs);
    if (! estr.empty())
      throw runtime_error(estr);
    ebTweak.Set(ebs);

    if (params[1].isNum())
        excessiveAcceptDepth = params[1].get_int64();
    else {
        string temp = params[1].get_str();
        if (temp[0] == '-') boost::throw_exception( boost::bad_lexical_cast() );
        excessiveAcceptDepth = boost::lexical_cast<unsigned int>(temp);
    }

    settingsToUserAgentString();
    std::ostringstream ret;
    ret << "Excessive Block set to " << excessiveBlockSize << " bytes.  Accept Depth set to " << excessiveAcceptDepth << " blocks.";    
    return UniValue(ret.str());
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
    if (fHelp || params.size() != 1)
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
    // This also fixes issues user issues where people provide the value as MB
    if (arg < 100)
        throw runtime_error("max generated block size must be greater than 100 bytes");

    std::string ret = miningBlockSize.Validate(params[0]);
    if (!ret.empty())
      throw runtime_error(ret.c_str());
    return miningBlockSize.Set(params[0]);
}

UniValue getblockversion(const UniValue& params, bool fHelp)
{
  if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockversion\n"
            "\nReturn the block version used when mining."
            "\nResult\n"
            "      (integer) block version number\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockversion", "") + HelpExampleRpc("getblockversion", ""));
  const CBlockIndex* pindex = chainActive.Tip();
  return UnlimitedComputeBlockVersion(pindex, Params().GetConsensus(),pindex->nTime);
      
}

UniValue setblockversion(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setblockversion blockVersionNumber\n"
            "\nSet the block version number.\n"
            "\nArguments:\n"
            "1. blockVersionNumber         (integer, hex integer, 'BIP109', 'BASE' or 'default'.  Required) The block version number.\n"
            "\nExamples:\n"
            "\nVote for 2MB blocks\n" +
            HelpExampleCli("setblockversion", "BIP109") +
            "\nCheck the setting\n" + HelpExampleCli("getblockversion", ""));

    uint32_t arg = 0;

    string temp = params[0].get_str();
    if (temp == "default")
      {
        arg = 0;
      }
    else if (temp == "BIP109")
      {
        arg = BASE_VERSION | FORK_BIT_2MB;
      }
    else if (temp == "BASE")
      {
        arg = BASE_VERSION;
      }
    else if ((temp[0] == '0') && (temp[1] == 'x'))
      {
        std::stringstream ss;
        ss << std::hex << (temp.c_str()+2);
        ss >> arg;
      }
    else
      {
      arg = boost::lexical_cast<unsigned int>(temp);
      }
   
    blockVersion = arg;

    return NullUniValue;
}

bool IsTrafficShapingEnabled()
{
    int64_t max, avg;

    sendShaper.get(&max, &avg);
    if (avg != std::numeric_limits<long long>::max() || max != std::numeric_limits<long long>::max())
        return true;

    receiveShaper.get(&max, &avg);
    if (avg != std::numeric_limits<long long>::max() || max != std::numeric_limits<long long>::max())
        return true;

    return false;
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
    int64_t max, avg;
    sendShaper.get(&max, &avg);
    if (avg != std::numeric_limits<long long>::max() || max != std::numeric_limits<long long>::max()) {
        ret.push_back(Pair("sendBurst", max / 1024));
        ret.push_back(Pair("sendAve", avg / 1024));
    }
    receiveShaper.get(&max, &avg);
    if (avg != std::numeric_limits<long long>::max() || max != std::numeric_limits<long long>::max()) {
        ret.push_back(Pair("recvBurst", max / 1024));
        ret.push_back(Pair("recvAve", avg / 1024));
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


// fIsChainNearlySyncd is updated only during startup and whenever we receive a header.
// This way we avoid having to lock cs_main so often which tends to be a bottleneck.
void IsChainNearlySyncdInit() 
{
    LOCK2(cs_main, cs_ischainnearlysyncd);
    if (!pindexBestHeader) fIsChainNearlySyncd = false;  // Not nearly synced if we don't have any blocks!
    else
      {
      if(chainActive.Height() < pindexBestHeader->nHeight - 2)
        fIsChainNearlySyncd = false;
      else
        fIsChainNearlySyncd = true;
      }
}

bool IsChainNearlySyncd()
{
    LOCK(cs_ischainnearlysyncd);
    return fIsChainNearlySyncd;
}

uint64_t LargestBlockSeen(uint64_t nBlockSize)
{
    // C++98 lacks the capability to do static initialization properly
    // so we need a runtime check to make sure it is.
    // This can be removed when moving to C++11 .
    if (nBlockSize < BLOCKSTREAM_CORE_MAX_BLOCK_SIZE) {
        nBlockSize = BLOCKSTREAM_CORE_MAX_BLOCK_SIZE;
    }

    // Return the largest block size that we have seen since startup
    uint64_t nSize = nLargestBlockSeen.load();
    while (nBlockSize > nSize) {
        if (nLargestBlockSeen.compare_exchange_weak(nSize, nBlockSize)) {
            return nBlockSize;
        }
    }

    return nSize;
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
    thindata.UpdateInBoundBloomFilter(nSizeFilter);
}

void HandleBlockMessage(CNode *pfrom, const string &strCommand, CBlock &block, const CInv &inv)
{
    int64_t startTime = GetTimeMicros();
    CValidationState state;
    uint64_t nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);

    // At this point we have either a block or a fully reconstructed thinblock but we still need to
    // maintain a mapThinBlocksInFlight entry so that we don't re-request a full block from
    // the same node while the block is processing. Furthermore by setting the time = -1 we prevent
    // the timeout from triggering and inadvertently disconnecting the node in the event that the block
    // takes a longer time to process than the THINBLOCK_DOWNLOAD_TIMEOUT interval.
    {
        LOCK(pfrom->cs_mapthinblocksinflight);
        if (pfrom->mapThinBlocksInFlight.count(inv.hash))
            pfrom->mapThinBlocksInFlight[inv.hash] = -1;
    }

    // Process all blocks from whitelisted peers, even if not requested,
    // unless we're still syncing with the network.
    // Such an unrequested block may still be processed, subject to the
    // conditions in AcceptBlock().
    bool forceProcessing = pfrom->fWhitelisted && !IsInitialBlockDownload();
    const CChainParams& chainparams = Params();
    pfrom->firstBlock += 1;
    ProcessNewBlock(state, chainparams, pfrom, &block, forceProcessing, NULL);
    int nDoS;
    if (state.IsInvalid(nDoS)) {
        LogPrintf("Invalid block due to %s\n", state.GetRejectReason().c_str());
        if (!strCommand.empty())
	  {
          pfrom->PushMessage("reject", strCommand, state.GetRejectCode(),
                           state.GetRejectReason().substr(0, MAX_REJECT_MESSAGE_LENGTH), inv.hash);
          if (nDoS > 0) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), nDoS);
          }
	}
    }
    else {
        LargestBlockSeen(nSizeBlock); // update largest block seen

        double nValidationTime = (double)(GetTimeMicros() - startTime) / 1000000.0;
        if (strCommand != NetMsgType::BLOCK) {
            LogPrint("thin", "Processed ThinBlock %s in %.2f seconds\n", inv.hash.ToString(), (double)(GetTimeMicros() - startTime) / 1000000.0);
            thindata.UpdateValidationTime(nValidationTime);
        }
        else
            LogPrint("thin", "Processed Regular Block %s in %.2f seconds\n", inv.hash.ToString(), (double)(GetTimeMicros() - startTime) / 1000000.0);
    }

    // When we request a thinblock we may get back a regular block if it is smaller than a thinblock
    // Therefore we have to remove the thinblock in flight if it exists and we also need to check that 
    // the block didn't arrive from some other peer.  This code ALSO cleans up the thin block that
    // was passed to us (&block), so do not use it after this.
    {
        int nTotalThinBlocksInFlight = 0;
        {
            LOCK2(cs_vNodes, pfrom->cs_mapthinblocksinflight);
            // Erase this thinblock from the tracking map now that we're done with it.
            if (pfrom->mapThinBlocksInFlight.erase(inv.hash))
            {
                // Clear out and reset thinblock data
                thindata.ClearThinBlockData(pfrom);
            }
          
            // Count up any other remaining nodes with thinblocks in flight.
            BOOST_FOREACH (CNode *pnode, vNodes)
            {
                if (pnode->mapThinBlocksInFlight.size() > 0)
                    nTotalThinBlocksInFlight++;
            }
            pfrom->firstBlock += 1; // update statistics, requires cs_vNodes
        }

        // When we no longer have any thinblocks in flight then clear the set
        // just to make sure we don't somehow get growth over time.
        if (nTotalThinBlocksInFlight == 0) {

            thindata.ResetThinBlockBytes();

            LOCK(cs_xval);
            setPreVerifiedTxHash.clear();
            setUnVerifiedOrphanTxHash.clear();
        }
    }

    if (!IsInitialBlockDownload())
    {
        LOCK(cs_orphancache);
        {
            // Erase any orphans that may have been in the previous block and arrived
            // after the previous block had already been processed.
            LOCK(cs_previousblock);
            for (unsigned int i = 0; i < vPreviousBlock.size(); i++)
            {
                EraseOrphanTx(vPreviousBlock[i]);
            }
            vPreviousBlock.clear();

            // Erase orphans from the current block that were already received.
            for (unsigned int i = 0; i < block.vtx.size(); i++)
            {
                uint256 hash = block.vtx[i].GetHash();
                vPreviousBlock.push_back(hash);
                EraseOrphanTx(hash);
            }
        }
    }

    // Clear the thinblock timer used for preferential download
    thindata.ClearThinBlockTimer(inv.hash);
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
  LOCK(cs_statMap);
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
  LOCK(cs_statMap);
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
            "2. \"series\"  (string, optional) Specify what data series you want.  Options are \"total\", \"now\",\"all\", \"sec10\", \"min5\", \"hourly\", \"daily\",\"monthly\".  Default is all.\n"
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
      seriesStr = "total";
    else seriesStr = params[1].get_str();
    //uint_t series = 0; 
    //if (series == "now") series |= 1;
    //if (series == "all") series = 0xfffffff;
    LOCK(cs_statMap);

    CStatBase* base = FindStatistic(params[0].get_str().c_str());
    if (base)
      {
        UniValue ustat(UniValue::VOBJ);
        if (seriesStr == "now")
          {
	    ustat.push_back(Pair("now", base->GetNow()));
	  }
        else if (seriesStr == "total")
          {
	    ustat.push_back(Pair("total", base->GetTotal()));
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

#ifdef DEBUG
#ifdef DEBUG_LOCKORDER
extern std::map<std::pair<void*, void*>, LockStack> lockorders;
#endif

extern std::vector<std::string> vUseDNSSeeds;
extern std::list<CNode*> vNodesDisconnected;
extern std::set<CNetAddr> setservAddNodeAddresses;
extern UniValue getstructuresizes(const UniValue& params, bool fHelp)
{
    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("time", GetTime()));
    ret.push_back(Pair("requester.mapTxnInfo", requester.mapTxnInfo.size()));
    ret.push_back(Pair("requester.mapBlkInfo", requester.mapBlkInfo.size()));
    unsigned long int max = 0;
    unsigned long int size = 0;
    for (CRequestManager::OdMap::iterator i = requester.mapTxnInfo.begin(); i != requester.mapTxnInfo.end(); i++)
    {
        unsigned long int temp = i->second.availableFrom.size();
        size += temp;
        if (max < temp) max = temp;
    }
    ret.push_back(Pair("requester.mapTxnInfo.maxobj", max));
    ret.push_back(Pair("requester.mapTxnInfo.totobj", size));

    max = 0;
    size = 0;
    for (CRequestManager::OdMap::iterator i = requester.mapBlkInfo.begin(); i != requester.mapBlkInfo.end(); i++)
    {
        unsigned long int temp = i->second.availableFrom.size();
        size += temp;
        if (max < temp) max = temp;
    }
    ret.push_back(Pair("requester.mapBlkInfo.maxobj", max));
    ret.push_back(Pair("requester.mapBlkInfo.totobj", size));

    ret.push_back(Pair("mapBlockIndex", mapBlockIndex.size()));
    // CChain
    ret.push_back(Pair("mapAlerts", mapAlerts.size()));
    ret.push_back(Pair("setPreVerifiedTxHash", setPreVerifiedTxHash.size()));
    ret.push_back(Pair("setUnVerifiedOrphanTxHash", setUnVerifiedOrphanTxHash.size()));
    ret.push_back(Pair("mapLocalHost", mapLocalHost.size()));
    ret.push_back(Pair("CNode::vWhitelistedRange", CNode::vWhitelistedRange.size()));
    ret.push_back(Pair("mapInboundConnectionTracker", mapInboundConnectionTracker.size()));
    ret.push_back(Pair("vUseDNSSeeds", vUseDNSSeeds.size()));
    ret.push_back(Pair("vAddedNodes", vAddedNodes.size()));
    ret.push_back(Pair("setservAddNodeAddresses", setservAddNodeAddresses.size()));
    ret.push_back(Pair("statistics", statistics.size()));
    ret.push_back(Pair("tweaks", tweaks.size()));
    ret.push_back(Pair("mapRelay", mapRelay.size()));
    ret.push_back(Pair("vRelayExpiration",vRelayExpiration.size()));
    ret.push_back(Pair("vNodes",vNodes.size()));
    ret.push_back(Pair("vNodesDisconnected",vNodesDisconnected.size()));
    // CAddrMan
    ret.push_back(Pair("mapOrphanTransactions",mapOrphanTransactions.size()));
    ret.push_back(Pair("mapOrphanTransactionsByPrev",mapOrphanTransactionsByPrev.size()));
    ret.push_back(Pair("xpeditedBlk",xpeditedBlk.size()));
    ret.push_back(Pair("xpeditedBlkUp",xpeditedBlkUp.size()));
    ret.push_back(Pair("xpeditedTxn",xpeditedTxn.size()));
#ifdef DEBUG_LOCKORDER
    ret.push_back(Pair("lockorders",lockorders.size()));
#endif

    LOCK(cs_vNodes);
    std::vector<CNode*>::iterator n;
    uint64_t totalThinBlockSize = 0;
    int disconnected = 0;  // watch # of disconnected nodes to ensure they are being cleaned up
    for (std::vector<CNode*>::iterator it = vNodes.begin(); it != vNodes.end(); ++it)
    {
        if (*it == NULL) continue;
        CNode& n = **it;
        UniValue node(UniValue::VOBJ);
        disconnected += (n.fDisconnect) ? 1: 0;

        node.push_back(Pair("vSendMsg", n.vSendMsg.size()));
        node.push_back(Pair("vRecvGetData", n.vRecvGetData.size()));
        node.push_back(Pair("vRecvMsg", n.vRecvMsg.size()));
        if (n.pfilter)
        {
            node.push_back(Pair("pfilter", n.pfilter->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION)));
        }
        if (n.pThinBlockFilter)
        {
            node.push_back(Pair("pThinBlockFilter",
                                n.pThinBlockFilter->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION)));
        }
        node.push_back(Pair("thinblock.vtx", n.thinBlock.vtx.size()));
        uint64_t thinBlockSize = ::GetSerializeSize(n.thinBlock, SER_NETWORK, PROTOCOL_VERSION);
        totalThinBlockSize += thinBlockSize;
        node.push_back(Pair("thinblock.size", thinBlockSize));
        node.push_back(Pair("thinBlockHashes", n.thinBlockHashes.size()));
        node.push_back(Pair("xThinBlockHashes", n.xThinBlockHashes.size()));
        node.push_back(Pair("vAddrToSend", n.vAddrToSend.size()));
        node.push_back(Pair("vInventoryToSend", n.vInventoryToSend.size()));
        node.push_back(Pair("setAskFor", n.setAskFor.size()));
        node.push_back(Pair("mapAskFor", n.mapAskFor.size()));
        ret.push_back(Pair(n.addrName,node));
    }
    ret.push_back(Pair("totalThinBlockSize", totalThinBlockSize));
    ret.push_back(Pair("disconnectedNodes", disconnected));

    return ret;
}
#endif
