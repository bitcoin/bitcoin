#include <mweb/mweb_policy.h>

#include <primitives/transaction.h>

using namespace MWEB;

bool Policy::IsStandardTx(const CTransaction& tx, std::string& reason)
{
    // MWEB: To help avoid mempool bugs, we don't yet allow transaction aggregation
    // for transactions with canonical LTC data.
    if (!tx.IsMWEBOnly()) {
        std::set<mw::Hash> pegin_kernels;
        for (const CTxOut& txout : tx.vout) {
            mw::Hash kernel_id;
            if (txout.scriptPubKey.IsMWEBPegin(&kernel_id)) {
                pegin_kernels.insert(std::move(kernel_id));
            }
        }

        if (pegin_kernels != tx.mweb_tx.GetKernelIDs()) {
            reason = "kernel-mismatch";
            return false;
        }
    }

    // MWEB: Check MWEB transaction for non-standard kernel features
    if (tx.HasMWEBTx() && !tx.mweb_tx.m_transaction->IsStandard()) {
        reason = "non-standard-mweb-tx";
        return false;
    }

    return true;
}