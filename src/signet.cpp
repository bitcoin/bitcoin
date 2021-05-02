// Copyright (c) 2019-2020 The XBit Core developers
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
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <span.h>
#include <script/interpreter.h>
#include <script/standard.h>
#include <streams.h>
#include <util/strencodings.h>
#include <util/system.h>
#include <uint256.h>

static constexpr uint8_t SIGNET_HEADER[4] = {0xec, 0xc7, 0xda, 0xa2};

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

Optional<SignetTxs> SignetTxs::Create(const CBlock& block, const CScript& challenge)
{
    CMutableTransaction tx_to_spend;
    tx_to_spend.nVersion = 0;
    tx_to_spend.nLockTime = 0;
    tx_to_spend.vin.emplace_back(COutPoint(), CScript(OP_0), 0);
    tx_to_spend.vout.emplace_back(0, challenge);

    CMutableTransaction tx_spending;
    tx_spending.nVersion = 0;
    tx_spending.nLockTime = 0;
    tx_spending.vin.emplace_back(COutPoint(), CScript(), 0);
    tx_spending.vout.emplace_back(0, CScript(OP_RETURN));

    // can't fill any other fields before extracting signet
    // responses from block coinbase tx

    // find and delete signet signature
    if (block.vtx.empty()) return nullopt; // no coinbase tx in block; invalid
    CMutableTransaction modified_cb(*block.vtx.at(0));

    const int cidx = GetWitnessCommitmentIndex(block);
    if (cidx == NO_WITNESS_COMMITMENT) {
        return nullopt; // require a witness commitment
    }

    CScript& witness_commitment = modified_cb.vout.at(cidx).scriptPubKey;

    std::vector<uint8_t> signet_solution;
    if (!FetchAndClearCommitmentSection(SIGNET_HEADER, witness_commitment, signet_solution)) {
        // no signet solution -- allow this to support OP_TRUE as trivial block challenge
    } else {
        try {
            VectorReader v(SER_NETWORK, INIT_PROTO_VERSION, signet_solution, 0);
            v >> tx_spending.vin[0].scriptSig;
            v >> tx_spending.vin[0].scriptWitness.stack;
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
    tx_to_spend.vin[0].scriptSig << block_data;
    tx_spending.vin[0].prevout = COutPoint(tx_to_spend.GetHash(), 0);

    return SignetTxs{tx_to_spend, tx_spending};
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
