// Copyright (c) 2019-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <signet.h>

#include <array>
#include <cstdint>
#include <vector>

#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <hash.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <span.h>
#include <script/interpreter.h>
#include <script/standard.h>
#include <streams.h>
#include <txmempool.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <uint256.h>
#include <validation.h>

static constexpr uint8_t SIGNET_HEADER[4] = {0xec, 0xc7, 0xda, 0xa2};

const CHashWriter HASHER_SIGNMESSAGE = TaggedHash("BIP0322-signed-message");

static constexpr unsigned int BLOCK_SCRIPT_VERIFY_FLAGS = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_DERSIG | SCRIPT_VERIFY_NULLDUMMY;

static bool FetchAndClearCommitmentSection(const Span<const uint8_t> header, CScript& witness_commitment, std::vector<uint8_t>& result)
{
    CScript replacement;
    bool found_header = false;
    result.clear();

    opcodetype opcode;
    CScript::const_iterator pc = witness_commitment.begin();
    std::vector<uint8_t> pushdata;
    while (witness_commitment.GetOp(pc, opcode, pushdata)) {
        if (pushdata.size() > 0) {
            if (!found_header && pushdata.size() > (size_t) header.size() && Span<const uint8_t>(pushdata.data(), header.size()) == header) {
                // pushdata only counts if it has the header _and_ some data
                result.insert(result.end(), pushdata.begin() + header.size(), pushdata.end());
                pushdata.erase(pushdata.begin() + header.size(), pushdata.end());
                found_header = true;
            }
            replacement << pushdata;
        } else {
            replacement << opcode;
        }
    }

    if (found_header) witness_commitment = replacement;
    return found_header;
}

static uint256 ComputeModifiedMerkleRoot(const CMutableTransaction& cb, const CBlock& block)
{
    std::vector<uint256> leaves;
    leaves.resize(block.vtx.size());
    leaves[0] = cb.GetHash();
    for (size_t s = 1; s < block.vtx.size(); ++s) {
        leaves[s] = block.vtx[s]->GetHash();
    }
    return ComputeMerkleRoot(std::move(leaves));
}

Optional<SignetTxs> SignetTxs::Create(const CScript& signature, const std::vector<std::vector<uint8_t>>& witnessStack, const std::vector<uint8_t>& commitment, const CScript& challenge)
{
    CMutableTransaction tx_to_spend;
    tx_to_spend.nVersion = 0;
    tx_to_spend.nLockTime = 0;
    tx_to_spend.vin.emplace_back(COutPoint(), CScript(OP_0), 0);
    tx_to_spend.vin[0].scriptSig << commitment;
    tx_to_spend.vout.emplace_back(0, challenge);

    CMutableTransaction tx_spending;
    tx_spending.nVersion = 0;
    tx_spending.nLockTime = 0;
    tx_spending.vin.emplace_back(COutPoint(), CScript(), 0);
    tx_spending.vin[0].scriptSig = signature;
    tx_spending.vin[0].scriptWitness.stack = witnessStack;
    tx_spending.vout.emplace_back(0, CScript(OP_RETURN));

    tx_spending.vin[0].prevout = COutPoint(tx_to_spend.GetHash(), 0);

    return SignetTxs{tx_to_spend, tx_spending};
}

Optional<SignetTxs> SignetTxs::Create(const CBlock& block, const CScript& challenge)
{
    // find and delete signet signature
    if (block.vtx.empty()) return nullopt; // no coinbase tx in block; invalid
    CMutableTransaction modified_cb(*block.vtx.at(0));

    const int cidx = GetWitnessCommitmentIndex(block);
    if (cidx == NO_WITNESS_COMMITMENT) {
        return nullopt; // require a witness commitment
    }

    CScript& witness_commitment = modified_cb.vout.at(cidx).scriptPubKey;

    CScript scriptSig;
    std::vector<std::vector<uint8_t>> witnessStack;
    std::vector<uint8_t> signet_solution;
    if (!FetchAndClearCommitmentSection(SIGNET_HEADER, witness_commitment, signet_solution)) {
        // no signet solution -- allow this to support OP_TRUE as trivial block challenge
    } else {
        try {
            VectorReader v(SER_NETWORK, INIT_PROTO_VERSION, signet_solution, 0);
            v >> scriptSig;
            v >> witnessStack;
            if (!v.empty()) return nullopt; // extraneous data encountered
        } catch (const std::exception&) {
            return nullopt; // parsing error
        }
    }
    uint256 signet_merkle = ComputeModifiedMerkleRoot(modified_cb, block);

    std::vector<uint8_t> block_data;
    CVectorWriter writer(SER_NETWORK, INIT_PROTO_VERSION, block_data, 0);
    writer << block.nVersion;
    writer << block.hashPrevBlock;
    writer << signet_merkle;
    writer << block.nTime;

    return Create(scriptSig, witnessStack, block_data, challenge);
}

// Signet block solution checker
bool CheckSignetBlockSolution(const CBlock& block, const Consensus::Params& consensusParams)
{
    if (block.GetHash() == consensusParams.hashGenesisBlock) {
        // genesis block solution is always valid
        return true;
    }

    const CScript challenge(consensusParams.signet_challenge.begin(), consensusParams.signet_challenge.end());
    const Optional<SignetTxs> signet_txs = SignetTxs::Create(block, challenge);

    if (!signet_txs) {
        LogPrint(BCLog::VALIDATION, "CheckSignetBlockSolution: Errors in block (block solution parse failure)\n");
        return false;
    }

    const CScript& scriptSig = signet_txs->m_to_sign.vin[0].scriptSig;
    const CScriptWitness& witness = signet_txs->m_to_sign.vin[0].scriptWitness;

    TransactionSignatureChecker sigcheck(&signet_txs->m_to_sign, /*nIn=*/ 0, /*amount=*/ signet_txs->m_to_spend.vout[0].nValue);

    if (!VerifyScript(scriptSig, signet_txs->m_to_spend.vout[0].scriptPubKey, &witness, BLOCK_SCRIPT_VERIFY_FLAGS, sigcheck)) {
        LogPrint(BCLog::VALIDATION, "CheckSignetBlockSolution: Errors in block (block solution invalid)\n");
        return false;
    }
    return true;
}

bool UpdateTransactionProof(const CTransaction& to_sign, size_t input_index, const std::map<COutPoint, Coin>& coins, TransactionProofResult& res)
{
    const CTxOut& out = coins.at(to_sign.vin[input_index].prevout).out;
    TransactionSignatureChecker sigcheck(&to_sign, /*nIn=*/ input_index, /*amount=*/ out.nValue);

    if (!VerifyScript(to_sign.vin[input_index].scriptSig, out.scriptPubKey, &to_sign.vin[input_index].scriptWitness, BLOCK_SCRIPT_VERIFY_FLAGS, sigcheck)) {
        res = TransactionProofInvalid;
        return false;
    }

    if (!(res & TransactionProofInconclusive) && !VerifyScript(to_sign.vin[input_index].scriptSig, out.scriptPubKey, &to_sign.vin[input_index].scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, sigcheck)) {
        res = TransactionProofInconclusive | (res & TransactionProofInFutureFlag);
    }

    return true;
}

TransactionProofResult CheckTransactionProof(const std::string& message, const CScript& challenge, const CTransaction& to_spend, const CTransaction& to_sign)
{
    auto message_hash = GetMessageCommitment(message);

    // Construct our version of the spend/sign transactions and verify that they match
    // The signature and witness are in the to_sign transaction's first input.
    if (to_sign.vin.size() < 1) {
        return TransactionProofInvalid;
    }
    const auto signet_txs = SignetTxs::Create(to_sign.vin[0].scriptSig, to_sign.vin[0].scriptWitness.stack, message_hash, challenge);
    if (!signet_txs) return TransactionProofInvalid;
    // the to_spend transaction should be identical
    if (to_spend != signet_txs->m_to_spend) return TransactionProofInvalid;
    // the to_sign transaction should have exactly 1 output
    if (to_sign.vout.size() != 1) return TransactionProofInvalid;
    // the to_sign transaction may have extra inputs, but the first one should be identical
    if (to_sign.vin[0] != signet_txs->m_to_sign.vin[0]) return TransactionProofInvalid;

    // Comparison check OK

    // Now we do the signature check for the transaction
    TransactionProofResult res = TransactionProofValid;
    std::map<COutPoint, Coin> coins;

    // The first input is virtual, so we don't need to fetch anything
    coins[COutPoint(to_spend.GetHash(), 0)] = Coin(to_spend.vout[0], 0, true);

    // The rest need to be fetched from the UTXO set
    if (to_sign.vin.size() > 1) {
        // Fetch a list of inputs
        {
            LOCK(cs_main);
            CCoinsViewCache& view = ::ChainstateActive().CoinsTip();
            for (size_t i = 1; i < to_sign.vin.size(); ++i) {
                if (!view.GetCoin(to_sign.vin[i].prevout, coins[to_sign.vin[i].prevout])) {
                    // TODO: deal with spent transactions somehow; a proof for a spend output is also useful (perhaps let the verifier provide the transactions)
                    return TransactionProofInvalid;
                }
            }
        }
    }

    // Validate spends
    for (size_t i = 0; i < to_sign.vin.size(); ++i) {
        if (!UpdateTransactionProof(to_sign, i, coins, res)) return res;
    }

    return res;
}

std::vector<uint8_t> GetMessageCommitment(const std::string& message)
{
    CHashWriter hasher = HASHER_SIGNMESSAGE;
    hasher << message;
    uint256 hash = hasher.GetHash();
    std::vector<uint8_t> commitment;
    commitment.resize(32);
    memcpy(commitment.data(), hash.begin(), 32);
    return commitment;
}
