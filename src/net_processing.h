// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Talkcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TALKCOIN_NET_PROCESSING_H
#define TALKCOIN_NET_PROCESSING_H

#include <net.h>
#include <validationinterface.h>
#include <consensus/params.h>
#include <consensus/consensus.h>
#include <sync.h>

class CChainParams;

extern CCriticalSection cs_main;

/** Default for -maxorphantx, maximum number of orphan transactions kept in memory */
static const unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS = 100;
/** Default number of orphan+recently-replaced txn to keep around for block reconstruction */
static const unsigned int DEFAULT_BLOCK_RECONSTRUCTION_EXTRA_TXN = 100;
/** Default for BIP61 (sending reject messages) */
static constexpr bool DEFAULT_ENABLE_BIP61{false};
static const bool DEFAULT_PEERBLOOMFILTERS = false;

/** if disabled, blocks will not be requested automatically, usefull for non-validation mode */
static const bool DEFAULT_AUTOMATIC_BLOCK_REQUESTS = true;
extern std::atomic<bool> fAutoRequestBlocks;

static const bool DEFAULT_FETCH_BLOCKS_WHILE_FETCH_HEADERS = true;
extern std::atomic<bool> fFetchBlocksWhileFetchingHeaders;

static const bool DEFAULT_HEADER_SPAM_FILTER = true;

static const unsigned int DEFAULT_MAX_ORPHAN_BLOCKS = 40;

static const unsigned int DEFAULT_HEADER_SPAM_FILTER_MAX_SIZE = COINBASE_MATURITY;
/** Default for -headerspamfiltermaxavg, maximum average size of an index occurrence in the header spam filter */
static const unsigned int DEFAULT_HEADER_SPAM_FILTER_MAX_AVG = 10;
/** Default for -headerspamfilterignoreport, ignore the port in the ip address when looking for header spam,
 multiple nodes on the same ip will be treated as the one when computing the filter*/
static const unsigned int DEFAULT_HEADER_SPAM_FILTER_IGNORE_PORT = true;

class PeerLogicValidation final : public CValidationInterface, public NetEventsInterface {
private:
    CConnman* const connman;
    BanMan* const m_banman;

    bool SendRejectsAndCheckIfBanned(CNode* pnode, bool enable_bip61) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
public:
    PeerLogicValidation(CConnman* connman, BanMan* banman, CScheduler &scheduler, bool enable_bip61);

    /**
     * Overridden from CValidationInterface.
     */
    void BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexConnected, const std::vector<CTransactionRef>& vtxConflicted) override;
    /**
     * Overridden from CValidationInterface.
     */
    void UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) override;
    /**
     * Overridden from CValidationInterface.
     */
    void BlockChecked(const CBlock& block, const CValidationState& state) override;
    /**
     * Overridden from CValidationInterface.
     */
    void NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock>& pblock) override;

    /** Initialize a peer by adding it to mapNodeState and pushing a message requesting its version */
    void InitializeNode(CNode* pnode) override;
    /** Handle removal of a peer by updating various state and removing it from mapNodeState */
    void FinalizeNode(NodeId nodeid, bool& fUpdateConnectionTime) override;
    /**
    * Process protocol messages received from a given node
    *
    * @param[in]   pfrom           The node which we have received messages from.
    * @param[in]   interrupt       Interrupt condition for processing threads
    */
    bool ProcessMessages(CNode* pfrom, std::atomic<bool>& interrupt) override;
    /**
    * Send queued protocol messages to be sent to a give node.
    *
    * @param[in]   pto             The node which we are sending messages to.
    * @return                      True if there is more work to be done
    */
    bool SendMessages(CNode* pto) override EXCLUSIVE_LOCKS_REQUIRED(pto->cs_sendProcessing);

    /** Consider evicting an outbound peer based on the amount of time they've been behind our tip */
    void ConsiderEviction(CNode *pto, int64_t time_in_seconds) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    /** Evict extra outbound peers. If we think our tip may be stale, connect to an extra outbound */
    void CheckForStaleTipAndEvictPeers(const Consensus::Params &consensusParams);
    /** If we have extra outbound peers, try to disconnect the one with the oldest block announcement */
    void EvictExtraOutboundPeers(int64_t time_in_seconds) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

private:
    int64_t m_stale_tip_check_time; //!< Next time to check for stale tip

    /** Enable BIP61 (sending reject messages) */
    const bool m_enable_bip61;
};

struct CNodeStateStats {
    int nMisbehavior = 0;
    int nSyncHeight = -1;
    int nCommonHeight = -1;
    std::vector<int> vHeightInFlight;
};

/** Get statistics from node state */
bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats);
/** Process network block received from a given node */
/**
 * Prioritize a block for downloading
 * Blocks requested with priority will be downloaded and processed first
 * Downloaded blocks will not trigger ActivateBestChain
 */
void AddPriorityDownload(const std::vector<const CBlockIndex*>& blocksToDownload);
void ProcessPriorityRequests(const std::shared_ptr<CBlock> block);
bool FlushPriorityDownloads();
size_t CountPriorityDownloads();

void SetAutoRequestBlocks(bool state);
bool isAutoRequestingBlocks();

/** Process network block received from a given node */
bool ProcessNetBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool* fNewBlock, CNode* pfrom, CConnman& connman, bool fPriorityRequest = false);

void RelayTransaction(const uint256&, const CConnman& connman);
/** Increase a node's misbehavior score. */
void Misbehaving(NodeId nodeid, int howmuch, const std::string& message="") EXCLUSIVE_LOCKS_REQUIRED(cs_main);
#endif // TALKCOIN_NET_PROCESSING_H
