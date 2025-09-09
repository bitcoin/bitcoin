// Copyright (c) 2023-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_EHF_SIGNALS_H
#define BITCOIN_LLMQ_EHF_SIGNALS_H

#include <llmq/signing.h>

#include <set>

class CBlockIndex;
class ChainstateManager;
class CMNHFManager;

namespace llmq {
class CQuorumManager;
class CSigSharesManager;
class CSigningManager;

class CEHFSignalsHandler : public CRecoveredSigsListener
{
private:
    ChainstateManager& m_chainman;
    CMNHFManager& mnhfman;
    CSigningManager& sigman;
    CSigSharesManager& shareman;
    const CQuorumManager& qman;

    /**
     * keep freshly generated IDs for easier filter sigs in HandleNewRecoveredSig
     */
    mutable Mutex cs;
    std::set<uint256> ids GUARDED_BY(cs);
public:
    explicit CEHFSignalsHandler(ChainstateManager& chainman, CMNHFManager& mnhfman, CSigningManager& sigman,
                                CSigSharesManager& shareman, const CQuorumManager& qman);

    ~CEHFSignalsHandler();

    /**
     * Since Tip is updated it could be a time to generate EHF Signal
     */
    void UpdatedBlockTip(const CBlockIndex* const pindexNew) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] MessageProcessingResult HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    void trySignEHFSignal(int bit, const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);
};
} // namespace llmq

#endif // BITCOIN_LLMQ_EHF_SIGNALS_H
