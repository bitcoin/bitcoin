// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// NOTE: This file is intended to be customised by the end user, and includes only local node policy logic

#include <policy/policy.h>

#include <coins.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <serialize.h>
#include <span.h>

#include <algorithm>
#include <cstddef>
#include <vector>

CAmount GetDustThreshold(const CTxOut& txout, const CFeeRate& dustRelayFeeIn)
{
    // "Dust" is defined in terms of dustRelayFee,
    // which has units satoshis-per-kilobyte.
    // If you'd pay more in fees than the value of the output
    // to spend something, then we consider it dust.
    // A typical spendable non-segwit txout is 34 bytes big, and will
    // need a CTxIn of at least 148 bytes to spend:
    // so dust is a spendable txout less than
    // 182*dustRelayFee/1000 (in satoshis).
    // 546 satoshis at the default rate of 3000 sat/kvB.
    // A typical spendable segwit P2WPKH txout is 31 bytes big, and will
    // need a CTxIn of at least 67 bytes to spend:
    // so dust is a spendable txout less than
    // 98*dustRelayFee/1000 (in satoshis).
    // 294 satoshis at the default rate of 3000 sat/kvB.
    if (txout.scriptPubKey.IsUnspendable())
        return 0;

    size_t nSize = GetSerializeSize(txout);
    int witnessversion = 0;
    std::vector<unsigned char> witnessprogram;

    // Note this computation is for spending a Segwit v0 P2WPKH output (a 33 bytes
    // public key + an ECDSA signature). For Segwit v1 Taproot outputs the minimum
    // satisfaction is lower (a single BIP340 signature) but this computation was
    // kept to not further reduce the dust level.
    // See discussion in https://github.com/bitcoin/bitcoin/pull/22779 for details.
    if (txout.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        // sum the sizes of the parts of a transaction input
        // with 75% segwit discount applied to the script size.
        nSize += (32 + 4 + 1 + (107 / WITNESS_SCALE_FACTOR) + 4);
    } else {
        nSize += (32 + 4 + 1 + 107 + 4); // the 148 mentioned above
    }

    return dustRelayFeeIn.GetFee(nSize);
}

bool IsDust(const CTxOut& txout, const CFeeRate& dustRelayFeeIn)
{
    return (txout.nValue < GetDustThreshold(txout, dustRelayFeeIn));
}

bool IsStandard(const CScript& scriptPubKey, const std::optional<unsigned>& max_datacarrier_bytes, TxoutType& whichType)
{
    std::vector<std::vector<unsigned char> > vSolutions;
    whichType = Solver(scriptPubKey, vSolutions);

    if (whichType == TxoutType::NONSTANDARD) {
        return false;
    } else if (whichType == TxoutType::MULTISIG) {
        unsigned char m = vSolutions.front()[0];
        unsigned char n = vSolutions.back()[0];
        // Support up to x-of-3 multisig txns as standard
        if (n < 1 || n > 3)
            return false;
        if (m < 1 || m > n)
            return false;
    } else if (whichType == TxoutType::NULL_DATA) {
        if (!max_datacarrier_bytes || scriptPubKey.size() > *max_datacarrier_bytes) {
            return false;
        }
    }

    return true;
}

bool IsStandardTx(const CTransaction& tx, const std::optional<unsigned>& max_datacarrier_bytes, bool permit_bare_multisig, const CFeeRate& dust_relay_fee, std::string& reason)
{
    if (tx.version > TX_MAX_STANDARD_VERSION || tx.version < 1) {
        reason = "version";
        return false;
    }

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_WEIGHT mitigates CPU exhaustion attacks.
    unsigned int sz = GetTransactionWeight(tx);
    if (sz > MAX_STANDARD_TX_WEIGHT) {
        reason = "tx-size";
        return false;
    }

    for (const CTxIn& txin : tx.vin)
    {
        // Biggest 'standard' txin involving only keys is a 15-of-15 P2SH
        // multisig with compressed keys (remember the MAX_SCRIPT_ELEMENT_SIZE byte limit on
        // redeemScript size). That works out to a (15*(33+1))+3=513 byte
        // redeemScript, 513+1+15*(73+1)+3=1627 bytes of scriptSig, which
        // we round off to 1650(MAX_STANDARD_SCRIPTSIG_SIZE) bytes for
        // some minor future-proofing. That's also enough to spend a
        // 20-of-20 CHECKMULTISIG scriptPubKey, though such a scriptPubKey
        // is not considered standard.
        if (txin.scriptSig.size() > MAX_STANDARD_SCRIPTSIG_SIZE) {
            reason = "scriptsig-size";
            return false;
        }
        if (!txin.scriptSig.IsPushOnly()) {
            reason = "scriptsig-not-pushonly";
            return false;
        }
    }

    unsigned int nDataOut = 0;
    TxoutType whichType;
    for (const CTxOut& txout : tx.vout) {
        if (!::IsStandard(txout.scriptPubKey, max_datacarrier_bytes, whichType)) {
            reason = "scriptpubkey";
            return false;
        }

        if (whichType == TxoutType::NULL_DATA)
            nDataOut++;
        else if ((whichType == TxoutType::MULTISIG) && (!permit_bare_multisig)) {
            reason = "bare-multisig";
            return false;
        } else if (IsDust(txout, dust_relay_fee)) {
            reason = "dust";
            return false;
        }
    }

    // only one OP_RETURN txout is permitted
    if (nDataOut > 1) {
        reason = "multi-op-return";
        return false;
    }

    return true;
}

/**
 * Check transaction inputs to mitigate two
 * potential denial-of-service attacks:
 *
 * 1. scriptSigs with extra data stuffed into them,
 *    not consumed by scriptPubKey (or P2SH script)
 * 2. P2SH scripts with a crazy number of expensive
 *    CHECKSIG/CHECKMULTISIG operations
 *
 * Why bother? To avoid denial-of-service attacks; an attacker
 * can submit a standard HASH... OP_EQUAL transaction,
 * which will get accepted into blocks. The redemption
 * script can be anything; an attacker could use a very
 * expensive-to-check-upon-redemption script like:
 *   DUP CHECKSIG DROP ... repeated 100 times... OP_1
 *
 * Note that only the non-witness portion of the transaction is checked here.
 */
bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs)
{
    if (tx.IsCoinBase()) {
        return true; // Coinbases don't use vin normally
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxOut& prev = mapInputs.AccessCoin(tx.vin[i].prevout).out;

        std::vector<std::vector<unsigned char> > vSolutions;
        TxoutType whichType = Solver(prev.scriptPubKey, vSolutions);
        if (whichType == TxoutType::NONSTANDARD || whichType == TxoutType::WITNESS_UNKNOWN) {
            // WITNESS_UNKNOWN failures are typically also caught with a policy
            // flag in the script interpreter, but it can be helpful to catch
            // this type of NONSTANDARD transaction earlier in transaction
            // validation.
            return false;
        } else if (whichType == TxoutType::SCRIPTHASH) {
            std::vector<std::vector<unsigned char> > stack;
            // convert the scriptSig into a stack, so we can inspect the redeemScript
            if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SigVersion::BASE))
                return false;
            if (stack.empty())
                return false;
            CScript subscript(stack.back().begin(), stack.back().end());
            if (subscript.GetSigOpCount(true) > MAX_P2SH_SIGOPS) {
                return false;
            }
        }
    }

    return true;
}

bool IsWitnessStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs)
{
    if (tx.IsCoinBase())
        return true; // Coinbases are skipped

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        // We don't care if witness for this input is empty, since it must not be bloated.
        // If the script is invalid without witness, it would be caught sooner or later during validation.
        if (tx.vin[i].scriptWitness.IsNull())
            continue;

        const CTxOut &prev = mapInputs.AccessCoin(tx.vin[i].prevout).out;

        // get the scriptPubKey corresponding to this input:
        CScript prevScript = prev.scriptPubKey;

        // witness stuffing detected
        if (prevScript.IsPayToAnchor()) {
            return false;
        }

        bool p2sh = false;
        if (prevScript.IsPayToScriptHash()) {
            std::vector <std::vector<unsigned char> > stack;
            // If the scriptPubKey is P2SH, we try to extract the redeemScript casually by converting the scriptSig
            // into a stack. We do not check IsPushOnly nor compare the hash as these will be done later anyway.
            // If the check fails at this stage, we know that this txid must be a bad one.
            if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SigVersion::BASE))
                return false;
            if (stack.empty())
                return false;
            prevScript = CScript(stack.back().begin(), stack.back().end());
            p2sh = true;
        }

        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        // Non-witness program must not be associated with any witness
        if (!prevScript.IsWitnessProgram(witnessversion, witnessprogram))
            return false;

        // Check P2WSH standard limits
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            if (tx.vin[i].scriptWitness.stack.back().size() > MAX_STANDARD_P2WSH_SCRIPT_SIZE)
                return false;
            size_t sizeWitnessStack = tx.vin[i].scriptWitness.stack.size() - 1;
            if (sizeWitnessStack > MAX_STANDARD_P2WSH_STACK_ITEMS)
                return false;
            for (unsigned int j = 0; j < sizeWitnessStack; j++) {
                if (tx.vin[i].scriptWitness.stack[j].size() > MAX_STANDARD_P2WSH_STACK_ITEM_SIZE)
                    return false;
            }
        }

        // Check policy limits for Taproot spends:
        // - MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE limit for stack item size
        // - No annexes
        if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE && !p2sh) {
            // Taproot spend (non-P2SH-wrapped, version 1, witness program size 32; see BIP 341)
            Span stack{tx.vin[i].scriptWitness.stack};
            if (stack.size() >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
                // Annexes are nonstandard as long as no semantics are defined for them.
                return false;
            }
            if (stack.size() >= 2) {
                // Script path spend (2 or more stack elements after removing optional annex)
                const auto& control_block = SpanPopBack(stack);
                SpanPopBack(stack); // Ignore script
                if (control_block.empty()) return false; // Empty control block is invalid
                if ((control_block[0] & TAPROOT_LEAF_MASK) == TAPROOT_LEAF_TAPSCRIPT) {
                    // Leaf version 0xc0 (aka Tapscript, see BIP 342)
                    for (const auto& item : stack) {
                        if (item.size() > MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE) return false;
                    }
                }
            } else if (stack.size() == 1) {
                // Key path spend (1 stack element after removing optional annex)
                // (no policy rules apply)
            } else {
                // 0 stack elements; this is already invalid by consensus rules
                return false;
            }
        }
    }
    return true;
}

int64_t GetVirtualTransactionSize(int64_t nWeight, int64_t nSigOpCost, unsigned int bytes_per_sigop)
{
    return (std::max(nWeight, nSigOpCost * bytes_per_sigop) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR;
}

int64_t GetVirtualTransactionSize(const CTransaction& tx, int64_t nSigOpCost, unsigned int bytes_per_sigop)
{
    return GetVirtualTransactionSize(GetTransactionWeight(tx), nSigOpCost, bytes_per_sigop);
}

int64_t GetVirtualTransactionInputSize(const CTxIn& txin, int64_t nSigOpCost, unsigned int bytes_per_sigop)
{
    return GetVirtualTransactionSize(GetTransactionInputWeight(txin), nSigOpCost, bytes_per_sigop);
}
