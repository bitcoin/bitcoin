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

unsigned int MIN_REQUEST_RETRY_INTERVAL = 2*1000*1000;  // When should I request an object from someone else (5 seconds in microseconds)


CRequestManager::CRequestManager(): inFlightTxns("reqMgr/inFlight",STAT_OP_MAX),receivedTxns("reqMgr/received"),rejectedTxns("reqMgr/rejected"),droppedTxns("reqMgr/dropped", STAT_KEEP),pendingTxns("reqMgr/pending", STAT_KEEP)
{
  inFlight=0;
  maxInFlight = 256;
  sendIter = mapTxnInfo.end();
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

  if (sendIter == itemIt) ++sendIter;
  mapTxnInfo.erase(itemIt);
}

// Get this object from somewhere, asynchronously.
void CRequestManager::AskFor(const CInv& obj, CNode* from, int priority)
{
  LogPrint("req", "ReqMgr: Ask for %s.\n",obj.ToString().c_str());

  LOCK(cs_objDownloader);
  if (obj.type == MSG_TX)
    {
      uint256 temp = obj.hash;
      OdMap::value_type v(temp,CUnknownObj());
      std::pair<OdMap::iterator,bool> result = mapTxnInfo.insert(v);
      OdMap::iterator& item = result.first;
      CUnknownObj& data = item->second;
      if (result.second)  // inserted
	{
	  pendingTxns+=1;
	  data.obj = obj;
	  // all other fields are zeroed on creation
	}
      else  // existing
	{
	}

      data.priority = max(priority,data.priority);
      // Got the data, now add the node as a source
      for (int i=0;i<CUnknownObj::MAX_AVAIL_FROM;i++)
	{
	  if (data.availableFrom[i] == from) break;  // don't add the same node twice
	  if (data.availableFrom[i] == NULL)
	  {
	    if (1)
	    {
              LOCK(cs_vNodes);
	      data.availableFrom[i] = from->AddRef();
	    }
            LogPrint("req", "ReqMgr: %s added ref to %d count %d.\n",obj.ToString(), from->GetId(), from->GetRefCount());
	    // I Don't need to clear this bit in receivingFrom, it should already be 0
            if (i==0) // First request for this object
	      {
		from->firstTx += 1;
	      }
	    break;
	  }
	}
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
  LOCK(cs_objDownloader);
  OdMap::iterator item = mapTxnInfo.find(obj.hash);
  if (item ==  mapTxnInfo.end()) return;  // item has already been removed
  LogPrint("req", "ReqMgr: Request received for %s.\n",item->second.obj.ToString().c_str());
  int64_t now = GetTimeMicros();
  from->txReqLatency << (now - item->second.lastRequestTime);  // keep track of response latency of this node
  // will be decremented in the item cleanup: if (inFlight) inFlight--;
  cleanup(item); // remove the item
  receivedTxns += 1;
}

// Indicate that we got this object, from and bytes are optional (for node performance tracking)
void CRequestManager::AlreadyReceived(const CInv& obj)
{
  LOCK(cs_objDownloader);
  OdMap::iterator item = mapTxnInfo.find(obj.hash);
  if (item ==  mapTxnInfo.end()) return;  // item has already been removed
  LogPrint("req", "ReqMgr: Request already received for %s.\n",item->second.obj.ToString().c_str());
  // will be decremented in the item cleanup: if (inFlight) inFlight--;
  cleanup(item); // remove the item
}

// Indicate that we got this object, from and bytes are optional (for node performance tracking)
void CRequestManager::Rejected(const CInv& obj, CNode* from, unsigned char reason)
{
  LOCK(cs_objDownloader);
  OdMap::iterator item = mapTxnInfo.find(obj.hash);
  if (item ==  mapTxnInfo.end()) 
    {
    LogPrint("req", "ReqMgr: Unknown object rejected %s.\n",obj.ToString().c_str());
    return;  // item has already been removed
    }
  if (inFlight) inFlight--;
  if (item->second.outstandingReqs) item->second.outstandingReqs--;

  LogPrint("req", "ReqMgr: Request rejected for %s.\n",item->second.obj.ToString().c_str());
  rejectedTxns += 1;
	
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


void CRequestManager::SendRequests()
{
  LOCK(cs_main);  // Must be locked here due to deadlock interaction with cs_objDownloader and  from->cs_vSend
  {
  LOCK(cs_objDownloader);
  if (sendIter == mapTxnInfo.end()) sendIter = mapTxnInfo.begin();
  int64_t now = GetTimeMicros();
  static int64_t lastPass=0;
 
  // TODO: if a node goes offline, rerequest txns from someone else and cleanup references right away

  while (((lastPass + MIN_REQUEST_RETRY_INTERVAL < now)||(inFlight < maxInFlight + droppedTxns()))&&(sendIter != mapTxnInfo.end()))
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
	      int next = item.outstandingReqs%CUnknownObj::MAX_AVAIL_FROM;
	      while (next < CUnknownObj::MAX_AVAIL_FROM)  // Go from last point forward
		{
                  CNode* from = item.availableFrom[next];
                  if (from)
		    {
		      if (from->fDisconnect)  // oops source is gone!
			{
			  item.availableFrom[next] = NULL;
                          from->Release();
			}
                      else break;
		    }
		  next++;
		}
	      if (next == CUnknownObj::MAX_AVAIL_FROM)
		for (next=0;next<CUnknownObj::MAX_AVAIL_FROM;next++)  // Go from the beginning to the last point
		  {
                  CNode* from = item.availableFrom[next];
                  if (from)
		    {
		      if (from->fDisconnect)  // oops source is gone!
			{
			  item.availableFrom[next] = NULL;
                          from->Release();
			}
                      else break;
		    }
		  }
	      if (next == CUnknownObj::MAX_AVAIL_FROM)  // I can't get the object from anywhere.  now what?
		{
		  // TODO: tell someone about this issue, look in a random node, or something.
		  cleanup(itemIter);
		}
	      else // Ok request this item.
		{
		  CNode* from = item.availableFrom[next];
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
  lastPass = now;
  }
}
