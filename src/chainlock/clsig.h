// Copyright (c) 2019-2022 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CHAINLOCK_CLSIG_H
#define BITCOIN_CHAINLOCK_CLSIG_H

#include <serialize.h>
#include <uint256.h>

#include <bls/bls.h>

#include <cstdint>

namespace chainlock {
extern const std::string CLSIG_REQUESTID_PREFIX;

struct ChainLockSig {
private:
    int32_t nHeight{-1};
    uint256 blockHash;
    CBLSSignature sig;

public:
    ChainLockSig();
    ~ChainLockSig();

    ChainLockSig(int32_t nHeight, const uint256& blockHash, const CBLSSignature& sig);

    [[nodiscard]] int32_t getHeight() const { return nHeight; }
    [[nodiscard]] const uint256& getBlockHash() const { return blockHash; }
    [[nodiscard]] const CBLSSignature& getSig() const { return sig; }
    [[nodiscard]] bool IsNull() const { return nHeight == -1 && blockHash == uint256(); }
    [[nodiscard]] std::string ToString() const;

    SERIALIZE_METHODS(ChainLockSig, obj)
    {
        READWRITE(obj.nHeight, obj.blockHash, obj.sig);
    }
};
} // namespace chainlock

#endif // BITCOIN_CHAINLOCK_CLSIG_H
