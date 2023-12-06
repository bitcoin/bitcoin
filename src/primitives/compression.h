#ifndef BITCOIN_PRIMITIVES_COMPRESSION_H
#define BITCOIN_PRIMITIVES_COMPRESSION_H

#include <addresstype.h>
#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <util/strencodings.h>
#include <bitset>
#include <cmath>
#include <logging.h>
#include <streams.h>
#include <secp256k1.h>
#include <secp256k1_extrakeys.h>
#include <secp256k1_recovery.h>
#include <secp256k1_schnorrsig.h>
#include <serialize.h>
#include <uint256.h>

class CCompressedOutPoint
{
private:
    uint32_t m_block_height;
    uint32_t m_block_index;
    uint256 m_txid;
    uint32_t m_vout;
public:
    const uint32_t&  block_height() const  { return m_block_height; }
    const uint32_t& block_index() const { return m_block_index; }
    const uint256& txid() const { return m_txid; }
    const uint32_t& vout() const { return m_vout; }

    explicit CCompressedOutPoint(const uint32_t block_height, const uint32_t block_index);
    explicit CCompressedOutPoint(const uint256& txid, const uint32_t vout);

    bool IsCompressed() const {
        return m_block_height > 0;
    }

    bool IsCoinbase() const {
        return !this->IsCompressed() && m_txid.IsNull();
    }

    friend bool operator==(const CCompressedOutPoint& a, const CCompressedOutPoint& b)
    {
        return (a.m_block_height == b.m_block_height && a.m_block_index == b.m_block_index && a.m_txid == b.m_txid && a.m_vout == b.m_vout);
    }

    friend bool operator!=(const CCompressedOutPoint& a, const CCompressedOutPoint& b)
    {
        return !(a == b);
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        /* BlockHeight: Section 3.6(1) */
        s << VARINT(this->m_block_height);
        if (this->IsCompressed()) {
            /* Txid/Vout: Section 3.6.1(1) */
            s << VARINT(this->m_block_index);
        } else {
            /* Txid: Section 3.6.2(1) */
            s.write(MakeByteSpan(this->m_txid));
            /* Vout: Section 3.6.2(2) */
            s << VARINT(this->m_vout);

        }
    }

    template <typename Stream>
    inline void Unserialize(Stream& s, bool constructor=false) {
        /* BlockHeight: Section 3.6(1) */
        this->m_block_height = std::numeric_limits<uint32_t>::max();
        s >> VARINT(this->m_block_height);
        if (this->IsCompressed()) {
            /* BlockHeight: Section 3.6.1(1) */
            this->m_block_index = std::numeric_limits<uint32_t>::max();
            s >> VARINT(this->m_block_index);
            this->m_vout = 0;
        } else {
            this->m_block_index = 0;
            /* Txid: Section 3.6.2(1) */
            std::vector<unsigned char> txid(32);
            s.read(MakeWritableByteSpan(txid));
            this->m_txid = uint256(txid);
            /* Vout: Section 3.6.2(2) */
            this->m_vout = std::numeric_limits<uint32_t>::max();
            s >> VARINT(this->m_vout);
        }
    }

    template <typename Stream>
    explicit CCompressedOutPoint(deserialize_type, Stream& s) {
        Unserialize(s, true);
    }

    std::string ToString() const;
};

class CCompressedTxIn
{
private:
    CCompressedOutPoint m_prevout;
    std::vector<unsigned char> m_signature;
    std::vector<unsigned char> m_pubkeyHash;
    bool m_compressedSignature;
    uint8_t m_hashType;
    bool m_isHashStandard;
    uint32_t m_nSequence;
    std::string m_warning;
public:
    const CCompressedOutPoint& prevout() const { return m_prevout; }
    const std::vector<unsigned char>& signature() const { return m_signature; }
    const std::vector<unsigned char>& pubkeyHash() const { return m_pubkeyHash; }
    const bool& compressedSignature() const { return m_compressedSignature; }
    const uint8_t& hashType() const { return m_hashType; }
    const bool& isHashStandard() const { return m_isHashStandard; }
    const uint32_t& nSequence() const { return m_nSequence; }
    const std::string& warning() const { return m_warning; }

    explicit CCompressedTxIn(const CTxIn& txin, const CCompressedOutPoint& out, const CScript& script_pubkey);

    bool IsSigned() const {
        return !this->m_signature.empty();
    }

    bool IsSequenceStandard() const {
        return this->m_nSequence == 0x00000000 || this->m_nSequence == 0xFFFFFFFE || this->m_nSequence == 0xFFFFFFFF;
    }

    friend bool operator==(const CCompressedTxIn& a, const CCompressedTxIn& b)
    {
        return (a.m_prevout == b.m_prevout && a.m_signature == b.m_signature && a.m_compressedSignature == b.m_compressedSignature && a.m_hashType == b.m_hashType && a.m_isHashStandard == b.m_isHashStandard && a.m_nSequence == b.m_nSequence);
    }

    friend bool operator!=(const CCompressedTxIn& a, const CCompressedTxIn& b)
    {
        return !(a == b);
    }

    uint64_t GetMetadata() const {
        uint64_t metadata = 0;
        metadata |= this->m_compressedSignature;
        metadata |= (!this->m_pubkeyHash.empty()) << 1;
        metadata |= this->m_isHashStandard << 2;
        switch (this->m_nSequence) {
            case 0x00000000:
                metadata |= (1 << 3);
                break;
            case 0xFFFFFFFE:
                metadata |= (0b10 << 3);
                break;
            case 0xFFFFFFFF:
                metadata |= (0b11 << 3);
                break;
        }
        return metadata;
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        /* Txid/Vout: Section 3.6(1-2) */
        this->m_prevout.Serialize(s);
        /* Signature: Section 3.6(3) */
        if (this->m_compressedSignature) {
            /* Compressed Signature: Section 3.6.1(2) */
            s.write(MakeByteSpan(this->m_signature));
            if (!this->m_pubkeyHash.empty()) {
                s.write(MakeByteSpan(this->m_pubkeyHash));
            }
            /* HashType: Section 3.6.1(3) */
            if (!this->m_isHashStandard) s << this->m_hashType;
        } else {
            s << COMPACTSIZE(this->m_signature.size());
            if (!this->m_signature.empty()) {
                /* Uncompressed Signature: Section 3.6.2(3) */
                s.write(MakeByteSpan(this->m_signature));
            }
        }
        /* Sequence: Section 3.6(4) */
        if (!this->IsSequenceStandard()) {
            s << VARINT(this->m_nSequence);
        }
    }

    template <typename Stream>
    inline void Unserialize(Stream& s, uint64_t& metadata, bool constructor=false) {
        m_hashType = 0;
        m_compressedSignature = metadata & 1;
        bool pubkeyHashNonEmpty = (metadata & (1 << 1)) >> 1;
        m_isHashStandard = (metadata & (1 << 2)) >> 2;
        uint8_t sequenceEncoding = (metadata & (0b11 << 3)) >> 3;

        /* Txid/Vout: Section 3.6(1-2) */
        if (!constructor) this->m_prevout.Unserialize(s);
        /* Signature: Section 3.6(3) */
        if (this->m_compressedSignature) {
            /* Compressed Signature: Section 3.6.1(2) */
            m_signature.resize(64);
            s.read(MakeWritableByteSpan(m_signature));
            /* Pubkey Hash: Section 3.6.1(2) */
            if (pubkeyHashNonEmpty) {
                m_pubkeyHash.resize(20);
                s.read(MakeWritableByteSpan(m_pubkeyHash));
            }

            /* HashType: Section 3.6.1(3) */
            if (!m_isHashStandard) {
                s >> this->m_hashType;
            }
        } else {
            /* Uncompressed Signature: Section 3.6.2(3) */
            uint64_t scriptLength = std::numeric_limits<uint64_t>::max();
            s >> COMPACTSIZE(scriptLength);
            if (scriptLength) {
                m_signature.resize(scriptLength);
                s.read(MakeWritableByteSpan(m_signature));
            }
        }

        /* Sequence: Section 3.6(4) */
        if (sequenceEncoding) {
            if (sequenceEncoding == 0x01) m_nSequence = 0x00000000;
            if (sequenceEncoding == 0x02) m_nSequence = 0xFFFFFFFE;
            if (sequenceEncoding == 0x03) m_nSequence = 0xFFFFFFFF;
        } else {
            this->m_nSequence = std::numeric_limits<uint32_t>::max();
            s >> VARINT(this->m_nSequence);
        }
    }

    template <typename Stream>
    explicit CCompressedTxIn(deserialize_type, Stream& s, uint64_t& metadata) : m_prevout(CCompressedOutPoint(deserialize, s)) {
        Unserialize(s, metadata, true);
    }

    std::string ToString() const;
};


class CCompressedTxOut
{
private:
    std::vector<unsigned char> m_scriptPubKey;
    TxoutType m_scriptType;
    int64_t m_nValue;
public:
    const std::vector<unsigned char>& scriptPubKey() const { return m_scriptPubKey; }
    const TxoutType& scriptType() const { return m_scriptType; }
    const int64_t& nValue() const { return m_nValue; }

    explicit CCompressedTxOut(const CTxOut& txout);



    friend bool operator==(const CCompressedTxOut& a, const CCompressedTxOut& b)
    {
        return (a.m_scriptPubKey == b.m_scriptPubKey && a.m_scriptType == b.m_scriptType && a.m_nValue == b.m_nValue);
    }

    friend bool operator!=(const CCompressedTxOut& a, const CCompressedTxOut& b)
    {
        return !(a == b);
    }

    uint8_t GetSerializedScriptType() const {
        if (this->m_scriptType == TxoutType::PUBKEY && this->m_scriptPubKey.size() == 65) return 0;
        if (this->m_scriptType == TxoutType::PUBKEY && this->m_scriptPubKey.size() == 33) return 1;
        if (this->m_scriptType == TxoutType::PUBKEYHASH) return 2;
        if (this->m_scriptType == TxoutType::SCRIPTHASH) return 3;
        if (this->m_scriptType == TxoutType::WITNESS_V0_SCRIPTHASH) return 4;
        if (this->m_scriptType == TxoutType::WITNESS_V0_KEYHASH) return 5;
        if (this->m_scriptType == TxoutType::WITNESS_V1_TAPROOT) return 6;
        return 7;
    }

    bool IsCompressed() const {
        return this->GetSerializedScriptType() != 7;
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        if (!this->m_scriptPubKey.empty()) {
            /* Output Script: Section 3.7(1) */
            if (!this->IsCompressed()) {
                s << COMPACTSIZE(this->m_scriptPubKey.size());
            }
            s.write(MakeByteSpan(this->m_scriptPubKey));
        }
        /* Amount: Section 3.7(2) */
        s << VARINT(static_cast<uint64_t>(this->m_nValue));
    }

    template <typename Stream>
    inline void Unserialize(Stream& s, uint8_t serialized_script_type) {
        /* Output Script: Section 3.7(1) */
        uint64_t scriptLength = std::numeric_limits<uint64_t>::max();
        switch (serialized_script_type) {
            case 0:
                scriptLength = 65;
                this->m_scriptType = TxoutType::PUBKEY;
                break;
            case 1:
                scriptLength = 33;
                this->m_scriptType = TxoutType::PUBKEY;
                break;
            case 2:
                scriptLength = 20;
                this->m_scriptType = TxoutType::PUBKEYHASH;
                break;
            case 3:
                scriptLength = 20;
                this->m_scriptType = TxoutType::SCRIPTHASH;
                break;
            case 4:
                scriptLength = 32;
                this->m_scriptType = TxoutType::WITNESS_V0_SCRIPTHASH;
                break;
            case 5:
                scriptLength = 20;
                this->m_scriptType = TxoutType::WITNESS_V0_KEYHASH;
                break;
            case 6:
                scriptLength = 32;
                this->m_scriptType = TxoutType::WITNESS_V1_TAPROOT;
                break;
            case 7:
                s >> COMPACTSIZE(scriptLength);
                if (scriptLength >= MAX_SCRIPT_SIZE) throw std::ios_base::failure("Output Script Exceeds MAX_SCRIPT_SIZE");
                this->m_scriptType = TxoutType::NONSTANDARD;
                break;
            default:
                throw std::ios_base::failure(strprintf("Script Type Deseralization must be 0-7, %u is not a valid Script Type.", serialized_script_type));
        }
        this->m_scriptPubKey.resize(scriptLength);
        s.read(MakeWritableByteSpan(this->m_scriptPubKey));

        /* Amount: Section 3.7(2) */
        uint64_t us_value = std::numeric_limits<uint64_t>::max();
        s >> VARINT(us_value);
        this->m_nValue = us_value;
    }

    template <typename Stream>
    explicit CCompressedTxOut(deserialize_type, Stream& s, uint8_t serialized_script_type) {
        Unserialize(s, serialized_script_type);
    }

    std::string ToString() const;
};

class CCompressedInput {
    private:
        CCompressedOutPoint m_outpoint;
        CScript m_scriptPubKey;
    public:
        const CCompressedOutPoint& outpoint() const { return m_outpoint; }
        const CScript& scriptPubKey() const { return m_scriptPubKey; }

        explicit CCompressedInput(const CCompressedOutPoint& outpoint, const CScript& scriptPubKey);

        friend bool operator==(const CCompressedInput& a, const CCompressedInput& b)
        {
            return (a.m_outpoint == b.m_outpoint && a.m_scriptPubKey == b.m_scriptPubKey);
        }

        friend bool operator!=(const CCompressedInput& a, const CCompressedInput& b)
        {
            return !(a == b);
        }

        std::string ToString() const;
};

class CCompressedTransaction
{
private:
    uint32_t m_minimumHeight;
    int32_t m_nVersion;
    uint32_t m_nLockTime;
    std::vector<CCompressedTxIn> m_vin;
    std::vector<CCompressedTxOut> m_vout;
public:
    const uint32_t& minimumHeight() const { return m_minimumHeight; }
    const int32_t& nVersion() const { return m_nVersion; }
    const uint32_t& nLockTime() const { return m_nLockTime; }
    const std::vector<CCompressedTxIn>& vin() const { return m_vin; }
    const std::vector<CCompressedTxOut>& vout() const { return m_vout; }

    explicit CCompressedTransaction(const CTransaction tx, const uint32_t& minimumHeight, const std::vector<CCompressedInput>& cinputs);

    friend bool operator==(const CCompressedTransaction& a, const CCompressedTransaction& b)
    {
        return (a.m_nVersion == b.m_nVersion && a.m_nLockTime == b.m_nLockTime && a.m_vin == b.m_vin && a.m_vout == b.m_vout);
    }

    friend bool operator!=(const CCompressedTransaction& a, const CCompressedTransaction& b)
    {
        return !(a == b);
    }

    template <typename Stream>
    inline void Serialize(Stream& s) const {
        if (this->m_vin.empty() || this->m_vout.empty()) throw std::ios_base::failure("Must have at least one input and output");

        /* Transaction Metadata: Section 3.2(1) */
        /* Version: Section 3.3(1) */
        uint8_t version = 0;
        if (this->m_nVersion < 4 && this->m_nVersion > 0) version = this->m_nVersion;
        uint8_t control = version;
        /* InputCount: Section 3.3(2) */
        uint8_t inputCount = 0;
        if (this->m_vin.size() < 4) inputCount = this->m_vin.size();
        control |= inputCount << 2;
        /* OutputCount: Section 3.3(3) */
        uint8_t outputCount = 0;
        if (this->m_vout.size() < 4) outputCount = this->m_vout.size();
        control |= outputCount << 4;
        /* LockTime: Section 3.3(4) */
        control |= (this->m_nLockTime > 0) << 6;

        s << control;

        /* Version: Section 3.2(2) */
        if (!version) s << VARINT(static_cast<uint32_t>(this->m_nVersion));
        /* InputCount: Section 3.2(3) */
        if (!inputCount) s << COMPACTSIZE(this->m_vin.size());
        /* OutputCount: Section 3.2(4) */
        if (!outputCount) s << COMPACTSIZE(this->m_vout.size());
        /* LockTime: Section 3.2(5) */
        if (this->m_nLockTime) s << VARINT(this->m_nLockTime);

        /* Minimum BlockHeight: Section 3.2(6) */
        s << VARINT(this->m_minimumHeight);

        /* Input-Output Metadata: Section 3.4-3.5 */
        std::vector<int> sizes;
        std::vector<unsigned char> controlVec;
        for (const auto &vin : this->m_vin) {
            controlVec.push_back(vin.GetMetadata());
            sizes.push_back(5);
        }
        for (const auto &vout : this->m_vout) {
            controlVec.push_back(vout.GetSerializedScriptType());
            sizes.push_back(3);
        }
        std::vector<int> resizes;
        resizes.push_back(8);
        std::vector<unsigned char> compressedControlVec;
        CHECK_NONFATAL(ConvertBitsVariable<true>(sizes, resizes, [&](unsigned char c) { compressedControlVec.push_back(c); }, controlVec.begin(), controlVec.end()));
        s.write(MakeByteSpan(compressedControlVec));

        /* Input: Section 3.6 */
        for (const auto &vin : this->m_vin) {
            vin.Serialize(s);
        }

        /* Output: Section 3.7 */
        for (const auto &vout : this->m_vout) {
            vout.Serialize(s);
        }
    }

    template <typename Stream>
    inline void Unserialize(Stream& s) {
        /* Transaction Metadata: Section 3.2 */
        uint8_t control;
        s >> control;
        /* Version: Section 3.3(1) */
        this->m_nVersion = control & 3;
        /* InputCount: Section 3.3(2) */
        uint32_t nInputCount = (control & (3 << 2)) >> 2;
        /* OutputCount: Section 3.3(3) */
        uint32_t nOutputCount = (control & (3 << 4)) >> 4;
        /* LockTime: Section 3.3(4) */
        this->m_nLockTime = (control & (3 << 6)) >> 6;

        /* Version: Section 3.2(2) */
        if (!this->m_nVersion) {
            uint32_t us_version = std::numeric_limits<uint32_t>::max();
            s >> VARINT(us_version);
            this->m_nVersion = static_cast<int32_t>(us_version);
        }

        /* InputCount: Section 3.2(3) */
        if (!nInputCount) {
            nInputCount = std::numeric_limits<uint32_t>::max();
            s >> COMPACTSIZE(nInputCount);
        }

        /* OutputCount: Section 3.2(4) */
        if (!nOutputCount) {
            nOutputCount = std::numeric_limits<uint32_t>::max();
            s >> COMPACTSIZE(nOutputCount);
        }

        if (!nInputCount || !nOutputCount) throw std::ios_base::failure("Must have at least one input and output");

        /* LockTime: Section 3.2(5) */
        if (this->m_nLockTime) {
            this->m_nLockTime = std::numeric_limits<uint32_t>::max();
            s >> VARINT(this->m_nLockTime);
        }

        /* Minimum BlockHeight: Section 3.2(6) */
        this->m_minimumHeight = std::numeric_limits<uint32_t>::max();
        s >> VARINT(this->m_minimumHeight);

        /* Input-Output Metadata: Section 3.4-3.5 */
        std::vector<unsigned char> controlVec;
        controlVec.resize(((nInputCount * 5) + (nOutputCount * 3) + 7) / 8);
        s.read(MakeWritableByteSpan(controlVec));

        std::vector<int> sizes;
        sizes.push_back(8);
        std::vector<int> resizes;
        for (size_t index = 0; index < nInputCount; index++) {
            resizes.push_back(5);
        }
        for (size_t index = 0; index < nOutputCount; index++) {
            resizes.push_back(3);
        }

        std::vector<unsigned char> parsedControlVec;
        CHECK_NONFATAL(ConvertBitsVariable<true>(sizes, resizes, [&](unsigned char c) { parsedControlVec.push_back(c); }, controlVec.begin(), controlVec.end()));

        /* Input: Section 3.6 */
        for (size_t index = 0; index < nInputCount; index++) {
            uint64_t metadata = (uint64_t)parsedControlVec[index];
            this->m_vin.push_back(CCompressedTxIn(deserialize, s, metadata));
        }

        /* Input: Section 3.7 */
        for (size_t index = 0; index < nOutputCount; index++) {
            uint8_t serializedScriptType = (uint8_t)parsedControlVec[index+nInputCount];
            this->m_vout.push_back(CCompressedTxOut(deserialize, s, serializedScriptType));
        }
    }

    template <typename Stream>
    CCompressedTransaction(deserialize_type, Stream& s) {
        Unserialize(s);
    }

    std::string ToString() const;
};

#endif // BITCOIN_PRIMITIVES_COMPRESSION_H
