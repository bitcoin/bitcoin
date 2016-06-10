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
#include <inttypes.h>

using namespace std;

// Request management
CRequestManager requester;

unsigned int MIN_REQUEST_RETRY_INTERVAL = 2*1000*1000;  // When should I request an object from someone else (in microseconds)
unsigned int MIN_BLK_REQUEST_RETRY_INTERVAL = 2*1000*1000;  // When should I request a block from someone else (in microseconds)

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

  // Got the data, now add the node as a source
  LOCK(cs_vNodes);
  for (int i=0;i<CUnknownObj::MAX_AVAIL_FROM;i++)
    if (item.availableFrom[i] != NULL)
      {
        CNode * node = item.availableFrom[i];
	node->Release();
        LogPrint("req", "ReqMgr: %s removed ref to %d count %d.\n",item.obj.ToString(), node->GetId(), node->GetRefCount());
	item.availableFrom[i]=NULL; // unnecessary but let's make it very clear
      }


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
  //LogPrint("req", "ReqMgr: Ask for %s.\n",obj.ToString().c_str());

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
      else  // existing
	{
	}

      data.priority = max(priority,data.priority);
      // Got the data, now add the node as a source
      data.AddSource(from);
    }
  else if ((obj.type == MSG_BLOCK)||(obj.type == MSG_THINBLOCK)||(obj.type == MSG_XTHINBLOCK))
    {
      uint256 temp = obj.hash;
      OdMap::value_type v(temp,CUnknownObj());
      std::pair<OdMap::iterator,bool> result = mapBlkInfo.insert(v);
      OdMap::iterator& item = result.first;
      CUnknownObj& data = item->second;
      data.obj = obj;
      if (result.second)  // inserted
	{
	}
      data.priority = max(priority,data.priority);
      data.AddSource(from);
      LogPrint("blk", "%s available at %s\n",obj.ToString().c_str(),from->addrName.c_str());
    }
  else
    {
      assert(!"TBD");
      // from->firstBlock += 1;
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
      LogPrint("req", "ReqMgr: TX received for %s.\n",item->second.obj.ToString().c_str());
      from->txReqLatency << (now - item->second.lastRequestTime);  // keep track of response latency of this node
      // will be decremented in the item cleanup: if (inFlight) inFlight--;
      cleanup(item); // remove the item
      receivedTxns += 1;
    }
  else if ((obj.type == MSG_BLOCK)||(obj.type == MSG_THINBLOCK)||(obj.type == MSG_XTHINBLOCK))
    {
      OdMap::iterator item = mapBlkInfo.find(obj.hash);
      if (item ==  mapBlkInfo.end()) return;  // item has already been removed
      LogPrint("blk", "%s removed from request queue (received from %s (%d)).\n",item->second.obj.ToString().c_str(),from->addrName.c_str(), from->id);
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
  LogPrint("req", "ReqMgr: Already received %s.  Removing request.\n",item->second.obj.ToString().c_str());
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
	  LogPrint("req", "ReqMgr: Unknown object rejected %s.\n",obj.ToString().c_str());
	  return;  // item has already been removed
	}
      if (inFlight) inFlight--;
      if (item->second.outstandingReqs) item->second.outstandingReqs--;

      rejectedTxns += 1;
    }
  else if ((obj.type == MSG_BLOCK)||(obj.type == MSG_THINBLOCK)||(obj.type == MSG_XTHINBLOCK))
    {
      item = mapBlkInfo.find(obj.hash);
      if (item ==  mapBlkInfo.end()) 
	{
	  LogPrint("req", "ReqMgr: Unknown object rejected %s.\n",obj.ToString().c_str());
	  return;  // item has already been removed
	}

    }

  LogPrint("req", "ReqMgr: Request rejected for %s.\n",item->second.obj.ToString().c_str());

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

int CUnknownObj::AddSource(CNode* from)
{
  int i;
  for (i=0;i<CUnknownObj::MAX_AVAIL_FROM;i++)
    {
      if (availableFrom[i] == from) break;  // don't add the same node twice
      if (availableFrom[i] == NULL)
	{
	  if (1)
	    {
              LOCK(cs_vNodes);
	      availableFrom[i] = from->AddRef();
	    }
	  LogPrint("req", "%s added ref to node %d.  Current count %d.\n",obj.ToString(), from->GetId(), from->GetRefCount());
	  // I Don't need to clear this bit in receivingFrom, it should already be 0
	  if (i==0) // First request for this object
	    {
	      from->firstTx += 1;
	    }
	  break;
	}
    }
  return i;
}

int CUnknownObj::NextSource()
{
  int next = outstandingReqs%CUnknownObj::MAX_AVAIL_FROM;
  while (next < CUnknownObj::MAX_AVAIL_FROM)  // Go from last point forward
    {
      CNode* from = availableFrom[next];
      if (from)
	{
	  if (from->fDisconnect)  // oops source is gone!
	    {
	      availableFrom[next] = NULL;
	      from->Release();
	    }
	  else if (requestCount[next] == 0) break;
	}
      next++;
    }
  if (next == CUnknownObj::MAX_AVAIL_FROM)
    for (next=0;next<CUnknownObj::MAX_AVAIL_FROM;next++)  // Go from the beginning to the last point
      {
	CNode* from = availableFrom[next];
	if (from)
	  {
	    if (from->fDisconnect)  // oops source is gone!
	      {
		availableFrom[next] = NULL;
		from->Release();
	      }
	    else if (requestCount[next] == 0) break;
	  }
      }
  return next;
}

void CRequestManager::SendRequests()
{
  LOCK(cs_main);  // Must be locked here due to deadlock interaction with cs_objDownloader and  from->cs_vSend
  {
  LOCK(cs_objDownloader);
  if (sendIter == mapTxnInfo.end()) sendIter = mapTxnInfo.begin();
  if (sendBlkIter == mapBlkInfo.end()) sendBlkIter = mapBlkInfo.begin();
  int64_t now = GetTimeMicros();
  //static int64_t lastPass=0;
 
  // TODO: if a node goes offline, rerequest txns from someone else and cleanup references right away
  while ((sendBlkIter != mapBlkInfo.end())&&(blockPacer.try_leak(1)))
    {
      OdMap::iterator itemIter = sendBlkIter;
      CUnknownObj& item = itemIter->second;
      ++sendBlkIter;  // move it forward up here in case we need to erase the item we are working with.
      if (now-item.lastRequestTime > MIN_BLK_REQUEST_RETRY_INTERVAL)  // if never requested then lastRequestTime==0 so this will always be true
	{
	  if (item.lastRequestTime)  // if this is positive, we've requested at least once
	    {
	      LogPrint("req", "Block request timeout for %s.  Retrying\n",item.obj.ToString().c_str());
	    }
          int next = item.NextSource();
	  if (next != CUnknownObj::MAX_AVAIL_FROM)
	    {
              const CChainParams& chainParams = Params();
	      CNode* pfrom = item.availableFrom[next];
              item.requestCount[next] += 1;
	      LOCK(pfrom->cs_vSend);
	      item.receivingFrom |= (1<<next);
	      item.outstandingReqs++;
	      item.lastRequestTime = now;

	      // First request the headers preceding the announced block. In the normal fully-synced
	      // case where a new block is announced that succeeds the current tip (no reorganization),
	      // there are no such headers.
	      // Secondly, and only when we are close to being synced, we request the announced block directly,
	      // to avoid an extra round-trip. Note that we must *first* ask for the headers, so by the
	      // time the block arrives, the header chain leading up to it is already validated. Not
	      // doing this will result in the received block being rejected as an orphan in case it is
	      // not a direct successor.
	      pfrom->PushMessage(NetMsgType::GETHEADERS, chainActive.GetLocator(pindexBestHeader), item.obj.hash);
	      if (CanDirectFetch(chainParams.GetConsensus())) // Consider necessity given overall block pacer: && nodestate->nBlocksInFlight < MAX_BLOCKS_IN_TRANSIT_PER_PEER) 
		{
		  // BUIP010 Xtreme Thinblocks: begin section
		  CInv inv2(item.obj);
		  CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
		  CBloomFilter filterMemPool;
		  if (IsThinBlocksEnabled() && IsChainNearlySyncd()) 
		    {
		      if (HaveConnectThinblockNodes() || (HaveThinblockNodes() && CheckThinblockTimer(item.obj.hash))) 
			{
			  // Must download a block from a ThinBlock peer
			  if (pfrom->mapThinBlocksInFlight.size() < 1 && pfrom->ThinBlockCapable()) 
			    { // We can only send one thinblock per peer at a time
			      pfrom->mapThinBlocksInFlight[inv2.hash] = GetTime();
			      inv2.type = MSG_XTHINBLOCK;
			      std::vector<uint256> vOrphanHashes;
			      for (map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi)
				vOrphanHashes.push_back((*mi).first);
			      BuildSeededBloomFilter(filterMemPool, vOrphanHashes);
			      ss << inv2;
			      ss << filterMemPool;
			      pfrom->PushMessage(NetMsgType::GET_XTHIN, ss);
			      MarkBlockAsInFlight(pfrom->GetId(), item.obj.hash, chainParams.GetConsensus());
			      LogPrint("thin", "Requesting Thinblock %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
			    }
			}
		      else 
			{
			  // Try to download a thinblock if possible otherwise just download a regular block
			  if (pfrom->mapThinBlocksInFlight.size() < 1 && pfrom->ThinBlockCapable()) { // We can only send one thinblock per peer at a time
			    pfrom->mapThinBlocksInFlight[inv2.hash] = GetTime();
			    inv2.type = MSG_XTHINBLOCK;
			    std::vector<uint256> vOrphanHashes;
			    for (map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi)
			      vOrphanHashes.push_back((*mi).first);
			    BuildSeededBloomFilter(filterMemPool, vOrphanHashes);
			    ss << inv2;
			    ss << filterMemPool;
			    pfrom->PushMessage(NetMsgType::GET_XTHIN, ss);
			    LogPrint("thin", "Requesting Thinblock %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
			  }
			  else 
			    {
			      LogPrint("thin", "Requesting Regular Block %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
			      //vToFetch.push_back(inv2);
                              inv2.type = MSG_BLOCK;
                              pfrom->PushMessage(NetMsgType::GETDATA, inv2);
			    }
			  MarkBlockAsInFlight(pfrom->GetId(), item.obj.hash, chainParams.GetConsensus());
			}
		    }
		  else 
		    {
		      //vToFetch.push_back(inv2);
                      inv2.type = MSG_BLOCK;
                      pfrom->PushMessage(NetMsgType::GETDATA, inv2);
		      MarkBlockAsInFlight(pfrom->GetId(), item.obj.hash, chainParams.GetConsensus());
		      LogPrint("thin", "Requesting Regular Block %s from peer %s (%d)\n", inv2.hash.ToString(), pfrom->addrName.c_str(),pfrom->id);
		    }
		  // BUIP010 Xtreme Thinblocks: end section
		}
	      LogPrint("net", "getheaders (%d) %s to peer=%d\n", pindexBestHeader->nHeight, item.obj.hash.ToString(), pfrom->id);
	    }
	}    
    }
  

  // while (((lastPass + MIN_REQUEST_RETRY_INTERVAL < now)||(inFlight < maxInFlight + droppedTxns()))&&(sendIter != mapTxnInfo.end()))
  while ((sendIter != mapTxnInfo.end())&&(requestPacer.try_leak(1)))
    {
      OdMap::iterator itemIter = sendIter;
      CUnknownObj& item = itemIter->second;
      ++sendIter;  // move it forward up here in case we need to erase the item we are working with.

      if (now-item.lastRequestTime > MIN_REQUEST_RETRY_INTERVAL)  // if never requested then lastRequestTime==0 so this will always be true
	{
          if (!item.rateLimited)
	    {
	      if (item.lastRequestTime)  // if this is positive, we've requested at least once
		{
		  LogPrint("req", "Request timeout for %s.  Retrying\n",item.obj.ToString().c_str());
		  // Not reducing inFlight; its still outstanding; will be cleaned up when item is removed from map
                  droppedTxns += 1;
		}

              int next = item.NextSource();
	      if (next == CUnknownObj::MAX_AVAIL_FROM)  // I can't get the object from anywhere.  now what?
		{
		  // TODO: tell someone about this issue, look in a random node, or something.
		  cleanup(itemIter);
		}
	      else // Ok request this item.
		{
		  CNode* from = item.availableFrom[next];
                  item.requestCount[next] += 1;
                  LOCK(from->cs_vSend);
		  item.receivingFrom |= (1<<next);
		  item.outstandingReqs++;
		  item.lastRequestTime = now;
		  // from->AskFor(item.obj); basically just shoves the req into mapAskFor
		  from->mapAskFor.insert(std::make_pair(now, item.obj));
		  inFlight++;
                  inFlightTxns << inFlight;
		}
	    }
	}

    }
  //lastPass = now;
  }
}
