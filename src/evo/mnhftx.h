// Copyright (c) 2021-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_MNHFTX_H
#define BITCOIN_EVO_MNHFTX_H

#include <bls/bls.h>
#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>
#include <univalue.h>

class CBlockIndex;
class CValidationState;
extern CCriticalSection cs_main;

// mnhf signal special transaction
class MNHFTx
{
public:
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint16_t nVersion{CURRENT_VERSION};
    uint256 quorumHash;
    CBLSSignature sig;

    MNHFTx() = default;
    bool Verify(const CBlockIndex* pQuorumIndex) const;

    SERIALIZE_METHODS(MNHFTx, obj)
    {
        READWRITE(obj.nVersion, obj.quorumHash, obj.sig);
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("version", (int)nVersion);
        obj.pushKV("quorumHash", quorumHash.ToString());
        obj.pushKV("sig", sig.ToString());
    }
};

class MNHFTxPayload
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_MNHF_SIGNAL;
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint16_t nVersion{CURRENT_VERSION};
    MNHFTx signal;

    SERIALIZE_METHODS(MNHFTxPayload, obj)
    {
        READWRITE(obj.nVersion, obj.signal);
    }

    void ToJson(UniValue& obj) const
    {
        obj.setObject();
        obj.pushKV("version", (int)nVersion);

        UniValue mnhfObj;
        signal.ToJson(mnhfObj);
        obj.pushKV("signal", mnhfObj);
    }
};

bool CheckMNHFTx(const CTransaction& tx, const CBlockIndex* pindexPrev, CValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

#endif // BITCOIN_EVO_MNHFTX_H
