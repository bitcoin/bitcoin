// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_EHF_SIGNALS_H
#define BITCOIN_LLMQ_EHF_SIGNALS_H

#include <llmq/signing.h>

#include <set>

class CBlockIndex;
class CChainState;
class CConnman;
class CSporkManager;
class CTxMemPool;

namespace llmq
{
class CQuorumManager;
class CSigSharesManager;
class CSigningManager;

class CEHFSignalsHandler : public CRecoveredSigsListener
{
private:
    CChainState& chainstate;
    CConnman& connman;
    CSigningManager& sigman;
    CSigSharesManager& shareman;
    const CSporkManager& sporkman;
    const CQuorumManager& qman;
    CTxMemPool& mempool;

    /**
     * keep freshly generated IDs for easier filter sigs in HandleNewRecoveredSig
     */
    mutable Mutex cs;
    std::set<uint256> ids GUARDED_BY(cs);
public:
    explicit CEHFSignalsHandler(CChainState& chainstate, CConnman& connman,
                                CSigningManager& sigman, CSigSharesManager& shareman,
                                const CSporkManager& sporkman, const CQuorumManager& qman, CTxMemPool& mempool);
    ~CEHFSignalsHandler();


    /**
     * Since Tip is updated it could be a time to generate EHF Signal
     */
    void UpdatedBlockTip(const CBlockIndex* const pindexNew);

    void HandleNewRecoveredSig(const CRecoveredSig& recoveredSig) override LOCKS_EXCLUDED(cs);

private:
    void trySignEHFSignal(int bit, const CBlockIndex* const pindex) LOCKS_EXCLUDED(cs);

};

} // namespace llmq

#endif // BITCOIN_LLMQ_EHF_SIGNALS_H
