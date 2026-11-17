// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_GOVERNANCE_SUPERBLOCK_H
#define BITCOIN_GOVERNANCE_SUPERBLOCK_H

#include <consensus/amount.h>
#include <governance/object.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>
#include <uint256.h>

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

class CChain;
class CDeterministicMNList;
class CTransaction;
class CTxOut;

CAmount ParsePaymentAmount(const std::string& strAmount);

/**
 * Governance Object Payment
 */
class CGovernancePayment
{
private:
    bool fValid;

public:
    CScript script;
    CAmount nAmount;
    uint256 proposalHash;

    CGovernancePayment() :
        fValid(false),
        script(),
        nAmount(0),
        proposalHash(0)
    {
    }

    CGovernancePayment(const CTxDestination& destIn, CAmount nAmountIn, const uint256& proposalHash);

    bool IsValid() const { return fValid; }
};


/**
*  Trigger : Superblock
*
*   - Create payments on the network
*
*   object structure:
*   {
*       "governance_object_id" : last_id,
*       "type" : govtypes.trigger,
*       "subtype" : "superblock",
*       "superblock_name" : superblock_name,
*       "start_epoch" : start_epoch,
*       "payment_addresses" : "addr1|addr2|addr3",
*       "payment_amounts"   : "amount1|amount2|amount3"
*   }
*/
class CSuperblock : public CGovernanceObject
{
private:
    uint256 nGovObjHash;

    int nBlockHeight;
    SeenObjectStatus nStatus;
    std::vector<CGovernancePayment> vecPayments;

    void ParsePaymentSchedule(const std::string& strPaymentAddresses, const std::string& strPaymentAmounts, const std::string& strProposalHashes);

public:
    CSuperblock();
    CSuperblock(int nBlockHeight, std::vector<CGovernancePayment> vecPayments);
    explicit CSuperblock(const CGovernanceObject& obj, uint256& nHash);

    static bool IsValidBlockHeight(int nBlockHeight);
    static void GetNearestSuperblocksHeights(int nBlockHeight, int& nLastSuperblockRet, int& nNextSuperblockRet);
    static CAmount GetPaymentsLimit(const CChain& active_chain, int nBlockHeight);

    SeenObjectStatus GetStatus() const { return nStatus; }
    void SetStatus(SeenObjectStatus nStatusIn) { nStatus = nStatusIn; }

    std::string GetHexStrData() const;

    // TELL THE ENGINE WE EXECUTED THIS EVENT
    void SetExecuted() { nStatus = SeenObjectStatus::Executed; }

    int GetBlockHeight() const
    {
        return nBlockHeight;
    }

    uint256 GetGovernanceObjHash() const { return nGovObjHash; }

    int CountPayments() const { return static_cast<int>(vecPayments.size()); }
    bool GetPayment(int nPaymentIndex, CGovernancePayment& paymentRet);
    CAmount GetPaymentsTotalAmount();

    bool IsValid(const CChain& active_chain, const CTransaction& txNew, int block_height, CAmount blockReward, bool is_v24);
    bool IsExpired(int heightToTest) const;

    std::vector<uint256> GetProposalHashes() const;
};

using CSuperblock_sptr = std::shared_ptr<CSuperblock>;

namespace governance {

/** Estimate when the block at `height` will be mined, measured from a chain point at `ref_height` and `ref_time`. */
int64_t EstimateBlockTime(int height, int ref_height, int64_t ref_time);

struct ProposalWindow {
    int64_t start_epoch;
    int64_t end_epoch;
};

/**
 * Payment window of a proposal paid by `payments` consecutive superblocks starting at `first_superblock`: from half
 * a cycle before the first one to half a cycle after the last one. Masternodes only put a proposal into a superblock
 * whose estimated time lies inside this window.
 */
ProposalWindow EstimateProposalWindow(int first_superblock, int payments, int superblock_cycle, int ref_height,
                                      int64_t ref_time);

/**
 * Owns the set of active superblock triggers. Trigger lifecycle is fully
 * owned here; CGovernanceManager pushes new TRIGGER objects in via
 * AddTrigger() and notifies us via RemoveTrigger() when it erases the
 * underlying object from its store. Each entry holds a strong reference
 * to the underlying CGovernanceObject so we never look it up by hash.
 *
 * Lifetime: typically owned by CChainstateHelper, which outlives the
 * paired CGovernanceManager. The govman drives m_loaded via SetLoaded()
 * from LoadCache() and Clear()s us in its destructor, so IsValid() and
 * the trigger map track the live govman's state.
 */
class SuperblockManager
{
public:
    bool IsValid() const { return m_loaded; }
    void SetLoaded(bool loaded) { m_loaded = loaded; }

    /** Register a TRIGGER governance object. Returns true if the resulting
     *  trigger is live (not already height-expired). */
    bool AddTrigger(std::shared_ptr<CGovernanceObject> obj, int cachedHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    /** Single-direction notification from CGovernanceManager when it erases
     *  a TRIGGER object from its store. */
    void RemoveTrigger(const uint256& hash) EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    /** Prune triggers that are invalid or height-expired. Calls
     *  PrepareDeletion() on the underlying object so CGovernanceManager
     *  picks it up on its own sweep. */
    void Clean(int cachedHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    std::vector<CSuperblock_sptr> GetActiveTriggers() const EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    bool GetBestSuperblock(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& sbRet, int nBlockHeight) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    bool IsSuperblockTriggered(const CDeterministicMNList& tip_mn_list, int nBlockHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    bool IsValidSuperblock(const CChain& active_chain, const CDeterministicMNList& tip_mn_list, const CTransaction& txNew,
                           int nBlockHeight, CAmount blockReward, bool is_v24) const EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    bool GetSuperblockPayments(const CDeterministicMNList& tip_mn_list, int nBlockHeight,
                               std::vector<CTxOut>& voutSuperblockRet) const EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

    void ExecuteBestSuperblock(const CDeterministicMNList& tip_mn_list, int nBlockHeight) EXCLUSIVE_LOCKS_REQUIRED(!cs_sb);

private:
    struct TriggerEntry {
        CSuperblock_sptr sb;
        std::shared_ptr<CGovernanceObject> obj;
    };

    bool GetBestSuperblockInternal(const CDeterministicMNList& tip_mn_list, CSuperblock_sptr& sbRet,
                                   int nBlockHeight) const EXCLUSIVE_LOCKS_REQUIRED(cs_sb);

    mutable Mutex cs_sb;
    std::atomic<bool> m_loaded{false};
    std::map<uint256, TriggerEntry> m_triggers GUARDED_BY(cs_sb);
};

} // namespace governance

#endif // BITCOIN_GOVERNANCE_SUPERBLOCK_H
