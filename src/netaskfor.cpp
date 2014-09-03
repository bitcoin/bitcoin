// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include "config/bitcoin-config.h"
#endif

#include "netaskfor.h"

#include <boost/thread.hpp>

#include "net.h"
#include "util.h"

/**
 * Inventory item request management.
 *
 * Maintains state separate from CNode.
 * There is a two-way mapping between CNodeAskForState and mapInvRequests.
 */
namespace {

/** Node specific state for netaskfor module */
class CNodeAskForState
{
public:
    CNodeAskForState(): node(0) {}

    /** Set of inv items that are being requested from this node */
    std::set<CInv> setAskFor;

    /** Group up to 1000 outgoing requests per node */
    std::vector<CInv> outgoingRequests;

    /** Network connection associated with this node */
    CNode *node; /// TODO ugly but needed for sending getdata
};

typedef std::multimap<int64_t, CInv> InvRequestsWorkQueue;

/** State of one inventory item request.
 * @note in this structure we store NodeIds instead of CNodeAskForState*, as this
 * would double memory usage on 64-bit (on the other hand, that would reduce the number of map
 * lookups needed...)
 */
class CInvState
{
public:
    typedef std::set<NodeId> NodeSet;
    /** IDs of nodes that have this item */
    NodeSet nodes;
    /** IDs of nodes that we have not tried requesting this inv from already */
    NodeSet notRequestedFrom;
    /** Current node that this is being requested from */
    NodeId beingRequestedFrom;
    /** Must correspond to current entry in work queue for this
     *  inventory item (or invRequestsWorkQueue.end())
     */
    InvRequestsWorkQueue::iterator workQueueIter;
};
typedef std::map<CInv, CInvState> MapInvRequests;

/** Local data, all protected by cs_invRequests */
CCriticalSection cs_invRequests;
/** Map node id to local state */
std::map<NodeId, CNodeAskForState> mapNodeAskForState;
/** Map inv to current request state */
MapInvRequests mapInvRequests;
/** The work queue keeps track of when is the next time a request needs
 * to be revisited.
 * @invariant Each request has at most one entry.
 * @note Theoretically this could be a deque instead of a map, as we only add to
 * the end (assuming monotonic time) and to the beginning (first tries have
 * t=0), and remove from the beginning. However std::deque invalidates
 * iterators so we cannot keep around an iterator to remove the work queue item
 * on cancel.
 */
InvRequestsWorkQueue invRequestsWorkQueue;
CTimeoutCondition condInvRequests;
/** Exit thread flag */
bool fStopThread;

/** Get local state for a node id.
 * @note Requires that cs_invRequests is held
 */
CNodeAskForState *State(NodeId pnode)
{
    std::map<NodeId, CNodeAskForState>::iterator it = mapNodeAskForState.find(pnode);
    if (it == mapNodeAskForState.end())
        return NULL;
    return &it->second;
}

/** Handler for when a new node appears */
void InitializeNode(NodeId nodeid, const CNode *pnode)
{
    LOCK(cs_invRequests);
    mapNodeAskForState.insert(std::make_pair(nodeid, CNodeAskForState()));
}

/** Handler to clean up when a node goes away */
void FinalizeNode(NodeId nodeid)
{
    LOCK(cs_invRequests);
    CNodeAskForState *state = State(nodeid);
    assert(state);

    /// Clean up any requests that were underway to the node,
    /// or refer to the node.
    BOOST_FOREACH(const CInv &inv, state->setAskFor)
    {
        MapInvRequests::iterator i = mapInvRequests.find(inv);
        if (i != mapInvRequests.end())
        {
            i->second.nodes.erase(nodeid);
            i->second.notRequestedFrom.erase(nodeid);

            if (i->second.beingRequestedFrom == nodeid)
            {
                LogPrint("netaskfor", "%s: Inv item %s was being requested from destructing node %i\n",
                        __func__,
                        inv.ToString(), nodeid);
                i->second.beingRequestedFrom = 0;
                /// Make sure the old workqueue item for the inv is removed,
                /// to avoid spurious retries
                if (i->second.workQueueIter != invRequestsWorkQueue.end())
                    invRequestsWorkQueue.erase(i->second.workQueueIter);
                /// Re-trigger request logic
                i->second.workQueueIter = invRequestsWorkQueue.insert(std::make_pair(0, inv));
                condInvRequests.notify_one();
            }
        }
    }
    mapNodeAskForState.erase(nodeid);
}

/** Forget a certain inventory item request
 * @note requires that cs_invRequests lock is held.
 */
void Forget(MapInvRequests::iterator i)
{
    /// Remove reference to this inventory item request from nodes
    BOOST_FOREACH(NodeId nodeid, i->second.nodes)
    {
        CNodeAskForState *state = State(nodeid);
        assert(state);
        state->setAskFor.erase(i->first);
    }
    /// Remove from workqueue
    if (i->second.workQueueIter != invRequestsWorkQueue.end())
        invRequestsWorkQueue.erase(i->second.workQueueIter);
    /// Remove from map
    mapInvRequests.erase(i);
}

/** Flushes queued getdatas to a node by id.
 * @note requires that cs_invRequests lock is held.
 */
void FlushGetdata(NodeId nodeid)
{
    CNodeAskForState *state = State(nodeid);
    assert(state && state->node);
    CNode *node = state->node;

    /// Don't send empty getdatas, ever
    if (state->outgoingRequests.empty())
        return;

    std::stringstream debug;
    BOOST_FOREACH(const CInv &inv, state->outgoingRequests)
        debug << inv.ToString() << " ";
    LogPrint("netaskfor", "%s: peer=%i getdata %s\n", __func__, nodeid, debug.str());

    node->PushMessage("getdata", state->outgoingRequests);
    state->outgoingRequests.clear();
}

/** Queues a getdata to a node by id
 * @note requires that cs_invRequests lock is held.
 */
void QueueGetdata(NodeId nodeid, const CInv &inv, bool isRetry)
{
    CNodeAskForState *state = State(nodeid);
    assert(state);

    LogPrint("netaskfor", "%s: Requesting item %s from node %i (%s)\n", __func__,
            inv.ToString(), nodeid,
            isRetry ? "retry" : "first request");

    state->outgoingRequests.push_back(inv);

    /// Make sure a getdata request doesn't contain more than 1000 entries
    /// Flush prematurely if this is going to be the case.
    if(state->outgoingRequests.size() >= 1000)
        FlushGetdata(nodeid);
}

void ThreadHandleAskFor()
{
    while (!fStopThread)
    {
        LogPrint("netaskfor", "%s: iteration\n", __func__);
        int64_t timeToNext = std::numeric_limits<int64_t>::max();

        {
            LOCK(cs_invRequests);
            int64_t now = GetTimeMillis();
            std::set<NodeId> nodesToFlush;
            /// Process work queue entries that are timestamped either now or before now
            while (!invRequestsWorkQueue.empty() && invRequestsWorkQueue.begin()->first <= now)
            {
                const CInv &inv = invRequestsWorkQueue.begin()->second;
                MapInvRequests::iterator it = mapInvRequests.find(inv);
                LogPrint("netaskfor", "%s: processing item %s\n", __func__, inv.ToString());
                if (it != mapInvRequests.end())
                {
                    CInvState &invstate = it->second;
                    invstate.workQueueIter = invRequestsWorkQueue.end();
                    /// Pick a node to request from, if available
                    ///
                    /// Currently this picks the node with the lowest node id
                    /// (notRequestedFrom is an ordered set) that offers the item
                    /// that we haven't tried yet. This is usually the
                    /// longest-connected node that we know to have the data.
                    if (invstate.notRequestedFrom.empty())
                    {
                        LogPrint("netaskfor", "%s: No more nodes to request item %s from, discarding request\n", __func__, inv.ToString());
                        Forget(it);
                    } else {
                        CInvState::NodeSet::iterator first = invstate.notRequestedFrom.begin();
                        NodeId nodeid = *first;
                        invstate.notRequestedFrom.erase(first);
                        invstate.beingRequestedFrom = nodeid;

                        QueueGetdata(nodeid, inv, invRequestsWorkQueue.begin()->first != 0);
                        nodesToFlush.insert(nodeid);

                        /// Need to revisit this request after timeout
                        invstate.workQueueIter = invRequestsWorkQueue.insert(std::make_pair(now + NetAskFor::REQUEST_TIMEOUT, inv));
                    }
                } else {
                    LogPrint("netaskfor", "%s: request for item %s is missing!\n", __func__, inv.ToString());
                }
                invRequestsWorkQueue.erase(invRequestsWorkQueue.begin());
            }

            /// Send remaining outgoing requests to nodes
            /// Not doing this after every iteration could save a few bytes at the
            /// expense of delaying getdatas for longer (would need to be taken into
            /// account in the timeout logic!).
            BOOST_FOREACH(NodeId nodeid, nodesToFlush)
                FlushGetdata(nodeid);

            /// Compute time to next event
            if (!invRequestsWorkQueue.empty())
                timeToNext = invRequestsWorkQueue.begin()->first - GetTimeMillis();
        }


        /// If we don't know how long until next work item, wait until woken up
        if (timeToNext == std::numeric_limits<int64_t>::max())
        {
            LogPrint("netaskfor", "%s: blocking\n", __func__);
            condInvRequests.wait();
        } else if (timeToNext > 0)
        {
            LogPrint("netaskfor", "%s: waiting for %d ms\n", __func__, timeToNext);
            condInvRequests.timed_wait(timeToNext);
        }
    }
}

void StartThreads(boost::thread_group& threadGroup)
{
    fStopThread = false;
    /// Inventory management thread
    threadGroup.create_thread(boost::bind(&TraceThread<void (*)()>, "askfor", &ThreadHandleAskFor));
}

void StopThreads()
{
    fStopThread = true;
    condInvRequests.notify_one();
}

}

namespace NetAskFor
{

void Completed(CNode *node, const CInv& inv)
{
    /// As soon as this is possible this should be subscribed to the 'tx' P2P
    /// message directly, instead of rely on being called from main.
    LOCK(cs_invRequests);
    MapInvRequests::iterator i = mapInvRequests.find(inv);
    if (i != mapInvRequests.end())
    {
        LogPrint("netaskfor", "%s: %s peer=%i\n", __func__, inv.ToString(), node->GetId());
        Forget(i);
    } else {
        /// This can happen if a node sends a transaction without unannouncing it with 'inv'
        /// first, or then we retry a request, which completes (and thus forget about), and then
        /// the original node comes back and sends our requested data anyway.
        LogPrint("netaskfor", "%s: %s not found! peer=%i\n", __func__, inv.ToString(), node->GetId());
    }
}

void AskFor(CNode *node, const CInv& inv)
{
    LOCK(cs_invRequests);
    NodeId nodeid = node->GetId();
    CNodeAskForState *state = State(nodeid);
    assert(state);
    state->node = node;

    /// Bound number of concurrent inventory requests to each node, this has
    /// the indirect effect of bounding all data structures.
    if (state->setAskFor.size() > MAX_SETASKFOR_SZ)
        return;

    LogPrint("netaskfor", "askfor %s  peer=%d\n", inv.ToString(), nodeid);

    MapInvRequests::iterator it = mapInvRequests.find(inv);
    if (it == mapInvRequests.end())
    {
        std::pair<MapInvRequests::iterator, bool> ins = mapInvRequests.insert(std::make_pair(inv, CInvState()));
        it = ins.first;
        /// As this is the first time that this item gets announced by anyone, add it to the work queue immediately
        it->second.workQueueIter = invRequestsWorkQueue.insert(std::make_pair(0, inv));
        condInvRequests.notify_one();
    }
    std::pair<CInvState::NodeSet::iterator, bool> ins2 = it->second.nodes.insert(nodeid);
    if (ins2.second)
    {
        /// If this is the first time this node announces the inv item, add it to the set of untried nodes
        /// for the item.
        it->second.notRequestedFrom.insert(nodeid);
    }
    state->setAskFor.insert(inv);
}

void RegisterNodeSignals(CNodeSignals& nodeSignals)
{
    nodeSignals.InitializeNode.connect(&InitializeNode);
    nodeSignals.FinalizeNode.connect(&FinalizeNode);
    nodeSignals.StartThreads.connect(&StartThreads);
    nodeSignals.StopThreads.connect(&StopThreads);
}

void UnregisterNodeSignals(CNodeSignals& nodeSignals)
{
    nodeSignals.InitializeNode.disconnect(&InitializeNode);
    nodeSignals.FinalizeNode.disconnect(&FinalizeNode);
    nodeSignals.StartThreads.disconnect(&StartThreads);
    nodeSignals.StopThreads.disconnect(&StopThreads);
}

};
