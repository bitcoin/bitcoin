// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txreconciliation.h>

std::tuple<bool, bool, uint32_t, uint64_t> TxReconciliationTracker::SuggestReconciling(const NodeId peer_id, bool inbound)
{
    bool be_recon_requestor, be_recon_responder;
    // Currently reconciliation requests flow only in one direction inbound->outbound.
    if (inbound) {
        be_recon_requestor = false;
        be_recon_responder = true;
    } else {
        be_recon_requestor = true;
        be_recon_responder = false;
    }

    uint32_t recon_version = 1;
    uint64_t m_local_recon_salt(GetRand(UINT64_MAX));
    WITH_LOCK(m_local_salts_mutex, m_local_salts.emplace(peer_id, m_local_recon_salt));

    return std::make_tuple(be_recon_requestor, be_recon_responder, recon_version, m_local_recon_salt);
}
