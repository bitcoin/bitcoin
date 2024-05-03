// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_EHF_SIGNALS_H
#define BITCOIN_LLMQ_EHF_SIGNALS_H

#include <llmq/signing.h>

#include <set>

class CBlockIndex;
class CChainState;
class CMNHFManager;
class CSporkManager;
class CTxMemPool;
class PeerManager;

namespace llmq
{
class CQuorumManager;
class CSigSharesManager;
class CSigningManager;

class CEHFSignalsHandler : public CRecoveredSigsListener
{
private:
    CChainState& chainstate;
    CMNHFManager& mnhfman;
    CSigningManager& sigman;
    CSigSharesManager& shareman;
    CTxMemPool& mempool;
    const CQuorumManager& qman;
    const CSporkManager& sporkman;
    const std::unique_ptr<PeerManager>& m_peerman;

    /**
     * keep freshly generated IDs for easier filter sigs in HandleNewRecoveredSig
     */
    mutable Mutex cs;
    std::set<uint256> ids GUARDED_BY(cs);
public:
    explicit CEHFSignalsHandler(CChainState& chainstate, CMNHFManager& mnhfman, CSigningManager& sigman,
                                CSigSharesManager& shareman, CTxMemPool& mempool, const CQuorumManager& qman,
                                const CSporkManager& sporkman, const std::unique_ptr<PeerManager>& peerman);
    ~CEHFSignalsHandler();


    /**
     * Since Tip is updated it could be a time to generate EHF Signal
     */
    void UpdatedBlockTip(const CBlockIndex* const pindexNew, bool is_masternode) EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override EXCLUSIVE_LOCKS_REQUIRED(!cs);

private:
    void trySignEHFSignal(int bit, const CBlockIndex* const pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs);

};

} // namespace llmq

#endif // BITCOIN_LLMQ_EHF_SIGNALS_H
