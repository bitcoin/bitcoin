#include "omnicore/script.h"

#include "amount.h"
#include "script/script.h"
#include "script/standard.h"
#include "serialize.h"
#include "utilstrencodings.h"

#include <boost/foreach.hpp>

#include <string>
#include <utility>
#include <vector>

/** The minimum transaction relay fee. */
extern CFeeRate minRelayTxFee;

/**
 * Determines the minimum output amount to be spent by an output, based on the
 * scriptPubKey size in relation to the minimum relay fee.
 *
 * @param scriptPubKey[in]  The scriptPubKey
 * @return The dust threshold value
 */
int64_t GetDustThreshold(const CScript& scriptPubKey)
{
    CTxOut txOut(0, scriptPubKey);

    return txOut.GetDustThreshold(minRelayTxFee);
}

/**
 * Identifies standard output types based on a scriptPubKey.
 *
 * Note: whichTypeRet is set to TX_NONSTANDARD, if no standard script was found.
 *
 * @param scriptPubKey[in]   The script
 * @param whichTypeRet[out]  The output type
 * @return True if a standard script was found
 */
bool GetOutputType(const CScript& scriptPubKey, txnouttype& whichTypeRet)
{
    std::vector<std::vector<unsigned char> > vSolutions;

    if (SafeSolver(scriptPubKey, whichTypeRet, vSolutions)) {
        return true;
    }
    whichTypeRet = TX_NONSTANDARD;

    return false;
}

/**
 * Extracts the pushed data as hex-encoded string from a script.
 *
 * @param script[in]      The script
 * @param vstrRet[out]    The extracted pushed data as hex-encoded string
 * @param fSkipFirst[in]  Whether the first push operation should be skipped (default: false)
 * @return True if the extraction was successful (result can be empty)
 */
bool GetScriptPushes(const CScript& script, std::vector<std::string>& vstrRet, bool fSkipFirst)
{
    int count = 0;
    CScript::const_iterator pc = script.begin();

    while (pc < script.end()) {
        opcodetype opcode;
        std::vector<unsigned char> data;
        if (!script.GetOp(pc, opcode, data))
            return false;
        if (0x00 <= opcode && opcode <= OP_PUSHDATA4)
            if (count++ || !fSkipFirst) vstrRet.push_back(HexStr(data));
    }

    return true;
}

/**
 * Returns public keys or hashes from scriptPubKey, for standard transaction types.
 *
 * Note: in contrast to the script/standard/Solver, this Solver is not affected by
 * user settings, and in particular any OP_RETURN size is considered as standard.
 *
 * @param scriptPubKey[in]    The script
 * @param typeRet[out]        The output type
 * @param vSolutionsRet[out]  The extracted public keys or hashes
 * @return True if a standard script was found
 */
bool SafeSolver(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<std::vector<unsigned char> >& vSolutionsRet)
{
    // Templates
    static std::multimap<txnouttype, CScript> mTemplates;
    if (mTemplates.empty())
    {
        // Standard tx, sender provides pubkey, receiver adds signature
        mTemplates.insert(std::make_pair(TX_PUBKEY, CScript() << OP_PUBKEY << OP_CHECKSIG));

        // Bitcoin address tx, sender provides hash of pubkey, receiver provides signature and pubkey
        mTemplates.insert(std::make_pair(TX_PUBKEYHASH, CScript() << OP_DUP << OP_HASH160 << OP_PUBKEYHASH << OP_EQUALVERIFY << OP_CHECKSIG));

        // Sender provides N pubkeys, receivers provides M signatures
        mTemplates.insert(std::make_pair(TX_MULTISIG, CScript() << OP_SMALLINTEGER << OP_PUBKEYS << OP_SMALLINTEGER << OP_CHECKMULTISIG));

        // Empty, provably prunable, data-carrying output
        mTemplates.insert(std::make_pair(TX_NULL_DATA, CScript() << OP_RETURN));
    }

    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash())
    {
        typeRet = TX_SCRIPTHASH;
        std::vector<unsigned char> hashBytes(scriptPubKey.begin()+2, scriptPubKey.begin()+22);
        vSolutionsRet.push_back(hashBytes);
        return true;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 2 && scriptPubKey[0] == OP_RETURN)
    {
        CScript script(scriptPubKey.begin()+1, scriptPubKey.end());
        if (script.IsPushOnly()) {
            typeRet = TX_NULL_DATA;
            return true;
        }
    }

    // Scan templates
    const CScript& script1 = scriptPubKey;
    BOOST_FOREACH(const PAIRTYPE(txnouttype, CScript)& tplate, mTemplates)
    {
        const CScript& script2 = tplate.second;
        vSolutionsRet.clear();

        opcodetype opcode1, opcode2;
        std::vector<unsigned char> vch1, vch2;

        // Compare
        CScript::const_iterator pc1 = script1.begin();
        CScript::const_iterator pc2 = script2.begin();
        while (true)
        {
            if (pc1 == script1.end() && pc2 == script2.end())
            {
                // Found a match
                typeRet = tplate.first;
                if (typeRet == TX_MULTISIG)
                {
                    // Additional checks for TX_MULTISIG:
                    unsigned char m = vSolutionsRet.front()[0];
                    unsigned char n = vSolutionsRet.back()[0];
                    if (m < 1 || n < 1 || m > n || vSolutionsRet.size()-2 != n)
                        return false;
                }
                return true;
            }
            if (!script1.GetOp(pc1, opcode1, vch1))
                break;
            if (!script2.GetOp(pc2, opcode2, vch2))
                break;

            // Template matching opcodes:
            if (opcode2 == OP_PUBKEYS)
            {
                while (vch1.size() >= 33 && vch1.size() <= 65)
                {
                    vSolutionsRet.push_back(vch1);
                    if (!script1.GetOp(pc1, opcode1, vch1))
                        break;
                }
                if (!script2.GetOp(pc2, opcode2, vch2))
                    break;
                // Normal situation is to fall through
                // to other if/else statements
            }

            if (opcode2 == OP_PUBKEY)
            {
                if (vch1.size() < 33 || vch1.size() > 65)
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_PUBKEYHASH)
            {
                if (vch1.size() != sizeof(uint160))
                    break;
                vSolutionsRet.push_back(vch1);
            }
            else if (opcode2 == OP_SMALLINTEGER)
            {   // Single-byte small integer pushed onto vSolutions
                if (opcode1 == OP_0 ||
                    (opcode1 >= OP_1 && opcode1 <= OP_16))
                {
                    char n = (char)CScript::DecodeOP_N(opcode1);
                    vSolutionsRet.push_back(std::vector<unsigned char>(1, n));
                }
                else
                    break;
            }
            else if (opcode1 != opcode2 || vch1 != vch2)
            {
                // Others must match exactly
                break;
            }
        }
    }

    vSolutionsRet.clear();
    typeRet = TX_NONSTANDARD;
    return false;
}
