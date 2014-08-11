// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef H_BITCOIN_SCRIPTUTILS
#define H_BITCOIN_SCRIPTUTILS

#include "key.h"
#include "util.h"
#include "script/script.h"

#include <stdexcept>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/variant.hpp>

class CCoins;
class CKeyStore;
class CTransaction;
struct CMutableTransaction;

static const unsigned int MAX_SCRIPT_ELEMENT_SIZE = 520; // bytes
static const unsigned int MAX_OP_RETURN_RELAY = 40;      // bytes

/** IsMine() return codes */
enum isminetype
{
    ISMINE_NO = 0,
    ISMINE_WATCH_ONLY = 1,
    ISMINE_SPENDABLE = 2,
    ISMINE_ALL = ISMINE_WATCH_ONLY | ISMINE_SPENDABLE
};
/** used for bitflags of isminetype */
typedef uint8_t isminefilter;

// For convenience, standard but not mandatory verify flags.
static const unsigned int STANDARD_NOT_MANDATORY_VERIFY_FLAGS = STANDARD_SCRIPT_VERIFY_FLAGS & ~MANDATORY_SCRIPT_VERIFY_FLAGS;

enum txnouttype
{
    TX_NONSTANDARD,
    // 'standard' transaction types:
    TX_PUBKEY,
    TX_PUBKEYHASH,
    TX_SCRIPTHASH,
    TX_MULTISIG,
    TX_NULL_DATA,
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

/** A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * CKeyID: TX_PUBKEYHASH destination
 *  * CScriptID: TX_SCRIPTHASH destination
 *  A CTxDestination is the internal data type encoded in a CBitcoinAddress
 */
typedef boost::variant<CNoDestination, CKeyID, CScriptID> CTxDestination;

const char* GetTxnOutputType(txnouttype t);


const char* GetOpName(opcodetype opcode);



inline std::string ValueString(const std::vector<unsigned char>& vch)
{
    if (vch.size() <= 4)
        return strprintf("%d", CScriptNum(vch).getint());
    else
        return HexStr(vch);
}

inline std::string StackString(const std::vector<std::vector<unsigned char> >& vStack)
{
    std::string str;
    BOOST_FOREACH(const std::vector<unsigned char>& vch, vStack)
    {
        if (!str.empty())
            str += " ";
        str += ValueString(vch);
    }
    return str;
}








/** Serialized script, used inside transaction inputs and outputs */
class CScript : public std::vector<unsigned char>
{
protected:
    CScript& push_int64(int64_t n)
    {
        if (n == -1 || (n >= 1 && n <= 16))
        {
            push_back(n + (OP_1 - 1));
        }
        else
        {
            *this << CScriptNum::serialize(n);
        }
        return *this;
    }
public:
    CScript() { }
    CScript(const CScript& b) : std::vector<unsigned char>(b.begin(), b.end()) { }
    CScript(const_iterator pbegin, const_iterator pend) : std::vector<unsigned char>(pbegin, pend) { }
#ifndef _MSC_VER
    CScript(const unsigned char* pbegin, const unsigned char* pend) : std::vector<unsigned char>(pbegin, pend) { }
#endif

    CScript& operator+=(const CScript& b)
    {
        insert(end(), b.begin(), b.end());
        return *this;
    }

    friend CScript operator+(const CScript& a, const CScript& b)
    {
        CScript ret = a;
        ret += b;
        return ret;
    }


    CScript(int64_t b)        { operator<<(b); }

    explicit CScript(opcodetype b)     { operator<<(b); }
    explicit CScript(const uint256& b) { operator<<(b); }
    explicit CScript(const CScriptNum& b) { operator<<(b); }
    explicit CScript(const std::vector<unsigned char>& b) { operator<<(b); }


    CScript& operator<<(int64_t b) { return push_int64(b); }

    CScript& operator<<(opcodetype opcode)
    {
        if (opcode < 0 || opcode > 0xff)
            throw std::runtime_error("CScript::operator<<() : invalid opcode");
        insert(end(), (unsigned char)opcode);
        return *this;
    }

    CScript& operator<<(const uint160& b)
    {
        insert(end(), sizeof(b));
        insert(end(), (unsigned char*)&b, (unsigned char*)&b + sizeof(b));
        return *this;
    }

    CScript& operator<<(const uint256& b)
    {
        insert(end(), sizeof(b));
        insert(end(), (unsigned char*)&b, (unsigned char*)&b + sizeof(b));
        return *this;
    }

    CScript& operator<<(const CPubKey& key)
    {
        assert(key.size() < OP_PUSHDATA1);
        insert(end(), (unsigned char)key.size());
        insert(end(), key.begin(), key.end());
        return *this;
    }

    CScript& operator<<(const CScriptNum& b)
    {
        *this << b.getvch();
        return *this;
    }

    CScript& operator<<(const std::vector<unsigned char>& b)
    {
        if (b.size() < OP_PUSHDATA1)
        {
            insert(end(), (unsigned char)b.size());
        }
        else if (b.size() <= 0xff)
        {
            insert(end(), OP_PUSHDATA1);
            insert(end(), (unsigned char)b.size());
        }
        else if (b.size() <= 0xffff)
        {
            insert(end(), OP_PUSHDATA2);
            unsigned short nSize = b.size();
            insert(end(), (unsigned char*)&nSize, (unsigned char*)&nSize + sizeof(nSize));
        }
        else
        {
            insert(end(), OP_PUSHDATA4);
            unsigned int nSize = b.size();
            insert(end(), (unsigned char*)&nSize, (unsigned char*)&nSize + sizeof(nSize));
        }
        insert(end(), b.begin(), b.end());
        return *this;
    }

    CScript& operator<<(const CScript& b)
    {
        // I'm not sure if this should push the script or concatenate scripts.
        // If there's ever a use for pushing a script onto a script, delete this member fn
        assert(!"Warning: Pushing a CScript onto a CScript with << is probably not intended, use + to concatenate!");
        return *this;
    }


    bool GetOp(iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>& vchRet)
    {
         // Wrapper so it can be called with either iterator or const_iterator
         const_iterator pc2 = pc;
         bool fRet = GetOp2(pc2, opcodeRet, &vchRet);
         pc = begin() + (pc2 - begin());
         return fRet;
    }

    bool GetOp(iterator& pc, opcodetype& opcodeRet)
    {
         const_iterator pc2 = pc;
         bool fRet = GetOp2(pc2, opcodeRet, NULL);
         pc = begin() + (pc2 - begin());
         return fRet;
    }

    bool GetOp(const_iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>& vchRet) const
    {
        return GetOp2(pc, opcodeRet, &vchRet);
    }

    bool GetOp(const_iterator& pc, opcodetype& opcodeRet) const
    {
        return GetOp2(pc, opcodeRet, NULL);
    }

    bool GetOp2(const_iterator& pc, opcodetype& opcodeRet, std::vector<unsigned char>* pvchRet) const
    {
        opcodeRet = OP_INVALIDOPCODE;
        if (pvchRet)
            pvchRet->clear();
        if (pc >= end())
            return false;

        // Read instruction
        if (end() - pc < 1)
            return false;
        unsigned int opcode = *pc++;

        // Immediate operand
        if (opcode <= OP_PUSHDATA4)
        {
            unsigned int nSize = 0;
            if (opcode < OP_PUSHDATA1)
            {
                nSize = opcode;
            }
            else if (opcode == OP_PUSHDATA1)
            {
                if (end() - pc < 1)
                    return false;
                nSize = *pc++;
            }
            else if (opcode == OP_PUSHDATA2)
            {
                if (end() - pc < 2)
                    return false;
                nSize = 0;
                memcpy(&nSize, &pc[0], 2);
                pc += 2;
            }
            else if (opcode == OP_PUSHDATA4)
            {
                if (end() - pc < 4)
                    return false;
                memcpy(&nSize, &pc[0], 4);
                pc += 4;
            }
            if (end() - pc < 0 || (unsigned int)(end() - pc) < nSize)
                return false;
            if (pvchRet)
                pvchRet->assign(pc, pc + nSize);
            pc += nSize;
        }

        opcodeRet = (opcodetype)opcode;
        return true;
    }

    // Encode/decode small integers:
    static int DecodeOP_N(opcodetype opcode)
    {
        if (opcode == OP_0)
            return 0;
        assert(opcode >= OP_1 && opcode <= OP_16);
        return (int)opcode - (int)(OP_1 - 1);
    }
    static opcodetype EncodeOP_N(int n)
    {
        assert(n >= 0 && n <= 16);
        if (n == 0)
            return OP_0;
        return (opcodetype)(OP_1+n-1);
    }

    int FindAndDelete(const CScript& b)
    {
        int nFound = 0;
        if (b.empty())
            return nFound;
        iterator pc = begin();
        opcodetype opcode;
        do
        {
            while (end() - pc >= (long)b.size() && memcmp(&pc[0], &b[0], b.size()) == 0)
            {
                erase(pc, pc + b.size());
                ++nFound;
            }
        }
        while (GetOp(pc, opcode));
        return nFound;
    }
    int Find(opcodetype op) const
    {
        int nFound = 0;
        opcodetype opcode;
        for (const_iterator pc = begin(); pc != end() && GetOp(pc, opcode);)
            if (opcode == op)
                ++nFound;
        return nFound;
    }

    // Pre-version-0.6, Bitcoin always counted CHECKMULTISIGs
    // as 20 sigops. With pay-to-script-hash, that changed:
    // CHECKMULTISIGs serialized in scriptSigs are
    // counted more accurately, assuming they are of the form
    //  ... OP_N CHECKMULTISIG ...
    unsigned int GetSigOpCount(bool fAccurate) const;

    // Accurately count sigOps, including sigOps in
    // pay-to-script-hash transactions:
    unsigned int GetSigOpCount(const CScript& scriptSig) const;

    bool IsPayToScriptHash() const;

    // Called by IsStandardTx and P2SH VerifyScript (which makes it consensus-critical).
    bool IsPushOnly() const;

    // Called by IsStandardTx.
    bool HasCanonicalPushes() const;

    // Returns whether the script is guaranteed to fail at execution,
    // regardless of the initial stack. This allows outputs to be pruned
    // instantly when entering the UTXO set.
    bool IsUnspendable() const
    {
        return (size() > 0 && *begin() == OP_RETURN);
    }

    void SetDestination(const CTxDestination& address);
    void SetMultisig(int nRequired, const std::vector<CPubKey>& keys);

    std::string ToString() const
    {
        std::string str;
        opcodetype opcode;
        std::vector<unsigned char> vch;
        const_iterator pc = begin();
        while (pc < end())
        {
            if (!str.empty())
                str += " ";
            if (!GetOp(pc, opcode, vch))
            {
                str += "[error]";
                return str;
            }
            if (0 <= opcode && opcode <= OP_PUSHDATA4)
                str += ValueString(vch);
            else
                str += GetOpName(opcode);
        }
        return str;
    }

    CScriptID GetID() const
    {
        return CScriptID(Hash160(*this));
    }
};

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
    // make this static for now (there are only 6 special scripts defined)
    // this can potentially be extended together with a new nVersion for
    // transactions, in which case this value becomes dependent on nVersion
    // and nHeight of the enclosing transaction.
    static const unsigned int nSpecialScripts = 6;

    CScript &script;
protected:
    // These check for scripts for which a special case with a shorter encoding is defined.
    // They are implemented separately from the CScript test, as these test for exact byte
    // sequence correspondences, and are more strict. For example, IsToPubKey also verifies
    // whether the public key is valid (as invalid ones cannot be represented in compressed
    // form).
    bool IsToKeyID(CKeyID &hash) const;
    bool IsToScriptID(CScriptID &hash) const;
    bool IsToPubKey(CPubKey &pubkey) const;

    bool Compress(std::vector<unsigned char> &out) const;
    unsigned int GetSpecialSize(unsigned int nSize) const;
    bool Decompress(unsigned int nSize, const std::vector<unsigned char> &out);
public:
    CScriptCompressor(CScript &scriptIn) : script(scriptIn) { }

    unsigned int GetSerializeSize(int nType, int nVersion) const {
        std::vector<unsigned char> compr;
        if (Compress(compr))
            return compr.size();
        unsigned int nSize = script.size() + nSpecialScripts;
        return script.size() + VARINT(nSize).GetSerializeSize(nType, nVersion);
    }

    template<typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
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
    void Unserialize(Stream &s, int nType, int nVersion) {
        unsigned int nSize = 0;
        s >> VARINT(nSize);
        if (nSize < nSpecialScripts) {
            std::vector<unsigned char> vch(GetSpecialSize(nSize), 0x00);
            s >> REF(CFlatData(vch));
            Decompress(nSize, vch);
            return;
        }
        nSize -= nSpecialScripts;
        script.resize(nSize);
        s >> REF(CFlatData(script));
    }
};

bool IsCanonicalPubKey(const std::vector<unsigned char> &vchPubKey, unsigned int flags);
bool IsCanonicalSignature(const std::vector<unsigned char> &vchSig, unsigned int flags);

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);
uint256 SignatureHash(const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType);
bool Solver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet);
int ScriptSigArgsExpected(txnouttype t, const std::vector<std::vector<unsigned char> >& vSolutions);
bool IsStandard(const CScript& scriptPubKey, txnouttype& whichType);
isminetype IsMine(const CKeyStore& keystore, const CScript& scriptPubKey);
isminetype IsMine(const CKeyStore& keystore, const CTxDestination& dest);
void ExtractAffectedKeys(const CKeyStore &keystore, const CScript& scriptPubKey, std::vector<CKeyID> &vKeys);
bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet);
bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet);
bool SignSignature(const CKeyStore& keystore, const CScript& fromPubKey, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
bool SignSignature(const CKeyStore& keystore, const CTransaction& txFrom, CMutableTransaction& txTo, unsigned int nIn, int nHashType=SIGHASH_ALL);
bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType);

// Given two sets of signatures for scriptPubKey, possibly with OP_0 placeholders,
// combine them intelligently and return the result.
CScript CombineSignatures(CScript scriptPubKey, const CTransaction& txTo, unsigned int nIn, const CScript& scriptSig1, const CScript& scriptSig2);

#endif
