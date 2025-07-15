// Copyright (c) 2019-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INSTANTSEND_LOCK_H
#define BITCOIN_INSTANTSEND_LOCK_H

#include <bls/bls.h>
#include <serialize.h>
#include <uint256.h>

#include <cstdint>
#include <memory>
#include <vector>

class COutPoint;

namespace llmq {
struct CInstantSendLock {
    static constexpr uint8_t CURRENT_VERSION{1};

    uint8_t nVersion{CURRENT_VERSION};
    std::vector<COutPoint> inputs;
    uint256 txid;
    uint256 cycleHash;
    CBLSLazySignature sig;

    CInstantSendLock() = default;

    SERIALIZE_METHODS(CInstantSendLock, obj)
    {
        READWRITE(obj.nVersion);
        READWRITE(obj.inputs);
        READWRITE(obj.txid);
        READWRITE(obj.cycleHash);
        READWRITE(obj.sig);
    }

    uint256 GetRequestId() const;
    bool TriviallyValid() const;
};

using CInstantSendLockPtr = std::shared_ptr<CInstantSendLock>;
} // namespace llmq

#endif // BITCOIN_INSTANTSEND_LOCK_H
