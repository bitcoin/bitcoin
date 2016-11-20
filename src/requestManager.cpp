// Copyright (c) 2016 The Bitcoin Unlimited developers
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
#include "requestManager.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <inttypes.h>



using namespace std;

extern CCriticalSection cs_orphancache; // from main.h

// Request management
extern CRequestManager requester;

unsigned int MIN_TX_REQUEST_RETRY_INTERVAL = 5*1000*1000;  // When should I request an object from someone else (in microseconds)
unsigned int MIN_BLK_REQUEST_RETRY_INTERVAL = 5*1000*1000;  // When should I request a block from someone else (in microseconds)

// defined in main.cpp.  should be moved into a utilities file but want to make rebasing easier
extern bool CanDirectFetch(const Consensus::Params &consensusParams);
extern void MarkBlockAsInFlight(NodeId nodeid, const uint256& hash, const Consensus::Params& consensusParams, CBlockIndex *pindex = NULL);
// note mark block as in flight is redundant with the request manager now...

CRequestManager::CRequestManager(): inFlightTxns("reqMgr/inFlight",STAT_OP_MAX),receivedTxns("reqMgr/received"),rejectedTxns("reqMgr/rejected"),droppedTxns("reqMgr/dropped", STAT_KEEP),pendingTxns("reqMgr/pending", STAT_KEEP)
  ,requestPacer(512,256) // Max and average # of requests that can be made per second
  ,blockPacer(64,32) // Max and average # of block requests that can be made per second
{
  inFlight=0;
  // maxInFlight = 256;
  
  sendIter = mapTxnInfo.end();
  sendBlkIter = mapBlkInfo.end();
}


void CRequestManager::cleanup(OdMap::iterator& itemIt)
{
  CUnknownObj& item = itemIt->second;
  inFlight-= item.outstandingReqs; // Because we'll ignore anything deleted from the map, reduce the # of requests in flight by every request we made for this object
  droppedTxns -= (item.outstandingReqs-1);
  pendingTxns -= 1;

  LOCK(cs_vNodes);

  // remove all the source nodes
  for (CUnknownObj::ObjectSourceList::iterator i = item.availableFrom.begin(); i != item.availableFrom.end(); ++i)
    {
      CNode* node = i->node;
      if (node)
        {
	  i->clear();
          LogPrint("req", "ReqMgr: %s removed ref to %d count %d.\n", item.obj.ToString(), node->GetId(), node->GetRefCount());
          node->Release();     
	}
    }
  item.availableFrom.clear();

  if (item.obj.type == MSG_TX)
    {
      if (sendIter == itemIt) ++sendIter;
      mapTxnInfo.erase(itemIt);
    }
  else
    {
      if (sendBlkIter == itemIt) ++sendBlkIter;    
      mapBlkInfo.erase(itemIt);
    }
}

// Get this object from somewhere, asynchronously.
void CRequestManager::AskFor(const CInv& obj, CNode* from, int priority)
{
  //LogPrint("req", "ReqMgr: Ask for %s.\n", obj.ToString().c_str());

  LOCK(cs_objDownloader);
  if (obj.type == MSG_TX)
    {
      uint256 temp = obj.hash;
      OdMap::value_type v(temp,CUnknownObj());
      std::pair<OdMap::iterator,bool> result = mapTxnInfo.insert(v);
      OdMap::iterator& item = result.first;
      CUnknownObj& data = item->second;
      data.obj = obj;
      if (result.second)  // inserted
	{
	  pendingTxns+=1;
	  // all other fields are zeroed on creation
	}
      // else the txn already existed so nothing to do

      data.priority = max(priority,data.priority);
      // Got the data, now add the node as a source
      data.AddSource(from);
    }
  else if ((obj.type == MSG_BLOCK) || (obj.type == MSG_THINBLOCK) || (obj.type == MSG_XTHINBLOCK))
    {
      uint256 temp = obj.hash;
      OdMap::value_type v(temp,CUnknownObj());
      std::pair<OdMap::iterator,bool> result = mapBlkInfo.insert(v);
      OdMap::iterator& item = result.first;
      CUnknownObj& data = item->second;
      data.obj = obj;
      // if (result.second)  // means this was inserted rather than already existed
      // { } nothing to do
      data.priority = max(priority,data.priority);
      data.AddSource(from);
      LogPrint("blk", "%s available at %s\n", obj.ToString().c_str(), from->addrName.c_str());
    }
  else
    {
      assert(!"TBD");
    }

}

// Get these objects from somewhere, asynchronously.
void CRequestManager::AskFor(const std::vector<CInv>& objArray, CNode* from, int priority)
{
  unsigned int sz = objArray.size();
  for (unsigned int nInv = 0; nInv < sz; nInv++)
    {
      AskFor(objArray[nInv],from, priority);
    }
}


// Indicate that we got this object, from and bytes are optional (for node performance tracking)
void CRequestManager::Received(const CInv& obj, CNode* from, int bytes)
{
  int64_t now = GetTimeMicros();
  LOCK(cs_objDownloader);
  if (obj.type == MSG_TX)
    {
      OdMap::iterator item = mapTxnInfo.find(obj.hash);
      if (item ==  mapTxnInfo.end()) return;  // item has already been removed
      LogPrint("req", "ReqMgr: TX received for %s.\n", item->second.obj.ToString().c_str());
      from->txReqLatency << (now - item->second.lastRequestTime);  // keep track of response latency of this node
      // will be decremented in the item cleanup: if (inFlight) inFlight--;
      cleanup(item); // remove the item
      receivedTxns += 1;
    }
  else if ((obj.type == MSG_BLOCK) || (obj.type == MSG_THINBLOCK) || (obj.type == MSG_XTHINBLOCK))
    {
      OdMap::iterator item = mapBlkInfo.find(obj.hash);
      if (item ==  mapBlkInfo.end()) return;  // item has already been removed
      LogPrint("blk", "%s removed from request queue (received from %s (%d)).\n", item->second.obj.ToString().c_str(),from->addrName.c_str(), from->id);
      //from->blkReqLatency << (now - item->second.lastRequestTime);  // keep track of response latency of this node
      cleanup(item); // remove the item
      //receivedTxns += 1;
    }
}

// Indicate that we got this object, from and bytes are optional (for node performance tracking)
void CRequestManager::AlreadyReceived(const CInv& obj)
{
  LOCK(cs_objDownloader);
  OdMap::iterator item = mapTxnInfo.find(obj.hash);
  if (item ==  mapTxnInfo.end()) 
    {
    item = mapBlkInfo.find(obj.hash);
    if (item ==  mapBlkInfo.end()) return;  // Not in any map
    }
  LogPrint("req", "ReqMgr: Already received %s.  Removing request.\n", item->second.obj.ToString().c_str());
  // will be decremented in the item cleanup: if (inFlight) inFlight--;
  cleanup(item); // remove the item
}

// Indicate that we got this object, from and bytes are optional (for node performance tracking)
void CRequestManager::Rejected(const CInv& obj, CNode* from, unsigned char reason)
{
  LOCK(cs_objDownloader);
  OdMap::iterator item;
  if (obj.type == MSG_TX)
    {
      item = mapTxnInfo.find(obj.hash);
      if (item ==  mapTxnInfo.end()) 
	{
	  LogPrint("req", "ReqMgr: Item already removed. Unknown txn rejected %s\n", obj.ToString().c_str());
	  return;
	}
      if (inFlight) inFlight--;
      if (item->second.outstandingReqs) item->second.outstandingReqs--;

      rejectedTxns += 1;
    }
  else if ((obj.type == MSG_BLOCK) || (obj.type == MSG_THINBLOCK) || (obj.type == MSG_XTHINBLOCK))
    {
      item = mapBlkInfo.find(obj.hash);
      if (item ==  mapBlkInfo.end()) 
	{
	  LogPrint("req", "ReqMgr: Item already removed. Unknown block rejected %s\n", obj.ToString().c_str());
	  return;
	}
    }

  if (reason == REJECT_MALFORMED)
    {
    }
  else if (reason == REJECT_INVALID)
    {
    }
  else if (reason == REJECT_OBSOLETE)
    {
    }
  else if (reason == REJECT_CHECKPOINT)
    {
    }	  
  else if (reason == REJECT_INSUFFICIENTFEE)
    {
      item->second.rateLimited = true;
    }
  else if (reason == REJECT_DUPLICATE)
    {
      // TODO figure out why this might happen.
    }
  else if (reason == REJECT_NONSTANDARD)
    {
      // Not going to be in any memory pools... does the TX request also look in blocks?
      // TODO remove from request manager (and mark never receivable?)
      // TODO verify that the TX request command also looks in blocks?
    }
  else if (reason == REJECT_DUST)
    {
    }
  else if (reason == REJECT_NONSTANDARD)
    {
    }
  else
    {
      LogPrint("req", "ReqMgr: Unknown TX rejection code [0x%x].\n",reason);
      //assert(0); // TODO
    }
}

CNodeRequestData::CNodeRequestData(CNode* n)
{
  assert(n);
  node=n;  
  requestCount=0;
  desirability=0;

  const int MaxLatency = 10*1000*1000;  // After 10 seconds latency I don't care

  // Calculate how much we like this node:

  if (node->ThinBlockCapable() && IsChainNearlySyncd())  // Prefer thin block nodes over low latency ones when the chain is syncd
    {
      desirability += MaxLatency;
    }

  // The bigger the latency (in microseconds), the less we want to request from this node
  int latency = node->txReqLatency.GetTotal().get_int();
  if (latency == 0) // data has never been requested from this node.  Should we encourage investigation into whether this node is fast, or stick with nodes that we do have data on?
    {
      latency = 80*1000; // assign it a reasonably average latency (80ms) for sorting purposes
    }
  if (latency > MaxLatency) latency = MaxLatency;
  desirability -= latency;
}

void CUnknownObj::AddSource(CNode* from)
{
  if (std::find_if(availableFrom.begin(), availableFrom.end(), IsCNodeRequestDataThisNode(from)) == availableFrom.end())  // node is not in the request list
    {
      LogPrint("req", "%s added ref to node %d.  Current count %d.\n", obj.ToString(), from->GetId(), from->GetRefCount());
      {
        LOCK(cs_vNodes);  // This lock is needed to ensure that AddRef happens atomically
        from->AddRef();
      }
      CNodeRequestData req(from);
      for (ObjectSourceList::iterator i = availableFrom.begin(); i != availableFrom.end(); ++i)
        {
	  if (i->desirability < req.desirability)
	    {
              availableFrom.insert(i, req);
              return;
	    }
        }
      availableFrom.push_back(req);
    }
}

void RequestBlock(CNode* pfrom, CInv obj)
{
  const CChainParams& chainParams = Params();

  // First request the headers preceding the announced block. In the normal fully-synced
  // case where a new block is announced that succeeds the current tip (no reorganization),
  // there are no such headers.
  // Secondly, and only when we are close to being synced, we request the announced block directly,
  // to avoid an extra round-trip. Note that we must *first* ask for the headers, so by the
  // time the block arrives, the header chain leading up to it is already validated. Not
  // doing this will result in the received block being rejected as an orphan in case it is
  // not a direct successor.
  //  NOTE: only download headers if we're not doing IBD.  The IBD process will take care of it's own headers.
  //        Also, we need to always download headers for "regtest". TODO: we need to redesign how IBD is initiated here.
  if (IsChainNearlySyncd() || chainParams.NetworkIDString() == "regtest")
  {
    LogPrint("net", "getheaders (%d) %s to peer=%d\n", pindexBestHeader->nHeight, obj.hash.ToString(), pfrom->id);  
    pfrom->PushMessage(NetMsgType::GETHEADERS, chainActive.GetLocator(pindexBestHeader), obj.hash);
  }

  {
      // BUIP010 Xtreme Thinblocks: begin section
      CInv inv2(obj);
      CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
      CBloomFilter filterMemPool;
      if (IsThinBlocksEnabled() && IsChainNearlySyncd()) 
	{
	  if (HaveConnectThinblockNodes() || (HaveThinblockNodes() && CheckThinblockTimer(obj.hash))) 
	    {
	      // Must download a block from a ThinBlock peer
	      if (pfrom->mapThinBlocksInFlight.size() < 1 && CanThinBlockBeDownloaded(pfrom)) 
		{ // We can only send one thinblock per peer at a time
		  pfrom->mapThinBlocksInFlight[inv2.hash] = GetTime();
		  inv2.type = MSG_XTHINBLOCK;
		  std::vector<uint256> vOrphanHashes;
                  {
                    LOCK(cs_orphancache);
                    for (map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi)
                        vOrphanHashes.push_back((*mi).first);
                  }
		  BuildSeededBloomFilter(filterMemPool, vOrphanHashes,inv2.hash);
		  ss << inv2;
		  ss << filterMemPool;
		  pfrom->PushMessage(NetMsgType::GET_XTHIN, ss);
		  MarkBlockAsInFlight(pfrom->GetId(), obj.hash, chainParams.GetConsensus());
		  LogPrint("thin", "Requesting Thinblock %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
		}
	    }
	  else 
	    {
	      // Try to download a thinblock if possible otherwise just download a regular block
	      if (pfrom->mapThinBlocksInFlight.size() < 1 && CanThinBlockBeDownloaded(pfrom)) { // We can only send one thinblock per peer at a time
		  pfrom->mapThinBlocksInFlight[inv2.hash] = GetTime();
		  inv2.type = MSG_XTHINBLOCK;
		  std::vector<uint256> vOrphanHashes;
                  {
                    LOCK(cs_orphancache);
                    for (map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi)
                        vOrphanHashes.push_back((*mi).first);
                  }
		  BuildSeededBloomFilter(filterMemPool, vOrphanHashes,inv2.hash);
		  ss << inv2;
		  ss << filterMemPool;
		  pfrom->PushMessage(NetMsgType::GET_XTHIN, ss);
		  LogPrint("thin", "Requesting Thinblock %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
	        }
	      else 
		{
		  LogPrint("thin", "Requesting Regular Block %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
		  std::vector<CInv> vToFetch;
		  inv2.type = MSG_BLOCK;
		  vToFetch.push_back(inv2);
		  pfrom->PushMessage(NetMsgType::GETDATA, vToFetch);
		}
	      MarkBlockAsInFlight(pfrom->GetId(), obj.hash, chainParams.GetConsensus());
	    }
	}
      else 
	{
	  std::vector<CInv> vToFetch;
	  inv2.type = MSG_BLOCK;
	  vToFetch.push_back(inv2);
	  pfrom->PushMessage(NetMsgType::GETDATA, vToFetch);
	  MarkBlockAsInFlight(pfrom->GetId(), obj.hash, chainParams.GetConsensus());
	  LogPrint("thin", "Requesting Regular Block %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
	}
      // BUIP010 Xtreme Thinblocks: end section
  }
}


void CRequestManager::SendRequests()
{
  int64_t now = 0;

  // TODO: if a node goes offline, rerequest txns from someone else and cleanup references right away
  cs_objDownloader.lock();
  if (sendBlkIter == mapBlkInfo.end()) sendBlkIter = mapBlkInfo.begin();

  // Modify retry interval. If we're doing IBD or if Traffic Shaping is ON we want to have a longer interval because 
  // those blocks and txns can take much longer to download.
  unsigned int blkReqRetryInterval = MIN_BLK_REQUEST_RETRY_INTERVAL;
  unsigned int txReqRetryInterval = MIN_TX_REQUEST_RETRY_INTERVAL;
 if ((!IsChainNearlySyncd() && Params().NetworkIDString() != "regtest") || IsTrafficShapingEnabled()) 
    {
      blkReqRetryInterval *= 6;
      txReqRetryInterval *= (12*2);  // we want to optimise block DL during IBD (and give lots of time for shaped nodes) so push the TX retry up to 2 minutes (default val of MIN_TX is 5 sec)
    }

  // Get Blocks
  while (sendBlkIter != mapBlkInfo.end())
   {
      now = GetTimeMicros();
      OdMap::iterator itemIter = sendBlkIter;
      CUnknownObj& item = itemIter->second;

      ++sendBlkIter;  // move it forward up here in case we need to erase the item we are working with.
      if (itemIter == mapBlkInfo.end()) break;

      if (now-item.lastRequestTime > blkReqRetryInterval)  // if never requested then lastRequestTime==0 so this will always be true
	{
          if (!item.availableFrom.empty())
	    {
	      CNodeRequestData next;
              while (!item.availableFrom.empty() && (next.node == NULL)) // Go thru the availableFrom list, looking for the first node that isn't disconnected
                {
  	        next = item.availableFrom.front();  // Grab the next location where we can find this object.
                item.availableFrom.pop_front();
                if (next.node != NULL)
                  {
                    // Do not request from this node if it was disconnected or the node pingtime is far beyond acceptable during initial block download.
                    // We only check pingtime during IBD because we don't want to lock vNodes too often and when the chain is syncd, waiting
                    // just 5 seconds for a timeout is not an issue, however waiting for a slow node during IBD can really slow down the process.
                    //   TODO: Eventually when we move away from vNodes or have a different mechanism for tracking ping times we can include
                    //   this filtering in all our requests for blocks and transactions.
		    if (next.node->fDisconnect || (!IsChainNearlySyncd() && !IsNodePingAcceptable(next.node)))
		      {
                        LOCK(cs_vNodes);
                        LogPrint("req", "ReqMgr: %s removed ref to %d count %d (disconnect).\n", item.obj.ToString(), next.node->GetId(), next.node->GetRefCount());
                        next.node->Release();
                        next.node = NULL; // force the loop to get another node            
		      }
		  }
	        }

	      if (next.node != NULL )
		{
                  // If item.lastRequestTime is true then we've requested at least once and we'll try a re-request
		  if (item.lastRequestTime)
		    {
		      LogPrint("req", "Block request timeout for %s.  Retrying\n", item.obj.ToString().c_str());
		    }

		  CInv obj = item.obj;
		  cs_objDownloader.unlock();

                  RequestBlock(next.node, obj);
                  item.outstandingReqs++;
                  item.lastRequestTime = now;

		  cs_objDownloader.lock();

                  // If you wanted to remember that this node has this data, you could push it back onto the end of the availableFrom list like this:
                  // next.requestCount += 1;
		  // next.desirability /= 2;  // Make this node less desirable to re-request.
		  // item.availableFrom.push_back(next);  // Add the node back onto the end of the list

                  // Instead we'll forget about it -- the node is already popped of of the available list so now we'll release our reference.
                  LOCK(cs_vNodes);
                  LogPrint("req", "ReqMgr: %s removed ref to %d count %d (disconnect).\n", item.obj.ToString(), next.node->GetId(), next.node->GetRefCount());
                  next.node->Release();
                  next.node = NULL;
		}
              else
		{
		  // node should never be null... but if it is then there's nothing to do.
                  LogPrint("req", "Block %s has no sources\n",item.obj.ToString());
		}
	    }
	  else
	    {
	      // node should never be null... but if it is then there's nothing to do.
	      LogPrint("req", "Block %s has no available sources\n",item.obj.ToString());
	    }

	}    
    }
  
  // Get Transactions
  if (sendIter == mapTxnInfo.end()) sendIter = mapTxnInfo.begin();
  while ((sendIter != mapTxnInfo.end()) && requestPacer.try_leak(1))
    {
      now = GetTimeMicros();
      OdMap::iterator itemIter = sendIter;
      CUnknownObj& item = itemIter->second;

      ++sendIter;  // move it forward up here in case we need to erase the item we are working with.
      if (itemIter == mapTxnInfo.end()) break;

      if (now-item.lastRequestTime > txReqRetryInterval)  // if never requested then lastRequestTime==0 so this will always be true
	{
          if (!item.rateLimited)
	    {
                // If item.lastRequestTime is true then we've requested at least once, so this is a rerequest -> a txn request was dropped.
		if (item.lastRequestTime)
		{
		  LogPrint("req", "Request timeout for %s.  Retrying\n", item.obj.ToString().c_str());
		  // Not reducing inFlight; it's still outstanding and will be cleaned up when item is removed from map
                  droppedTxns += 1;  // note we can never be sure its really dropped verses just delayed for a long time so this is not authoritative.
		}

              if (item.availableFrom.empty())
		{
		  // TODO: tell someone about this issue, look in a random node, or something.
		  cleanup(itemIter);  // right now we give up requesting it if we have no other sources...
		}
              else  // Ok, we have at least on source so request this item.
	        {
		  CNodeRequestData next;
		  while (!item.availableFrom.empty() && (next.node == NULL)) // Go thru the availableFrom list, looking for the first node that isn't disconnected
                    {
		    next = item.availableFrom.front();  // Grab the next location where we can find this object.
		    item.availableFrom.pop_front();
		    if (next.node != NULL)
		      {
			if (next.node->fDisconnect)  // Node was disconnected so we can't request from it
			  {
			    LOCK(cs_vNodes);
			    LogPrint("req", "ReqMgr: %s removed ref to %d count %d (disconnect).\n", item.obj.ToString(), next.node->GetId(), next.node->GetRefCount());
			    next.node->Release();
			    next.node = NULL; // force the loop to get another node            
			  }
		      }
		    }

	          if (next.node != NULL )
		    {
                    if (1)
                      {
                        cs_objDownloader.unlock();
                        LOCK(next.node->cs_vSend);
  		        // from->AskFor(item.obj); basically just shoves the req into mapAskFor
                        if (1) // This commented code does skips requesting TX if the node is not synced.  But the req mgr should not make this decision, the caller should not give the TX to me... !item.lastRequestTime || (item.lastRequestTime && IsChainNearlySyncd()))
                          {
                            next.node->mapAskFor.insert(std::make_pair(now, item.obj));
                            item.outstandingReqs++;
		            item.lastRequestTime = now;
                          }
                        cs_objDownloader.lock();
                      }
                      {
                        LOCK(cs_vNodes);
                        LogPrint("req", "ReqMgr: %s removed ref to %d count %d (disconnect).\n", item.obj.ToString(), next.node->GetId(), next.node->GetRefCount());
                        next.node->Release();
                        next.node = NULL;
		      }
                      inFlight++;
                      inFlightTxns << inFlight;
		    }
		}
	    }
	}

    }

  cs_objDownloader.unlock();
}

bool CRequestManager::IsNodePingAcceptable(CNode* pfrom)
{
    // Calculate average ping time of all nodes
    uint16_t nValidNodes = 0;
    std::vector<uint64_t> vPingTimes;
    LOCK(cs_vNodes);
    BOOST_FOREACH (CNode* pnode, vNodes) {
        if (!pnode->fDisconnect && pnode->nPingUsecTime > 0) {
            nValidNodes++;
            vPingTimes.push_back(pnode->nPingUsecTime);
        }
    }
    if (nValidNodes == 1) return true;

    // Calculate Standard Deviation and Mean of Ping Time
    using namespace boost::accumulators;
    accumulator_set<double, stats<tag::variance> > acc;
    acc = for_each(vPingTimes.begin(), vPingTimes.end(), acc);
    double nMean = mean(acc);
    double sDeviation = sqrt(variance(acc));

    // If node ping time is greater than the average plus 2 times the standard deviation, or
    // the pong has not been received, then do not request from this node.
    if ((pfrom->nPingUsecTime > (int64_t)(nMean + (2 * sDeviation))) || (pfrom->nPingUsecTime == 0)) {
        return false;
    }
    return true;
}

