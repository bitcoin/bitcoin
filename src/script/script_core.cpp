// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "script/interpreter_core.h"

#include "core.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"

namespace {

/** Wrapper that serializes like CTransaction, but with the modifications
 *  required for the signature hash done in-place
 */
class CTransactionSignatureSerializer {
private:
    const CTransaction &txTo;  // reference to the spending transaction (the one being serialized)
    const CScript &scriptCode; // output script being consumed
    const unsigned int nIn;    // input index of txTo being signed
    const bool fAnyoneCanPay;  // whether the hashtype has the SIGHASH_ANYONECANPAY flag set
    const bool fHashSingle;    // whether the hashtype is SIGHASH_SINGLE
    const bool fHashNone;      // whether the hashtype is SIGHASH_NONE

public:
    CTransactionSignatureSerializer(const CTransaction &txToIn, const CScript &scriptCodeIn, unsigned int nInIn, int nHashTypeIn) :
        txTo(txToIn), scriptCode(scriptCodeIn), nIn(nInIn),
        fAnyoneCanPay(!!(nHashTypeIn & SIGHASH_ANYONECANPAY)),
        fHashSingle((nHashTypeIn & 0x1f) == SIGHASH_SINGLE),
        fHashNone((nHashTypeIn & 0x1f) == SIGHASH_NONE) {}

    /** Serialize the passed scriptCode, skipping OP_CODESEPARATORs */
    template<typename S>
    void SerializeScriptCode(S &s, int nType, int nVersion) const {
        CScript::const_iterator it = scriptCode.begin();
        CScript::const_iterator itBegin = it;
        opcodetype opcode;
        unsigned int nCodeSeparators = 0;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR)
                nCodeSeparators++;
        }
        ::WriteCompactSize(s, scriptCode.size() - nCodeSeparators);
        it = itBegin;
        while (scriptCode.GetOp(it, opcode)) {
            if (opcode == OP_CODESEPARATOR) {
                s.write((char*)&itBegin[0], it-itBegin-1);
                itBegin = it;
            }
        }
        s.write((char*)&itBegin[0], it-itBegin);
    }

    /** Serialize an input of txTo */
    template<typename S>
    void SerializeInput(S &s, unsigned int nInput, int nType, int nVersion) const {
        // In case of SIGHASH_ANYONECANPAY, only the input being signed is serialized
        if (fAnyoneCanPay)
            nInput = nIn;
        // Serialize the prevout
        ::Serialize(s, txTo.vin[nInput].prevout, nType, nVersion);
        // Serialize the script
        if (nInput != nIn)
            // Blank out other inputs' signatures
            ::Serialize(s, CScript(), nType, nVersion);
        else
            SerializeScriptCode(s, nType, nVersion);
        // Serialize the nSequence
        if (nInput != nIn && (fHashSingle || fHashNone))
            // let the others update at will
            ::Serialize(s, (int)0, nType, nVersion);
        else
            ::Serialize(s, txTo.vin[nInput].nSequence, nType, nVersion);
    }

    /** Serialize an output of txTo */
    template<typename S>
    void SerializeOutput(S &s, unsigned int nOutput, int nType, int nVersion) const {
        if (fHashSingle && nOutput != nIn)
            // Do not lock-in the txout payee at other indices as txin
            ::Serialize(s, CTxOut(), nType, nVersion);
        else
            ::Serialize(s, txTo.vout[nOutput], nType, nVersion);
    }

    /** Serialize txTo */
    template<typename S>
    void Serialize(S &s, int nType, int nVersion) const {
        // Serialize nVersion
        ::Serialize(s, txTo.nVersion, nType, nVersion);
        // Serialize vin
        unsigned int nInputs = fAnyoneCanPay ? 1 : txTo.vin.size();
        ::WriteCompactSize(s, nInputs);
        for (unsigned int nInput = 0; nInput < nInputs; nInput++)
             SerializeInput(s, nInput, nType, nVersion);
        // Serialize vout
        unsigned int nOutputs = fHashNone ? 0 : (fHashSingle ? nIn+1 : txTo.vout.size());
        ::WriteCompactSize(s, nOutputs);
        for (unsigned int nOutput = 0; nOutput < nOutputs; nOutput++)
             SerializeOutput(s, nOutput, nType, nVersion);
        // Serialie nLockTime
        ::Serialize(s, txTo.nLockTime, nType, nVersion);
    }
};

} // anon namespace

uint256 CTxSignatureSerializer::SignatureHash(const CScript& scriptCode, int nHashType) const
{
    if (nIn >= txTo.vin.size()) {
        LogPrintf("ERROR: SignatureHash() : nIn=%d out of range\n", nIn);
        return 1;
    }

    // Check for invalid use of SIGHASH_SINGLE
    if ((nHashType & 0x1f) == SIGHASH_SINGLE) {
        if (nIn >= txTo.vout.size()) {
            LogPrintf("ERROR: SignatureHash() : nOut=%d out of range\n", nIn);
            return 1;
        }
    }

    // Wrapper to serialize only the necessary parts of the transaction being signed
    CTransactionSignatureSerializer txTmp(txTo, scriptCode, nIn, nHashType);

    // Serialize and hash
    CHashWriter ss(SER_GETHASH, 0);
    ss << txTmp << nHashType;
    return ss.GetHash();
}

uint256 SignatureHash(const CScript& scriptCode, const CTransaction& txTo, unsigned int nIn, int nHashType)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return tx.SignatureHash(scriptCode, nHashType);
}

bool CheckSig(std::vector<unsigned char> vchSig, const std::vector<unsigned char>& vchPubKey, const CScript &scriptCode, const CTransaction& txTo, unsigned int nIn, int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return CheckSig(vchSig, vchPubKey, scriptCode, tx, flags);
}

bool EvalScript(std::vector<std::vector<unsigned char> >& stack, const CScript& script, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return EvalScript(stack, script, tx, flags);
}

bool VerifyScript(const CScript& scriptSig, const CScript& scriptPubKey, const CTransaction& txTo, unsigned int nIn, unsigned int flags)
{
    CTxSignatureSerializer tx(txTo, nIn);
    return VerifyScript(scriptSig, scriptPubKey, tx, flags);
}
