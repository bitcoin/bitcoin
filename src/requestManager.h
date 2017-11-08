// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/* The request manager creates an isolation layer between the bitcoin message processor and the network.
It tracks known locations of data objects and issues requests to the node most likely to respond.  It monitors responses
and is capable of re-requesting the object if the node disconnects or does not respond.

This stops this node from losing transactions if the remote node does not respond (previously, additional INVs would be
dropped because the transaction is "in flight"), yet when the request finally timed out or the connection dropped, the
INVs likely would no longer be propagating throughout the network so this node would miss the transaction.

It should also be possible to use the statistics gathered by the request manager to make unsolicited requests for data
likely held by other nodes, to choose the best node for expedited service, and to minimize data flow over poor
links, such as through the "Great Firewall of China".

This is a singleton class, instantiated as a global named "requester".

The P2P message processing software should no longer directly request data from a node.  Instead call:
requester.AskFor(...)

After the object arrives (its ok to call after ANY object arrives), call "requester.Received(...)" to indicate
successful receipt, "requester.Rejected(...)" to indicate a bad object (request manager will try someone else), or
"requester.AlreadyReceived" to indicate the receipt of an object that has already been received.
 */

#ifndef REQUEST_MANAGER_H
#define REQUEST_MANAGER_H
#include "net.h"
#include "stat.h"
// When should I request a tx from someone else (in microseconds). cmdline/bitcoin.conf: -txretryinterval
extern unsigned int txReqRetryInterval;
extern unsigned int MIN_TX_REQUEST_RETRY_INTERVAL;
static const unsigned int DEFAULT_MIN_TX_REQUEST_RETRY_INTERVAL = 5 * 1000 * 1000;
// When should I request a block from someone else (in microseconds). cmdline/bitcoin.conf: -blkretryinterval
extern unsigned int blkReqRetryInterval;
extern unsigned int MIN_BLK_REQUEST_RETRY_INTERVAL;
static const unsigned int DEFAULT_MIN_BLK_REQUEST_RETRY_INTERVAL = 5 * 1000 * 1000;

class CNode;

class CNodeRequestData
{
public:
    int requestCount;
    int desirability;
    CNode *node;
    CNodeRequestData(CNode *);

    CNodeRequestData() : requestCount(0), desirability(0), node(NULL) {}
    void clear(void)
    {
        requestCount = 0;
        node = 0;
        desirability = 0;
    }
    bool operator<(const CNodeRequestData &rhs) const { return desirability < rhs.desirability; }
};

struct MatchCNodeRequestData // Compare a CNodeRequestData object to a node
{
    CNode *node;
    MatchCNodeRequestData(CNode *n) : node(n){};
    inline bool operator()(const CNodeRequestData &nd) const { return nd.node == node; }
};

class CUnknownObj
{
public:
    typedef std::list<CNodeRequestData> ObjectSourceList;
    CInv obj;
    bool rateLimited;
    int64_t lastRequestTime; // In microseconds, 0 means no request
    unsigned int outstandingReqs;
    // unsigned int receivingFrom;
    // char    requestCount[MAX_AVAIL_FROM];
    // CNode* availableFrom[MAX_AVAIL_FROM];
    ObjectSourceList availableFrom;
    unsigned int priority;

    CUnknownObj()
    {
        rateLimited = false;
        outstandingReqs = 0;
        lastRequestTime = 0;
        priority = 0;
    }

    bool AddSource(CNode *from); // returns true if the source did not already exist
};

class CRequestManager
{
protected:
#ifdef DEBUG
    friend UniValue getstructuresizes(const UniValue &params, bool fHelp);
#endif

    // map of transactions
    typedef std::map<uint256, CUnknownObj> OdMap;
    OdMap mapTxnInfo;
    OdMap mapBlkInfo;
    CCriticalSection cs_objDownloader; // protects mapTxnInfo and mapBlkInfo

    OdMap::iterator sendIter;
    OdMap::iterator sendBlkIter;

    int inFlight;
    // int maxInFlight;
    CStatHistory<int> inFlightTxns;
    CStatHistory<int> receivedTxns;
    CStatHistory<int> rejectedTxns;
    CStatHistory<int> droppedTxns;
    CStatHistory<int> pendingTxns;

    void cleanup(OdMap::iterator &item);
    CLeakyBucket requestPacer;
    CLeakyBucket blockPacer;

public:
    CRequestManager();

    // Get this object from somewhere, asynchronously.
    void AskFor(const CInv &obj, CNode *from, unsigned int priority = 0);

    // Get these objects from somewhere, asynchronously.
    void AskFor(const std::vector<CInv> &objArray, CNode *from, unsigned int priority = 0);

    // Indicate that we got this object, from and bytes are optional (for node performance tracking)
    void Received(const CInv &obj, CNode *from, int bytes = 0);

    // Indicate that we previously got this object
    void AlreadyReceived(const CInv &obj);

    // Indicate that getting this object was rejected
    void Rejected(const CInv &obj, CNode *from, unsigned char reason = 0);

    void SendRequests();

    // Indicates whether a node ping time is acceptable relative to the overall average of all nodes.
    bool IsNodePingAcceptable(CNode *pnode);
};


extern CRequestManager requester; // Singleton class

#endif
