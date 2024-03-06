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
#include <kernel/mempool_options.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/solver.h>
#include <serialize.h>
#include <span.h>

#include <algorithm>
#include <cstddef>
#include <utility>
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

std::vector<uint32_t> GetDust(const CTransaction& tx, CFeeRate dust_relay_rate)
{
    std::vector<uint32_t> dust_outputs;
    for (uint32_t i{0}; i < tx.vout.size(); ++i) {
        if (IsDust(tx.vout[i], dust_relay_rate)) dust_outputs.push_back(i);
    }
    return dust_outputs;
}

/**
 * Note this must assign whichType even if returning false, in case
 * IsStandardTx ignores the "scriptpubkey" rejection.
 */
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

static inline bool MaybeReject_(std::string& out_reason, const std::string& reason, const std::string& reason_prefix, const ignore_rejects_type& ignore_rejects) {
    if (ignore_rejects.count(reason_prefix + reason)) {
        return false;
    }

    out_reason = reason_prefix + reason;
    return true;
}

#define MaybeReject(reason)  do {  \
    if (MaybeReject_(out_reason, reason, reason_prefix, ignore_rejects)) {  \
        return false;  \
    }  \
} while(0)

bool IsStandardTx(const CTransaction& tx, const kernel::MemPoolOptions& opts, std::string& out_reason, const ignore_rejects_type& ignore_rejects)
{
    const std::string reason_prefix;

    if (tx.version > TX_MAX_STANDARD_VERSION || tx.version < 1) {
        MaybeReject("version");
    }

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_WEIGHT mitigates CPU exhaustion attacks.
    unsigned int sz = GetTransactionWeight(tx);
    if (sz > MAX_STANDARD_TX_WEIGHT) {
        MaybeReject("tx-size");
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
            MaybeReject("scriptsig-size");
        }
        if (!txin.scriptSig.IsPushOnly()) {
            MaybeReject("scriptsig-not-pushonly");
        }
    }

    unsigned int nDataOut = 0;
    TxoutType whichType;
    for (const CTxOut& txout : tx.vout) {
        if (!::IsStandard(txout.scriptPubKey, opts.max_datacarrier_bytes, whichType)) {
            MaybeReject("scriptpubkey");
        }

        if (whichType == TxoutType::WITNESS_UNKNOWN && !opts.acceptunknownwitness) {
            MaybeReject("scriptpubkey-unknown-witnessversion");
        }

        if (whichType == TxoutType::NULL_DATA) {
            nDataOut++;
            continue;
        }
        else if ((whichType == TxoutType::PUBKEY) && (!opts.permit_bare_pubkey)) {
            MaybeReject("bare-pubkey");
        }
        else if ((whichType == TxoutType::MULTISIG) && (!opts.permit_bare_multisig)) {
            MaybeReject("bare-multisig");
        }
    }

    // Only MAX_DUST_OUTPUTS_PER_TX dust is permitted(on otherwise valid ephemeral dust)
    if (GetDust(tx, opts.dust_relay_feerate).size() > MAX_DUST_OUTPUTS_PER_TX) {
        MaybeReject("dust");
    }

    // only one OP_RETURN txout is permitted
    if (nDataOut > 1) {
        MaybeReject("multi-op-return");
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
bool AreInputsStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs, const std::string& reason_prefix, std::string& out_reason, const ignore_rejects_type& ignore_rejects)
{
    if (tx.IsCoinBase()) {
        return true; // Coinbases don't use vin normally
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxOut& prev = mapInputs.AccessCoin(tx.vin[i].prevout).out;

        std::vector<std::vector<unsigned char> > vSolutions;
        TxoutType whichType = Solver(prev.scriptPubKey, vSolutions);
        if (whichType == TxoutType::NONSTANDARD) {
            MaybeReject("script-unknown");
        } else if (whichType == TxoutType::WITNESS_UNKNOWN) {
            // WITNESS_UNKNOWN failures are typically also caught with a policy
            // flag in the script interpreter, but it can be helpful to catch
            // this type of NONSTANDARD transaction earlier in transaction
            // validation.
            MaybeReject("witness-unknown");
        } else if (whichType == TxoutType::SCRIPTHASH) {
            if (!tx.vin[i].scriptSig.IsPushOnly()) {
                // The only way we got this far, is if the user ignored scriptsig-not-pushonly.
                // However, this case is invalid, and will be caught later on.
                // But for now, we don't want to run the [possibly expensive] script here.
                continue;
            }
            std::vector<std::vector<unsigned char> > stack;
            // convert the scriptSig into a stack, so we can inspect the redeemScript
            if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SigVersion::BASE))
            {
                // This case is also invalid or a bug
                out_reason = reason_prefix + "scriptsig-failure";
                return false;
            }
            if (stack.empty())
            {
                // Also invalid
                out_reason = reason_prefix + "scriptcheck-missing";
                return false;
            }
            CScript subscript(stack.back().begin(), stack.back().end());
            if (subscript.GetSigOpCount(true) > MAX_P2SH_SIGOPS) {
                MaybeReject("scriptcheck-sigops");
            }
        }
    }

    return true;
}

bool IsWitnessStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs, const std::string& reason_prefix, std::string& out_reason, const ignore_rejects_type& ignore_rejects)
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
            MaybeReject("anchor-not-empty");
        }

        bool p2sh = false;
        if (prevScript.IsPayToScriptHash()) {
            std::vector <std::vector<unsigned char> > stack;
            // If the scriptPubKey is P2SH, we try to extract the redeemScript casually by converting the scriptSig
            // into a stack. We do not check IsPushOnly nor compare the hash as these will be done later anyway.
            // If the check fails at this stage, we know that this txid must be a bad one.
            if (!EvalScript(stack, tx.vin[i].scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SigVersion::BASE))
            {
                out_reason = reason_prefix + "scriptsig-failure";
                return false;
            }
            if (stack.empty())
            {
                out_reason = reason_prefix + "scriptcheck-missing";
                return false;
            }
            prevScript = CScript(stack.back().begin(), stack.back().end());
            p2sh = true;
        }

        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;

        // Non-witness program must not be associated with any witness
        if (!prevScript.IsWitnessProgram(witnessversion, witnessprogram))
        {
            out_reason = reason_prefix + "nonwitness-input";
            return false;
        }

        // Check P2WSH standard limits
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            if (tx.vin[i].scriptWitness.stack.back().size() > MAX_STANDARD_P2WSH_SCRIPT_SIZE)
                MaybeReject("script-size");
            size_t sizeWitnessStack = tx.vin[i].scriptWitness.stack.size() - 1;
            if (sizeWitnessStack > MAX_STANDARD_P2WSH_STACK_ITEMS)
                MaybeReject("stackitem-count");
            for (unsigned int j = 0; j < sizeWitnessStack; j++) {
                if (tx.vin[i].scriptWitness.stack[j].size() > MAX_STANDARD_P2WSH_STACK_ITEM_SIZE)
                    MaybeReject("stackitem-size");
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
                MaybeReject("taproot-annex");
                // If reject reason is ignored, continue as if the annex wasn't there.
                SpanPopBack(stack);
            }
            if (stack.size() >= 2) {
                // Script path spend (2 or more stack elements after removing optional annex)
                const auto& control_block = SpanPopBack(stack);
                SpanPopBack(stack); // Ignore script
                if (control_block.empty()) {
                    // Empty control block is invalid
                    out_reason = reason_prefix + "taproot-control-missing";
                    return false;
                }
                if ((control_block[0] & TAPROOT_LEAF_MASK) == TAPROOT_LEAF_TAPSCRIPT) {
                    // Leaf version 0xc0 (aka Tapscript, see BIP 342)
                    if (!ignore_rejects.count(reason_prefix + "taproot-stackitem-size")) {
                    for (const auto& item : stack) {
                            if (item.size() > MAX_STANDARD_TAPSCRIPT_STACK_ITEM_SIZE) {
                                out_reason = reason_prefix + "taproot-stackitem-size";
                                return false;
                            }
                        }
                    }
                }
            } else if (stack.size() == 1) {
                // Key path spend (1 stack element after removing optional annex)
                // (no policy rules apply)
            } else {
                // 0 stack elements; this is already invalid by consensus rules
                out_reason = reason_prefix + "taproot-witness-missing";
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

std::pair<CScript, unsigned int> GetScriptForTransactionInput(CScript prevScript, const CTxIn& txin)
{
    bool p2sh = false;
    if (prevScript.IsPayToScriptHash()) {
        std::vector <std::vector<unsigned char> > stack;
        if (!EvalScript(stack, txin.scriptSig, SCRIPT_VERIFY_NONE, BaseSignatureChecker(), SigVersion::BASE)) {
            return std::make_pair(CScript(), 0);
        }
        if (stack.empty()) {
            return std::make_pair(CScript(), 0);
        }
        prevScript = CScript(stack.back().begin(), stack.back().end());
        p2sh = true;
    }

    int witnessversion = 0;
    std::vector<unsigned char> witnessprogram;

    if (!prevScript.IsWitnessProgram(witnessversion, witnessprogram)) {
        // For P2SH, scriptSig is always push-only, so the actual script is only the last stack item
        // For non-P2SH, prevScript is likely the real script, but not part of this transaction, and scriptSig could very well be executable, so return the latter instead
        return std::make_pair(p2sh ? prevScript : txin.scriptSig, WITNESS_SCALE_FACTOR);
    }

    Span stack{txin.scriptWitness.stack};

    if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
        if (stack.empty()) return std::make_pair(CScript(), 0);  // invalid
        auto& script_data = stack.back();
        prevScript = CScript(script_data.begin(), script_data.end());
        return std::make_pair(prevScript, 1);
    }

    if (witnessversion == 1 && witnessprogram.size() == WITNESS_V1_TAPROOT_SIZE && !p2sh) {
        if (stack.size() >= 2 && !stack.back().empty() && stack.back()[0] == ANNEX_TAG) {
            SpanPopBack(stack);
        }
        if (stack.size() >= 2) {
            SpanPopBack(stack);  // Ignore control block
            prevScript = CScript(stack.back().begin(), stack.back().end());
            return std::make_pair(prevScript, 1);
        }
    }

    return std::make_pair(CScript(), 0);
}

std::pair<size_t, size_t> DatacarrierBytes(const CTransaction& tx, const CCoinsViewCache& view)
{
    std::pair<size_t, size_t> ret{0, 0};

    for (const CTxIn& txin : tx.vin) {
        const CTxOut &utxo = view.AccessCoin(txin.prevout).out;
        auto[script, consensus_weight_per_byte] = GetScriptForTransactionInput(utxo.scriptPubKey, txin);
        const auto dcb = script.DatacarrierBytes();
        ret.first += dcb.first;
        ret.second += dcb.second;
    }
    for (const CTxOut& txout : tx.vout) {
        const auto dcb = txout.scriptPubKey.DatacarrierBytes();
        ret.first += dcb.first;
        ret.second += dcb.second;
    }

    return ret;
}

int32_t CalculateExtraTxWeight(const CTransaction& tx, const CCoinsViewCache& view, const unsigned int weight_per_data_byte)
{
    int32_t mod_weight{0};

    // Add in any extra weight for data bytes
    if (weight_per_data_byte > 1) {
        for (const CTxIn& txin : tx.vin) {
            const CTxOut &utxo = view.AccessCoin(txin.prevout).out;
            auto[script, consensus_weight_per_byte] = GetScriptForTransactionInput(utxo.scriptPubKey, txin);
            if (weight_per_data_byte > consensus_weight_per_byte) {
                const auto dcb = script.DatacarrierBytes();
                mod_weight += (dcb.first + dcb.second) * (weight_per_data_byte - consensus_weight_per_byte);
            }
        }
        if (weight_per_data_byte > WITNESS_SCALE_FACTOR) {
            for (const CTxOut& txout : tx.vout) {
                const auto dcb = txout.scriptPubKey.DatacarrierBytes();
                mod_weight += (dcb.first + dcb.second) * (weight_per_data_byte - WITNESS_SCALE_FACTOR);
            }
        }
    }

    return mod_weight;
}
