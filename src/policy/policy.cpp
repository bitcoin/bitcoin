// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include "policy/policy.h"

#include "main.h"
#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <boost/foreach.hpp>

    /**
     * Check transaction inputs to mitigate two
     * potential denial-of-service attacks:
     * 
     * 1. scriptSigs with extra data stuffed into them,
     *    not consumed by scriptPubKey (or P2SH script)
     * 2. P2SH scripts with a crazy number of expensive
     *    CHECKSIG/CHECKMULTISIG operations
     *
     * Check transaction inputs, and make sure any
     * pay-to-script-hash transactions are evaluating IsStandard scripts
     * 
     * Why bother? To avoid denial-of-service attacks; an attacker
     * can submit a standard HASH... OP_EQUAL transaction,
     * which will get accepted into blocks. The redemption
     * script can be anything; an attacker could use a very
     * expensive-to-check-upon-redemption script like:
     *   DUP CHECKSIG DROP ... repeated 100 times... OP_1
     */

bool IsStandard(const CScript& scriptPubKey, CValidationState& state, txnouttype& whichType)
{
    std::vector<std::vector<unsigned char> > vSolutions;
    if (!Solver(scriptPubKey, whichType, vSolutions) || whichType == TX_NONSTANDARD)
        return state.DoS(0, false, REJECT_NONSTANDARD, "scriptpubkey-unkown-pattern");

    if (whichType == TX_MULTISIG)
    {
        unsigned char m = vSolutions.front()[0];
        unsigned char n = vSolutions.back()[0];
        // Support up to x-of-3 multisig txns as standard
        if (n < 1 || n > 3)
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptpubkey-multisig-n-outofrange");
        if (m < 1 || m > n)
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptpubkey-multisig-m-outofrange");
    }

    return true;
}

bool IsStandardTx(const CTransaction& tx, CValidationState& state)
{
    if (tx.nVersion > CTransaction::CURRENT_VERSION || tx.nVersion < 1)
        return state.DoS(0, false, REJECT_NONSTANDARD, "version");

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_SIZE mitigates CPU exhaustion attacks.
    unsigned int sz = tx.GetSerializeSize(SER_NETWORK, CTransaction::CURRENT_VERSION);
    if (sz >= MAX_STANDARD_TX_SIZE)
        return state.DoS(0, false, REJECT_NONSTANDARD, "tx-size");

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        // Biggest 'standard' txin is a 15-of-15 P2SH multisig with compressed
        // keys. (remember the 520 byte limit on redeemScript size) That works
        // out to a (15*(33+1))+3=513 byte redeemScript, 513+1+15*(73+1)+3=1627
        // bytes of scriptSig, which we round off to 1650 bytes for some minor
        // future-proofing. That's also enough to spend a 20-of-20
        // CHECKMULTISIG scriptPubKey, though such a scriptPubKey is not
        // considered standard)
        if (txin.scriptSig.size() > 1650)
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptsig-size");

        if (!txin.scriptSig.IsPushOnly())
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptsig-not-pushonly");
    }

    unsigned int nDataOut = 0;
    txnouttype whichType;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        if (!IsStandard(txout.scriptPubKey, state, whichType))
            return false;

        if (whichType == TX_NULL_DATA)
            nDataOut++;
        else if ((whichType == TX_MULTISIG) && (!fIsBareMultisigStd))
            return state.DoS(0, false, REJECT_NONSTANDARD, "bare-multisig");
        else if (txout.IsDust(::minRelayTxFee))
            return state.DoS(0, false, REJECT_NONSTANDARD, "dust");
    }

    // only one OP_RETURN txout is permitted
    if (nDataOut > 1)
        return state.DoS(0, false, REJECT_NONSTANDARD, "multi-op-return");

    return true;
}

bool AreInputsStandard(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& mapInputs)
{
    if (tx.IsCoinBase())
        return true; // Coinbases don't use vin normally

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut& prev = mapInputs.GetOutputFor(tx.vin[i]);

        std::vector<std::vector<unsigned char> > vSolutions;
        txnouttype whichType;
        // get the scriptPubKey corresponding to this input:
        const CScript& prevScript = prev.scriptPubKey;
        if (!Solver(prevScript, whichType, vSolutions))
            return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-prev-scriptpubkey-unkown-pattern");
        int nArgsExpected = ScriptSigArgsExpected(whichType, vSolutions);
        if (nArgsExpected < 0)
            return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-prev-scriptpubkey-no-args-expected");

        // Transactions with extra stuff in their scriptSigs are
        // non-standard. Note that this EvalScript() call will
        // be quick, because if there are any operations
        // beside "push data" in the scriptSig
        // IsStandardTx() will have already returned false
        // and this method isn't called.
        std::vector<std::vector<unsigned char> > stack;
        if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker()))
            return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-scriptsig-eval-fail");

        if (whichType == TX_SCRIPTHASH)
        {
            if (stack.empty())
                return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-scriptsig-p2sh-invalid");
            CScript subscript(stack.back().begin(), stack.back().end());
            std::vector<std::vector<unsigned char> > vSolutions2;
            txnouttype whichType2;
            if (Solver(subscript, whichType2, vSolutions2))
            {
                int tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
                if (tmpExpected < 0)
                    return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-scriptsig-p2sh-invalid");
                nArgsExpected += tmpExpected;
            }
            else
            {
                // Any other Script with less than 15 sigops OK:
                unsigned int sigops = subscript.GetSigOpCount(true);
                // ... extra data left on the stack after execution is OK, too:
                if (sigops > MAX_P2SH_SIGOPS)
                    return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-scriptsig-p2sh-toolong");
                else 
                    return true;
            }
        }

        if (stack.size() != (unsigned int)nArgsExpected)
            return state.DoS(0, false, REJECT_NONSTANDARD, "txinputs-scriptsig-non-empty-stack");
    }

    return true;
}
