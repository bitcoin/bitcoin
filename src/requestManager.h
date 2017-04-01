// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REQUEST_MANAGER_H
#define REQUEST_MANAGER_H
#include "net.h"
#include "stat.h"
extern unsigned int MIN_TX_REQUEST_RETRY_INTERVAL;  // When should I request a tx from someone else (in microseconds). cmdline/bitcoin.conf: -txretryinterval
extern unsigned int MIN_BLK_REQUEST_RETRY_INTERVAL;  // When should I request a block from someone else (in microseconds). cmdline/bitcoin.conf: -blkretryinterval

class CNodeRequestData
{
public:
  int    requestCount;
  int    desirability;
  CNode* node;
  CNodeRequestData(CNode*);

  CNodeRequestData():requestCount(0), desirability(0), node(NULL)
    {
    }
  void clear(void)
    {
    requestCount=0;
    node=0;
    desirability=0;
    }
  bool operator<(const CNodeRequestData &rhs) const { return desirability < rhs.desirability; }
};

struct IsCNodeRequestDataThisNode // Compare a CNodeRequestData object to a node
{
  CNode* node;
  IsCNodeRequestDataThisNode(CNode* n):node(n) {};
  inline bool operator()(const CNodeRequestData& nd) const { return nd.node == node; }
};

class CUnknownObj
{
public:
  typedef std::list<CNodeRequestData> ObjectSourceList;
  CInv obj;
  bool rateLimited;
  int64_t lastRequestTime;  // In microseconds, 0 means no request
  unsigned int outstandingReqs;
  //unsigned int receivingFrom;
  //char    requestCount[MAX_AVAIL_FROM];
  //CNode* availableFrom[MAX_AVAIL_FROM];
  ObjectSourceList availableFrom;
  int priority;
  
  CUnknownObj()
  {
    rateLimited = false;
    outstandingReqs = 0;
    lastRequestTime = 0;
  }

  void AddSource(CNode* from);
};

class CRequestManager
{
  protected:
  // map of transactions
  typedef std::map<uint256, CUnknownObj> OdMap;
  OdMap mapTxnInfo;
  OdMap mapBlkInfo;
  CCriticalSection cs_objDownloader;  // protects mapTxnInfo and mapBlkInfo

  OdMap::iterator sendIter;
  OdMap::iterator sendBlkIter;

  int inFlight;
  //int maxInFlight;
  CStatHistory<int> inFlightTxns;
  CStatHistory<int> receivedTxns;
  CStatHistory<int> rejectedTxns;
  CStatHistory<int> droppedTxns;
  CStatHistory<int> pendingTxns;
  
  void cleanup(OdMap::iterator& item);
  CLeakyBucket requestPacer;
  CLeakyBucket blockPacer;
  public:
  CRequestManager();

  // Get this object from somewhere, asynchronously.
  void AskFor(const CInv& obj, CNode* from, int priority=0);

  // Get these objects from somewhere, asynchronously.
  void AskFor(const std::vector<CInv>& objArray, CNode* from,int priority=0);

  // Indicate that we got this object, from and bytes are optional (for node performance tracking)
  void Received(const CInv& obj, CNode* from=NULL, int bytes=0);

  // Indicate that we previously got this object
  void AlreadyReceived(const CInv& obj);

  // Indicate that getting this object was rejected
  void Rejected(const CInv& obj, CNode* from=NULL, unsigned char reason=0);
  
  void SendRequests();
  
  // Indicates whether a node ping time is acceptable relative to the overall average of all nodes.
  bool IsNodePingAcceptable(CNode* pnode);

};


extern CRequestManager requester;  // Singleton class

#endif
