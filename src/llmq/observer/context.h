// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LLMQ_OBSERVER_CONTEXT_H
#define BITCOIN_LLMQ_OBSERVER_CONTEXT_H

#include <validationinterface.h>

#include <memory>

class CBlockIndex;

namespace llmq {
struct ObserverContext final : public CValidationInterface {
    ObserverContext(const ObserverContext&) = delete;
    ObserverContext& operator=(const ObserverContext&) = delete;
    ObserverContext();
    ~ObserverContext();

protected:
    // CValidationInterface
    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;
};
} // namespace llmq

#endif // BITCOIN_LLMQ_OBSERVER_CONTEXT_H
