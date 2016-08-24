// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (C) 2016 Tom Zander <tomz@freedommail.ch>
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_PRIMITIVES_TRANSACTION_H
#define BITCOIN_PRIMITIVES_TRANSACTION_H

#include "amount.h"
#include "script/script.h"
#include "serialize.h"
#include "uint256.h"

#include "../consensus/transactionv4.h"

/** An outpoint - a combination of a transaction hash and an index n into its vout */
class COutPoint
{
public:
    uint256 hash;
    uint32_t n;

    COutPoint() { SetNull(); }
    COutPoint(uint256 hashIn, uint32_t nIn) { hash = hashIn; n = nIn; }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(hash);
        READWRITE(n);
    }

    void SetNull() { hash.SetNull(); n = (uint32_t) -1; }
    bool IsNull() const { return (hash.IsNull() && n == (uint32_t) -1); }

    friend bool operator<(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash < b.hash || (a.hash == b.hash && a.n < b.n));
    }

    friend bool operator==(const COutPoint& a, const COutPoint& b)
    {
        return (a.hash == b.hash && a.n == b.n);
    }

    friend bool operator!=(const COutPoint& a, const COutPoint& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An input of a transaction.  It contains the location of the previous
 * transaction's output that it claims and a signature that matches the
 * output's public key.
 */
class CTxIn
{
public:
    COutPoint prevout;
    CScript scriptSig;
    uint32_t nSequence;

    /* Setting nSequence to this value for every input in a transaction
     * disables nLockTime. */
    static const uint32_t SEQUENCE_FINAL = 0xffffffff;

    /* Below flags apply in the context of BIP 68*/
    /* If this flag set, CTxIn::nSequence is NOT interpreted as a
     * relative lock-time. */
    static const uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = (1 << 31);

    /* If CTxIn::nSequence encodes a relative lock-time and this flag
     * is set, the relative lock-time has units of 512 seconds,
     * otherwise it specifies blocks with a granularity of 1. */
    static const uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG = (1 << 22);

    /* If CTxIn::nSequence encodes a relative lock-time, this mask is
     * applied to extract that lock-time from the sequence field. */
    static const uint32_t SEQUENCE_LOCKTIME_MASK = 0x0000ffff;

    /* In order to use the same number of bits to encode roughly the
     * same wall-clock duration, and because blocks are naturally
     * limited to occur every 600s on average, the minimum granularity
     * for time-based relative lock-time is fixed at 512 seconds.
     * Converting from CTxIn::nSequence to seconds is performed by
     * multiplying by 512 = 2^9, or equivalently shifting up by
     * 9 bits. */
    static const int SEQUENCE_LOCKTIME_GRANULARITY = 9;

    CTxIn()
    {
        nSequence = SEQUENCE_FINAL;
    }

    explicit CTxIn(COutPoint prevoutIn, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);
    CTxIn(uint256 hashPrevTx, uint32_t nOut, CScript scriptSigIn=CScript(), uint32_t nSequenceIn=SEQUENCE_FINAL);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(prevout);
        READWRITE(*(CScriptBase*)(&scriptSig));
        READWRITE(nSequence);
    }

    friend bool operator==(const CTxIn& a, const CTxIn& b)
    {
        return (a.prevout   == b.prevout &&
                a.scriptSig == b.scriptSig &&
                a.nSequence == b.nSequence);
    }

    friend bool operator!=(const CTxIn& a, const CTxIn& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

/** An output of a transaction.  It contains the public key that the next input
 * must be able to sign with to claim it.
 */
class CTxOut
{
public:
    CAmount nValue;
    CScript scriptPubKey;

    CTxOut()
    {
        SetNull();
    }

    CTxOut(const CAmount& nValueIn, CScript scriptPubKeyIn);

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nValue);
        READWRITE(*(CScriptBase*)(&scriptPubKey));
    }

    void SetNull()
    {
        nValue = -1;
        scriptPubKey.clear();
    }

    bool IsNull() const
    {
        return (nValue == -1);
    }

    uint256 GetHash() const;

    CAmount GetDustThreshold(const CFeeRate &minRelayTxFee) const
    {
        // "Dust" is defined in terms of CTransaction::minRelayTxFee,
        // which has units satoshis-per-kilobyte.
        // If you'd pay more than 1/3 in fees
        // to spend something, then we consider it dust.
        // A typical spendable txout is 34 bytes big, and will
        // need a CTxIn of at least 148 bytes to spend:
        // so dust is a spendable txout less than
        // 546*minRelayTxFee/1000 (in satoshis)
        if (scriptPubKey.IsUnspendable())
            return 0;

        size_t nSize = GetSerializeSize(SER_DISK,0)+148u;
        return 3*minRelayTxFee.GetFee(nSize);
    }

    bool IsDust(const CFeeRate &minRelayTxFee) const
    {
        return (nValue < GetDustThreshold(minRelayTxFee));
    }

    friend bool operator==(const CTxOut& a, const CTxOut& b)
    {
        return (a.nValue       == b.nValue &&
                a.scriptPubKey == b.scriptPubKey);
    }

    friend bool operator!=(const CTxOut& a, const CTxOut& b)
    {
        return !(a == b);
    }

    std::string ToString() const;
};

struct CMutableTransaction;

template<typename Stream, typename TxType>
inline void SerializeTransaction(TxType& tx, Stream& s, int nType, int nVersion, bool withSignatures = true) {
    ser_writedata32(s, tx.nVersion);
    nVersion = tx.nVersion;
    if (nVersion == 4) {
        for (auto in : tx.vin) {
            CMFToken hash(Consensus::TxInPrevHash, in.prevout.hash);
            STORECMF(hash);
            if (in.prevout.n > 0) {
                CMFToken index(Consensus::TxInPrevIndex, (uint64_t) in.prevout.n);
                STORECMF(index);
            }
            // TODO check sequence to maybe store the BIP68 stuff.
        }
        for (auto out : tx.vout) {
            CMFToken token(Consensus::TxOutValue, (uint64_t) out.nValue);
            STORECMF(token);
            std::vector<char> script(out.scriptPubKey.begin(), out.scriptPubKey.end());
            token = CMFToken(Consensus::TxOutScript, script);
            STORECMF(token);
        }
        if (withSignatures) {
            for (auto in : tx.vin) {
                CMFToken token(Consensus::TxInScript, std::vector<char>(in.scriptSig.begin(), in.scriptSig.end()));
                STORECMF(token);
            }
            CMFToken end(Consensus::TxEnd);
            STORECMF(end);
        }
    } else {
        CSerActionSerialize ser_action;
        READWRITE(tx.vin);
        READWRITE(tx.vout);
        READWRITE(tx.nLockTime);
    }
}

std::vector<char> loadTransaction(const std::vector<CMFToken>& tokens, std::vector<CTxIn> &inputs, std::vector<CTxOut> &outputs, int nVersion);

template<typename Stream, typename TxType>
inline std::vector<char> UnSerializeTransaction(TxType& tx, Stream& s, int nType, int nVersion) {
    *const_cast<int32_t*>(&tx.nVersion) = ser_readdata32(s);
    nVersion = tx.nVersion;
    if (nVersion == 4) {
        return loadTransaction(UnserializeCMFs(s, Consensus::TxEnd, nType, nVersion),
            *const_cast<std::vector<CTxIn>*>(&tx.vin),
            *const_cast<std::vector<CTxOut>*>(&tx.vout), nVersion);
    } else {
        CSerActionUnserialize ser_action;
        READWRITE(*const_cast<std::vector<CTxIn>*>(&tx.vin));
        READWRITE(*const_cast<std::vector<CTxOut>*>(&tx.vout));
        READWRITE(*const_cast<uint32_t*>(&tx.nLockTime));
    }
    return std::vector<char>();
}

/** The basic transaction that is broadcasted on the network and contained in
 * blocks.  A transaction can contain multiple inputs and outputs.
 */
class CTransaction
{
private:
    /** Memory only. */
    uint256 hash;
    void UpdateHash();
    std::vector<char> txData;

public:
    // Default transaction version.
    static const int32_t CURRENT_VERSION=1;

    // Changing the default transaction version requires a two step process: first
    // adapting relay policy by bumping MAX_STANDARD_VERSION, and then later date
    // bumping the default CURRENT_VERSION at which point both CURRENT_VERSION and
    // MAX_STANDARD_VERSION will be equal.
    static const int32_t MAX_STANDARD_VERSION = 4;

    // The local variables are made const to prevent unintended modification
    // without updating the cached hash value. However, CTransaction is not
    // actually immutable; deserialization and assignment are implemented,
    // and bypass the constness. This is safe, as they update the entire
    // structure, including the hash.
    const int32_t nVersion;
    const std::vector<CTxIn> vin;
    const std::vector<CTxOut> vout;
    const uint32_t nLockTime;

    /** Construct a CTransaction that qualifies as IsNull() */
    CTransaction();

    /** Convert a CMutableTransaction into a CTransaction. */
    CTransaction(const CMutableTransaction &tx);

    CTransaction& operator=(const CTransaction& tx);

    size_t GetSerializeSize(int nType, int nVersion) const {
        CSizeComputer s(nType, nVersion);
        Serialize(s, nType, nVersion);
        return s.size();
    }
    template<typename Stream>
    void Serialize(Stream& s, int nType, int version) const {
        if (!txData.empty()) {
            s.write(&txData[0], txData.size());
            for (auto in : vin)
                STORECMF(CMFToken(Consensus::TxInScript, std::vector<char>(in.scriptSig.begin(), in.scriptSig.end())));
            STORECMF(CMFToken(Consensus::TxEnd));
        } else {
            SerializeTransaction(*this, s, nType, nVersion);
        }
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType, int version) {
        txData = UnSerializeTransaction(*const_cast<CTransaction*>(this), s, nType, version);
        UpdateHash();
    }

    bool IsNull() const {
        return vin.empty() && vout.empty();
    }

    const uint256& GetHash() const {
        return hash;
    }

    uint256 CalculateSignaturesHash() const;

    // Return sum of txouts.
    CAmount GetValueOut() const;
    // GetValueIn() is a method on CCoinsViewCache, because
    // inputs must be known to compute value in.

    // Compute priority, given priority of inputs and (optionally) tx size
    double ComputePriority(double dPriorityInputs, unsigned int nTxSize=0) const;

    // Compute modified tx size for priority calculation (optionally given tx size)
    unsigned int CalculateModifiedSize(unsigned int nTxSize=0) const;

    bool IsCoinBase() const
    {
        return (vin.size() == 1 && vin[0].prevout.IsNull());
    }

    friend bool operator==(const CTransaction& a, const CTransaction& b)
    {
        return a.hash == b.hash;
    }

    friend bool operator!=(const CTransaction& a, const CTransaction& b)
    {
        return a.hash != b.hash;
    }

    std::string ToString() const;
};

/** A mutable version of CTransaction. */
struct CMutableTransaction
{
    int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    uint32_t nLockTime;

    CMutableTransaction();
    CMutableTransaction(const CTransaction& tx);

    size_t GetSerializeSize(int nType, int nVersion) const {
        CSizeComputer s(nType, nVersion);
        Serialize(s, nType, nVersion);
        return s.size();
    }
    template<typename Stream>
    void Serialize(Stream& s, int nType, int version) const {
        SerializeTransaction(*this, s, nType, nVersion);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType, int version) {
        (void) UnSerializeTransaction(*this, s, nType, nVersion);
    }

    /** Compute the hash of this CMutableTransaction. This is computed on the
     * fly, as opposed to GetHash() in CTransaction, which uses a cached result.
     */
    uint256 GetHash() const;
};

#endif // BITCOIN_PRIMITIVES_TRANSACTION_H
