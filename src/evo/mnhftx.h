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
class TxValidationState;
extern RecursiveMutex cs_main;

// mnhf signal special transaction
class MNHFTx
{
public:
    uint8_t versionBit{0};
    uint256 quorumHash;
    CBLSSignature sig;

    MNHFTx() = default;
    bool Verify(const CBlockIndex* pQuorumIndex, const uint256& msgHash, TxValidationState& state) const;

    SERIALIZE_METHODS(MNHFTx, obj)
    {
        READWRITE(obj.versionBit, obj.quorumHash);
        READWRITE(CBLSSignatureVersionWrapper(const_cast<CBLSSignature&>(obj.sig), /* fLegacy= */ false));
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.clear();
        obj.setObject();
        obj.pushKV("versionBit", (int)versionBit);
        obj.pushKV("quorumHash", quorumHash.ToString());
        obj.pushKV("sig", sig.ToString());
    }
};

class MNHFTxPayload
{
public:
    static constexpr auto SPECIALTX_TYPE = TRANSACTION_MNHF_SIGNAL;
    static constexpr uint16_t CURRENT_VERSION = 1;

    uint8_t nVersion{CURRENT_VERSION};
    MNHFTx signal;

    SERIALIZE_METHODS(MNHFTxPayload, obj)
    {
        READWRITE(obj.nVersion, obj.signal);
    }

    std::string ToString() const;

    void ToJson(UniValue& obj) const
    {
        obj.setObject();
        obj.pushKV("version", (int)nVersion);

        UniValue mnhfObj;
        signal.ToJson(mnhfObj);
        obj.pushKV("signal", mnhfObj);
    }
};

bool CheckMNHFTx(const CTransaction& tx, const CBlockIndex* pindexPrev, TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

#endif // BITCOIN_EVO_MNHFTX_H
