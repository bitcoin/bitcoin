// Copyright (c) 2015 G. Andrew Stone
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chain.h"
#include "clientversion.h"
#include "chainparams.h"
#include "consensus/consensus.h"
#include "consensus/params.h"
#include "consensus/validation.h"
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
#include <boost/thread.hpp>
#include <inttypes.h>

using namespace std;

// Variables for statistics tracking, must be before the "requester" singleton instantiation
const char* sampleNames[] = { "sec10", "min5", "hourly", "daily","monthly"};
int operateSampleCount[] = { 30,       12,   24,  30 };
int interruptIntervals[] = { 30,       30*12,   30*12*24,   30*12*24*30 };

boost::posix_time::milliseconds statMinInterval(10000);
boost::asio::io_service stat_io_service __attribute__((init_priority(101)));

CStatMap statistics __attribute__((init_priority(102)));

CStatHistory<unsigned int, MinValMax<unsigned int> > txAdded; //"memPool/txAdded");
CStatHistory<uint64_t, MinValMax<uint64_t> > poolSize; // "memPool/size",STAT_OP_AVE);


// BUIP010 Xtreme Thinblocks Variables
std::map<uint256, uint64_t> mapThinBlockTimer;

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
        pfrom->PushMessage("reject", strCommand, (unsigned char)state.GetRejectCode(),
                           state.GetRejectReason().substr(0, MAX_REJECT_MESSAGE_LENGTH), inv.hash);
        if (nDoS > 0) {
            LOCK(cs_main);
            Misbehaving(pfrom->GetId(), nDoS);
        }
    }
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
