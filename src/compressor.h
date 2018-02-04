// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMPRESSOR_H
#define BITCOIN_COMPRESSOR_H

#include <primitives/transaction.h>
#include <script/script.h>
#include <serialize.h>


class CKeyID;
class CPubKey;
class CScriptID;

/** Compact serializer for scripts.
 *
 *  It detects common cases and encodes them much more efficiently.
 *  3 special cases are defined:
 *  * Pay to pubkey hash (encoded as 21 bytes)
 *  * Pay to script hash (encoded as 21 bytes)
 *  * Pay to pubkey starting with 0x02, 0x03 or 0x04 (encoded as 33 bytes)
 *
 *  Other scripts up to 121 bytes require 1 byte + script length. Above
 *  that, scripts up to 16505 bytes require 2 bytes + script length.
 */
class CScriptCompressor
{
private:
    /**
     * make this static for now (there are only 6 special scripts defined)
     * this can potentially be extended together with a new nVersion for
     * transactions, in which case this value becomes dependent on nVersion
     * and nHeight of the enclosing transaction.
     */
    static const unsigned int nSpecialScripts = 6;

    CScript &script;
protected:
    /**
     * These check for scripts for which a special case with a shorter encoding is defined.
     * They are implemented separately from the CScript test, as these test for exact byte
     * sequence correspondences, and are more strict. For example, IsToPubKey also verifies
     * whether the public key is valid (as invalid ones cannot be represented in compressed
     * form).
     */
    bool IsToKeyID(CKeyID &hash) const;
    bool IsToScriptID(CScriptID &hash) const;
    bool IsToPubKey(CPubKey &pubkey) const;

    bool Compress(std::vector<unsigned char> &out) const;
    unsigned int GetSpecialSize(unsigned int nSize) const;
    bool Decompress(unsigned int nSize, const std::vector<unsigned char> &out);
public:
    explicit CScriptCompressor(CScript &scriptIn) : script(scriptIn) { }

    template<typename Stream>
    void Serialize(Stream &s) const {
        std::vector<unsigned char> compr;
        if (Compress(compr)) {
            s << CFlatData(compr);
            return;
        }
        unsigned int nSize = script.size() + nSpecialScripts;
        s << VARINT(nSize);
        s << CFlatData(script);
    }

    template<typename Stream>
    void Unserialize(Stream &s) {
        unsigned int nSize = 0;
        s >> VARINT(nSize);
        if (nSize < nSpecialScripts) {
            std::vector<unsigned char> vch(GetSpecialSize(nSize), 0x00);
            s >> REF(CFlatData(vch));
            Decompress(nSize, vch);
            return;
        }
        nSize -= nSpecialScripts;
        if (nSize > MAX_SCRIPT_SIZE) {
            // Overly long script, replace with a short invalid one
            script << OP_RETURN;
            s.ignore(nSize);
        } else {
            script.resize(nSize);
            s >> REF(CFlatData(script));
        }
    }
};

class CTxCompressorMethods
{
public:
    static uint64_t CompressAmount(uint64_t nAmount);
    static uint64_t DecompressAmount(uint64_t nAmount);
};

/** wrapper for CTxOut that provides a more compact serialization */
class CTxOutCompressor : public CTxCompressorMethods
{
private:
    CTxOut &txout;

public:
    //static uint64_t CompressAmount(uint64_t nAmount);
    //static uint64_t DecompressAmount(uint64_t nAmount);

    explicit CTxOutCompressor(CTxOut &txoutIn) : txout(txoutIn) { }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        if (!ser_action.ForRead()) {
            uint64_t nVal = CompressAmount(txout.nValue);
            READWRITE(VARINT(nVal));
        } else {
            uint64_t nVal = 0;
            READWRITE(VARINT(nVal));
            txout.nValue = DecompressAmount(nVal);
        }
        CScriptCompressor cscript(REF(txout.scriptPubKey));
        READWRITE(cscript);
    }
};

/** wrapper for CTxOutBase that provides a more compact serialization */
class CTxOutBaseCompressor: public CTxCompressorMethods
{
private:
    CTxOutBaseRef &txout;

public:

    CTxOutBaseCompressor(CTxOutBaseRef &txoutIn) : txout(txoutIn) { }
    
    
    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        if (!ser_action.ForRead())
        {
            if (txout == nullptr)
            {
                uint8_t bv = OUTPUT_NULL;
                READWRITE(bv);
                return;
            };
            
            uint8_t bv = txout->nVersion & 0xFF;
            READWRITE(bv);
            
            switch (bv)
            {
                case OUTPUT_STANDARD:
                    {
                    std::shared_ptr<CTxOutStandard> p = std::dynamic_pointer_cast<CTxOutStandard>(txout);
                    
                    uint64_t nVal = CompressAmount(p->nValue);
                    READWRITE(VARINT(nVal));
                    
                    CScriptCompressor cscript(REF(p->scriptPubKey));
                    READWRITE(cscript);
                    }
                    break;
                case OUTPUT_CT:
                    {
                    std::shared_ptr<CTxOutCT> p = std::dynamic_pointer_cast<CTxOutCT>(txout);
                    
                    // TODO: need all fields?
                    CScriptCompressor cscript(REF(p->scriptPubKey));
                    READWRITE(cscript);
                    
                    s.write((char*)&p->commitment.data[0], 33);
                    }
                    break;
                default:
                    assert(false);
            };
        } else
        {
            uint8_t bv;
            READWRITE(bv);
            
            switch (bv)
            {
                case OUTPUT_NULL:
                    // do nothing
                    return;
                case OUTPUT_STANDARD:
                    {
                    std::shared_ptr<CTxOutStandard> p;
                    txout = p = MAKE_OUTPUT<CTxOutStandard>();
                    
                    uint64_t nVal = 0;
                    READWRITE(VARINT(nVal));
                    p->nValue = DecompressAmount(nVal);
                    
                    CScriptCompressor cscript(REF(p->scriptPubKey));
                    READWRITE(cscript);
                    }
                    break;
                case OUTPUT_CT:
                    {
                    std::shared_ptr<CTxOutCT> p;
                    txout = p = MAKE_OUTPUT<CTxOutCT>();
                    
                    // TODO: need all fields?
                    CScriptCompressor cscript(REF(p->scriptPubKey));
                    READWRITE(cscript);
                    
                    s.read((char*)&p->commitment.data[0], 33);
                    }
                    break;
                default:
                    assert(false);
            };
            txout->nVersion = bv;
        };
    }
    
};

#endif // BITCOIN_COMPRESSOR_H
