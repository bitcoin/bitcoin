#include "omnicore/pending.h"

#include "omnicore/log.h"
#include "omnicore/omnicore.h"
#include "omnicore/sp.h"
#include "omnicore/walletcache.h"
#include "omnicore/mdex.h"

#include "amount.h"
#include "main.h"
#include "sync.h"
#include "txmempool.h"
#include "uint256.h"
#include "ui_interface.h"

#include <string>

namespace mastercore
{
//! Guards my_pending
CCriticalSection cs_pending;

//! Global map of pending transaction objects
PendingMap my_pending;

/**
 * Adds a transaction to the pending map using supplied parameters.
 */
void PendingAdd(const uint256& txid, const std::string& sendingAddress, uint16_t type, uint32_t propertyId, int64_t amount, bool fSubtract)
{
    if (msc_debug_pending) PrintToLog("%s(%s,%s,%d,%d,%d,%s)\n", __func__, txid.GetHex(), sendingAddress, type, propertyId, amount, fSubtract);

    // bypass tally update for pending transactions, if there the amount should not be subtracted from the balance (e.g. for cancels)
    if (fSubtract) {
        if (!update_tally_map(sendingAddress, propertyId, -amount, PENDING)) {
            PrintToLog("ERROR - Update tally for pending failed! %s(%s,%s,%d,%d,%d,%s)\n", __func__, txid.GetHex(), sendingAddress, type, propertyId, amount, fSubtract);
            return;
        }
    }

    // add pending object
    CMPPending pending;
    pending.src = sendingAddress;
    pending.amount = amount;
    pending.prop = propertyId;
    pending.type = type;
    {
        LOCK(cs_pending);
        my_pending.insert(std::make_pair(txid, pending));
    }
    // after adding a transaction to pending the available balance may now be reduced, refresh wallet totals
    CheckWalletUpdate(true); // force an update since some outbound pending (eg MetaDEx cancel) may not change balances
    uiInterface.OmniPendingChanged(true);
}

/**
 * Deletes a transaction from the pending map and credits the amount back to the pending tally for the address.
 *
 * NOTE: this is currently called for every bitcoin transaction prior to running through the parser.
 */
void PendingDelete(const uint256& txid)
{
    LOCK(cs_pending);

    PendingMap::iterator it = my_pending.find(txid);
    if (it != my_pending.end()) {
        const CMPPending& pending = it->second;
        int64_t src_amount = getMPbalance(pending.src, pending.prop, PENDING);
        if (msc_debug_pending) PrintToLog("%s(%s): amount=%d\n", __FUNCTION__, txid.GetHex(), src_amount);
        if (src_amount) update_tally_map(pending.src, pending.prop, pending.amount, PENDING);
        my_pending.erase(it);

        // if pending map is now empty following deletion, trigger a status change
        if (my_pending.empty()) uiInterface.OmniPendingChanged(false);
    }
}

/**
 * Performs a check to ensure all pending transactions are still in the mempool.
 *
 * NOTE: Transactions no longer in the mempool (eg orphaned) are deleted from
 *       the pending map and credited back to the pending tally.
 */
void PendingCheck()
{
    LOCK(cs_pending);

    std::vector<uint256> vecMemPoolTxids;
    mempool.queryHashes(vecMemPoolTxids);

    for (PendingMap::iterator it = my_pending.begin(); it != my_pending.end(); ++it) {
        const uint256& txid = it->first;
        if (std::find(vecMemPoolTxids.begin(), vecMemPoolTxids.end(), txid) == vecMemPoolTxids.end()) {
            PrintToLog("WARNING: Pending transaction %s is no longer in this nodes mempool and will be discarded\n", txid.GetHex());
            PendingDelete(txid);
        }
    }
}

} // namespace mastercore

/**
 * Prints information about a pending transaction object.
 */
void CMPPending::print(const uint256& txid) const
{
    PrintToConsole("%s : %s %d %d %d %s\n", txid.GetHex(), src, prop, amount, type);
}

