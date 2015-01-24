// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include "policy/policy.h"

#include "amount.h"
#include "coins.h"
#include "consensus/validation.h"
#include "policy/feerate.h"
#include "primitives/transaction.h"
#include "tinyformat.h"
#include "ui_interface.h"
#include "util.h"
#include "utilstrencodings.h"

/** Fees smaller than this (in satoshi) are considered zero fee (for relaying and mining) */
CFeeRate minRelayTxFee = CFeeRate(1000);
/** The maximum number of bytes in OP_RETURN outputs that we're willing to relay/mine */
static const unsigned int MAX_OP_RETURN_RELAY = 80;

/** Declaration of Standard Policy implementing CPolicy */
class CStandardPolicy : public CPolicy
{
protected:
    unsigned nMaxDatacarrierBytes;
    bool fIsBareMultisigStd;
public:
    CStandardPolicy() : nMaxDatacarrierBytes(MAX_OP_RETURN_RELAY),
                        fIsBareMultisigStd(true) {};

    virtual std::vector<std::pair<std::string, std::string> > GetOptionsHelp() const;
    virtual void InitFromArgs(const std::map<std::string, std::string>&);
    virtual bool ApproveScript(const CScript&, txnouttype&) const;
    /**
     * "Dust" is defined in terms of CTransaction::minRelayTxFee,
     * which has units satoshis-per-kilobyte.
     * If you'd pay more than 1/3 in fees
     * to spend something, then we consider it dust.
     * A typical txout is 34 bytes big, and will
     * need a CTxIn of at least 148 bytes to spend:
     * so dust is a txout less than 546 satoshis 
     * with default minRelayTxFee.
     */
    virtual CAmount GetDustThreshold(const CTxOut& txout) const;
    virtual bool ApproveOutput(const CTxOut& txout) const;
    virtual bool ApproveTx(const CTransaction&, CValidationState&) const;
    /**
     * Check transaction inputs to mitigate two
     * potential denial-of-service attacks:
     * 
     * 1. scriptSigs with extra data stuffed into them,
     *    not consumed by scriptPubKey (or P2SH script)
     * 2. P2SH scripts with a crazy number of expensive
     *    CHECKSIG/CHECKMULTISIG operations
     */
    virtual bool ApproveTxInputs(const CTransaction& tx, const CCoinsViewEfficient& mapInputs) const;
    virtual bool ApproveTxInputsScripts(const CTransaction&, CValidationState&, const CCoinsViewEfficient&, bool cacheStore) const;
};

/** Global variables and their interfaces */

static CStandardPolicy standardPolicy;

static CPolicy* pCurrentPolicy = 0;

CPolicy& Policy(std::string policy)
{
    if (policy == "standard")
        return standardPolicy;
    throw std::runtime_error(strprintf(_("Unknown policy '%s'"), policy));
}

void SelectPolicy(std::string policy)
{
    pCurrentPolicy = &Policy(policy);
}

const CPolicy& Policy()
{
    assert(pCurrentPolicy);
    return *pCurrentPolicy;
}

std::string GetPolicyUsageStr(std::string selectedPolicy)
{
    CPolicy& policy = standardPolicy;
    try {
        policy = Policy(selectedPolicy);
    } catch(std::exception &e) {
        selectedPolicy = "standard";
    }
    std::string strUsage = HelpMessageGroup(strprintf(_("Policy options (for policy: %s):"), selectedPolicy));
    strUsage += HelpMessageOpt("-policy", strprintf(_("Select a specific type of policy (default: %s)"), "standard"));
    strUsage += HelpMessagesOpt(policy.GetOptionsHelp());
    return strUsage;
}

void InitPolicyFromArgs(const std::map<std::string, std::string>& mapArgs)
{
    SelectPolicy(GetArg("-policy", "standard", mapArgs));
    pCurrentPolicy->InitFromArgs(mapArgs);
}

/** CStandardPolicy implementation */

std::vector<std::pair<std::string, std::string> > CStandardPolicy::GetOptionsHelp() const
{
    std::vector<std::pair<std::string, std::string> > optionsHelp;
    optionsHelp.push_back(std::make_pair("-datacarrier", strprintf(_("Relay and mine data carrier transactions (default: %u)"), 1)));
    optionsHelp.push_back(std::make_pair("-datacarriersize", strprintf(_("Maximum size of data in data carrier transactions we relay and mine (default: %u)"), MAX_OP_RETURN_RELAY)));
    optionsHelp.push_back(std::make_pair("-permitbaremultisig", strprintf(_("Relay non-P2SH multisig (default: %u)"), 1)));
    return optionsHelp;
}

void CStandardPolicy::InitFromArgs(const std::map<std::string, std::string>& mapArgs)
{
    if (GetArg("-datacarrier", true, mapArgs))
        nMaxDatacarrierBytes = GetArg("-datacarriersize", nMaxDatacarrierBytes, mapArgs);
    else
        nMaxDatacarrierBytes = 0;
    fIsBareMultisigStd = GetArg("-permitbaremultisig", fIsBareMultisigStd, mapArgs);
}

bool CStandardPolicy::ApproveScript(const CScript& scriptPubKey, txnouttype& whichType) const
{
    std::vector<std::vector<unsigned char> > vSolutions;
    if (!Solver(scriptPubKey, whichType, vSolutions))
        return false;

    switch (whichType)
    {
        case TX_MULTISIG:
        {
            unsigned char m = vSolutions.front()[0];
            unsigned char n = vSolutions.back()[0];
            // Support up to x-of-3 multisig txns as standard
            if (n < 1 || n > 3)
                return false;
            if (m < 1 || m > n)
                return false;
            break;
        }

        case TX_NULL_DATA:
            // TX_NULL_DATA without any vSolutions is a lone OP_RETURN, which traditionally is accepted regardless of the -datacarrier option, so we skip the check.
            // If you want to filter lone OP_RETURNs, be sure to handle vSolutions being empty below where vSolutions.front() is accessed!
            if (vSolutions.size())
            {
                if (!nMaxDatacarrierBytes)
                    return false;

                if (vSolutions.front().size() > nMaxDatacarrierBytes)
                    return false;
            }

            break;

        default:
            // no other restrictions on standard scripts
            break;
    }

    return whichType != TX_NONSTANDARD;
}

CAmount CStandardPolicy::GetDustThreshold(const CTxOut& txout) const
{
    size_t nSize = txout.GetSerializeSize(SER_DISK,0) + 148u;
    return 3 * minRelayTxFee.GetFee(nSize);
}

bool CStandardPolicy::ApproveOutput(const CTxOut& txout) const
{
    return txout.nValue < GetDustThreshold(txout);
}

bool CStandardPolicy::ApproveTx(const CTransaction& tx, CValidationState& state) const
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

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        // Biggest 'standard' txin is a 15-of-15 P2SH multisig with compressed
        // keys. (remember the 520 byte limit on redeemScript size) That works
        // out to a (15*(33+1))+3=513 byte redeemScript, 513+1+15*(73+1)+3=1627
        // bytes of scriptSig, which we round off to 1650 bytes for some minor
        // future-proofing. That's also enough to spend a 20-of-20
        // CHECKMULTISIG scriptPubKey, though such a scriptPubKey is not
        // considered standard)
        if (tx.vin[i].scriptSig.size() > 1650)
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptsig-size");

        if (!tx.vin[i].scriptSig.IsPushOnly())
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptsig-not-pushonly");
    }

    unsigned int nDataOut = 0;
    txnouttype whichType;
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        if (!ApproveScript(tx.vout[i].scriptPubKey, whichType))
            return state.DoS(0, false, REJECT_NONSTANDARD, "scriptpubkey");

        if (whichType == TX_NULL_DATA)
            nDataOut++;
        else if ((whichType == TX_MULTISIG) && (!fIsBareMultisigStd))
            return state.DoS(0, false, REJECT_NONSTANDARD, "bare-multisig");
        else if (ApproveOutput(tx.vout[i]))
            return state.DoS(0, false, REJECT_NONSTANDARD, "dust");
    }

    // only one OP_RETURN txout is permitted
    if (nDataOut > 1)
        return state.DoS(0, false, REJECT_NONSTANDARD, "multi-op-return");

    return true;
}

bool CStandardPolicy::ApproveTxInputs(const CTransaction& tx, const CCoinsViewEfficient& mapInputs) const
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
            return false;
        int nArgsExpected = ScriptSigArgsExpected(whichType, vSolutions);
        if (nArgsExpected < 0)
            return false;

        // Transactions with extra stuff in their scriptSigs are
        // non-standard. Note that this EvalScript() call will
        // be quick, because if there are any operations
        // beside "push data" in the scriptSig
        // Policy().ValidateTx() will have already returned false
        // and this method isn't called.
        std::vector<std::vector<unsigned char> > stack;
        if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker()))
            return false;

        if (whichType == TX_SCRIPTHASH)
        {
            if (stack.empty())
                return false;
            CScript subscript(stack.back().begin(), stack.back().end());
            std::vector<std::vector<unsigned char> > vSolutions2;
            txnouttype whichType2;
            if (Solver(subscript, whichType2, vSolutions2))
            {
                int tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
                if (tmpExpected < 0)
                    return false;
                nArgsExpected += tmpExpected;
            }
            else
            {
                // Any other Script with less than 15 sigops OK:
                unsigned int sigops = subscript.GetSigOpCount(true);
                // ... extra data left on the stack after execution is OK, too:
                return (sigops <= MAX_P2SH_SIGOPS);
            }
        }

        if (stack.size() != (unsigned int)nArgsExpected)
            return false;
    }

    return true;
}

bool CStandardPolicy::ApproveTxInputsScripts(const CTransaction& tx, CValidationState& state, const CCoinsViewEfficient& view, bool cacheStore) const
{
    // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
    // Helps prevent CPU exhaustion attacks.

    // Skip ECDSA signature verification when connecting blocks
    // before the last block chain checkpoint. This is safe because block merkle hashes are
    // still computed and checked, and any change will be caught at the next checkpoint.
    if (!Consensus::CheckTxInputsScripts(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS))
        // Failures of non-policy flags indicate a transaction that is
        // invalid in new blocks, e.g. a invalid P2SH. We DoS ban
        // such nodes as they are not following the protocol. That
        // said during an upgrade careful thought should be taken
        // as to the correct behavior - we may want to continue
        // peering with non-upgraded nodes even after a soft-fork
        // super-majority vote has passed.
        return state.DoS(100,false, REJECT_INVALID, strprintf("with flags: MANDATORY (%s)", state.GetRejectReason()));

    // Check again against just the non-consensus-critical policy but
    // not mandatory script verification flags, such as
    // non-standard DER encodings or non-null dummy
    // arguments; if so, don't trigger DoS protection to
    // avoid splitting the network between upgraded and
    // non-upgraded nodes.
    // This is done later in case of bugs in the standard flags that cause
    // transactions to pass as valid when they're actually invalid. For
    // instance the STRICTENC flag was incorrectly allowing certain
    // CHECKSIG NOT scripts to pass, even though they were invalid.
    if (!Consensus::CheckTxInputsScripts(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS | STANDARD_NOT_MANDATORY_VERIFY_FLAGS))
        return state.Invalid(false, REJECT_NONSTANDARD, strprintf("with flags: STANDARD_NOT_MANDATORY (%s)", state.GetRejectReason()));

    return true;
}
