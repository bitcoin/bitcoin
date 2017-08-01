// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODESTATE_H
#define BITCOIN_NODESTATE_H

#include "net.h" // For NodeId

/** Blocks that are in flight, and that are in the queue to be downloaded. Protected by cs_main. */
struct QueuedBlock
{
    uint256 hash;
    CBlockIndex *pindex; //!< Optional.
    int64_t nTime; //! Time of "getdata" request in microseconds.
    bool fValidatedHeaders; //!< Whether this block has validated headers at the time of request.
};

extern std::map<uint256, std::pair<NodeId, std::list<QueuedBlock>::iterator> > mapBlocksInFlight;

/**
* Maintain validation-specific state about nodes, protected by cs_main, instead
* by CNode's own locks. This simplifies asynchronous operation, where
* processing of incoming data is done after the ProcessMessage call returns,
* and we're no longer holding the node's locks.
*/
struct CNodeState
{
    //! The peer's address
    CService address;
    //! String name of this peer (debugging/logging purposes).
    std::string name;
    //! The best known block we know this peer has announced.
    CBlockIndex *pindexBestKnownBlock;
    //! The hash of the last unknown block this peer has announced.
    uint256 hashLastUnknownBlock;
    //! The last full block we both have.
    CBlockIndex *pindexLastCommonBlock;
    //! The best header we have sent our peer.
    CBlockIndex *pindexBestHeaderSent;
    //! Whether we've started headers synchronization with this peer.
    bool fSyncStarted;
    //! The start time of the sync
    int64_t fSyncStartTime;
    //! Were the first headers requested in a sync received
    bool fFirstHeadersReceived;
    //! Our current block height at the time we requested GETHEADERS
    int nFirstHeadersExpectedHeight;

    std::list<QueuedBlock> vBlocksInFlight;
    //! When the first entry in vBlocksInFlight started downloading. Don't care when vBlocksInFlight is empty.
    int64_t nDownloadingSince;
    int nBlocksInFlight;
    int nBlocksInFlightValidHeaders;
    //! Whether we consider this a preferred download peer.
    bool fPreferredDownload;
    //! Whether this peer wants invs or headers (when possible) for block announcements.
    bool fPreferHeaders;

    CNodeState();
};

/** Map maintaining per-node state. Requires cs_main. */
extern std::map<NodeId, CNodeState> mapNodeState;

// Requires cs_main.
extern CNodeState *State(NodeId pnode);

#endif // BITCOIN_NODESTATE_H
