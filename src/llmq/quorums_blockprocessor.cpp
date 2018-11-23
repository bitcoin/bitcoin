// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "quorums_blockprocessor.h"
#include "quorums_commitment.h"
#include "quorums_utils.h"

#include "evo/specialtx.h"

#include "chain.h"
#include "chainparams.h"
#include "consensus/validation.h"
#include "net.h"
#include "net_processing.h"
#include "primitives/block.h"
#include "validation.h"

namespace llmq
{

CQuorumBlockProcessor* quorumBlockProcessor;

static const std::string DB_MINED_COMMITMENT = "q_mc";

void CQuorumBlockProcessor::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman& connman)
{
    if (strCommand == NetMsgType::QFCOMMITMENT) {
        CFinalCommitment qc;
        vRecv >> qc;

        if (qc.IsNull()) {
            LOCK(cs_main);
            LogPrintf("CQuorumBlockProcessor::%s -- null commitment from peer=%d\n", __func__, pfrom->id);
            Misbehaving(pfrom->id, 100);
            return;
        }

        if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)qc.llmqType)) {
            LOCK(cs_main);
            LogPrintf("CQuorumBlockProcessor::%s -- invalid commitment type %d from peer=%d\n", __func__,
                    qc.llmqType, pfrom->id);
            Misbehaving(pfrom->id, 100);
            return;
        }
        auto type = (Consensus::LLMQType)qc.llmqType;
        const auto& params = Params().GetConsensus().llmqs.at(type);

        // Verify that quorumHash is part of the active chain and that it's the first block in the DKG interval
        {
            LOCK(cs_main);
            if (!mapBlockIndex.count(qc.quorumHash)) {
                LogPrintf("CQuorumBlockProcessor::%s -- unknown block %s in commitment, peer=%d\n", __func__,
                        qc.quorumHash.ToString(), pfrom->id);
                // can't really punish the node here, as we might simply be the one that is on the wrong chain or not
                // fully synced
                return;
            }
            auto pquorumIndex = mapBlockIndex[qc.quorumHash];
            if (chainActive.Tip()->GetAncestor(pquorumIndex->nHeight) != pquorumIndex) {
                LogPrintf("CQuorumBlockProcessor::%s -- block %s not in active chain, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->id);
                // same, can't punish
                return;
            }
            int quorumHeight = pquorumIndex->nHeight - (pquorumIndex->nHeight % params.dkgInterval);
            if (quorumHeight != pquorumIndex->nHeight) {
                LogPrintf("CQuorumBlockProcessor::%s -- block %s is not the first block in the DKG interval, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->id);
                Misbehaving(pfrom->id, 100);
                return;
            }
        }

        {
            // Check if we already got a better one locally
            // We do this before verifying the commitment to avoid DoS
            LOCK(minableCommitmentsCs);
            auto k = std::make_pair(type, qc.quorumHash);
            auto it = minableCommitmentsByQuorum.find(k);
            if (it != minableCommitmentsByQuorum.end()) {
                auto jt = minableCommitments.find(it->second);
                if (jt != minableCommitments.end()) {
                    if (jt->second.CountSigners() <= qc.CountSigners()) {
                        return;
                    }
                }
            }
        }

        auto members = CLLMQUtils::GetAllQuorumMembers(type, qc.quorumHash);

        if (!qc.Verify(members)) {
            LOCK(cs_main);
            LogPrintf("CQuorumBlockProcessor::%s -- commitment for quorum %s:%d is not valid, peer=%d\n", __func__,
                      qc.quorumHash.ToString(), qc.llmqType, pfrom->id);
            Misbehaving(pfrom->id, 100);
            return;
        }

        LogPrintf("CQuorumBlockProcessor::%s -- received commitment for quorum %s:%d, validMembers=%d, signers=%d, peer=%d\n", __func__,
                  qc.quorumHash.ToString(), qc.llmqType, qc.CountValidMembers(), qc.CountSigners(), pfrom->id);

        AddMinableCommitment(qc);
    }
}

bool CQuorumBlockProcessor::ProcessBlock(const CBlock& block, const CBlockIndex* pindexPrev, CValidationState& state)
{
    AssertLockHeld(cs_main);

    bool fDIP0003Active = VersionBitsState(pindexPrev, Params().GetConsensus(), Consensus::DEPLOYMENT_DIP0003, versionbitscache) == THRESHOLD_ACTIVE;
    if (!fDIP0003Active) {
        return true;
    }

    int nHeight = pindexPrev->nHeight + 1;

    std::map<Consensus::LLMQType, CFinalCommitment> qcs;
    if (!GetCommitmentsFromBlock(block, pindexPrev, qcs, state)) {
        return false;
    }

    // The following checks make sure that there is always a (possibly null) commitment while in the mining phase
    // until the first non-null commitment has been mined. After the non-null commitment, no other commitments are
    // allowed, including null commitments.
    for (const auto& p : Params().GetConsensus().llmqs) {
        auto type = p.first;

        uint256 quorumHash = GetQuorumBlockHash(type, pindexPrev);

        // does the currently processed block contain a (possibly null) commitment for the current session?
        bool hasCommitmentInNewBlock = qcs.count(type) != 0;
        bool isCommitmentRequired = IsCommitmentRequired(type, pindexPrev);

        if (hasCommitmentInNewBlock && !isCommitmentRequired) {
            // If we're either not in the mining phase or a non-null commitment was mined already, reject the block
            return state.DoS(100, false, REJECT_INVALID, "bad-qc-not-allowed");
        }

        if (!hasCommitmentInNewBlock && isCommitmentRequired) {
            // If no non-null commitment was mined for the mining phase yet and the new block does not include
            // a (possibly null) commitment, the block should be rejected.
            return state.DoS(100, false, REJECT_INVALID, "bad-qc-missing");
        }
    }

    for (auto& p : qcs) {
        auto& qc = p.second;
        if (!ProcessCommitment(pindexPrev, qc, state)) {
            return false;
        }
    }
    return true;
}

bool CQuorumBlockProcessor::ProcessCommitment(const CBlockIndex* pindexPrev, const CFinalCommitment& qc, CValidationState& state)
{
    auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)qc.llmqType);

    uint256 quorumHash = GetQuorumBlockHash((Consensus::LLMQType)qc.llmqType, pindexPrev);
    if (quorumHash.IsNull()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-block");
    }
    if (quorumHash != qc.quorumHash) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-block");
    }

    if (qc.IsNull()) {
        if (!qc.VerifyNull(pindexPrev->nHeight + 1)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-qc-invalid-null");
        }
        return true;
    }

    if (HasMinedCommitment(params.type, quorumHash)) {
        // should not happen as it's already handled in ProcessBlock
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-dup");
    }

    if (!IsMiningPhase(params.type, pindexPrev->nHeight + 1)) {
        // should not happen as it's already handled in ProcessBlock
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-height");
    }

    auto members = CLLMQUtils::GetAllQuorumMembers(params.type, quorumHash);

    if (!qc.Verify(members)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-invalid");
    }

    // Store commitment in DB
    evoDb.Write(std::make_pair(DB_MINED_COMMITMENT, std::make_pair((uint8_t)params.type, quorumHash)), qc);

    LogPrintf("CQuorumBlockProcessor::%s -- processed commitment from block. type=%d, quorumHash=%s, signers=%s, validMembers=%d, quorumPublicKey=%s\n", __func__,
              qc.llmqType, quorumHash.ToString(), qc.CountSigners(), qc.CountValidMembers(), qc.quorumPublicKey.ToString());

    return true;
}

bool CQuorumBlockProcessor::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);

    std::map<Consensus::LLMQType, CFinalCommitment> qcs;
    CValidationState dummy;
    if (!GetCommitmentsFromBlock(block, pindex->pprev, qcs, dummy)) {
        return false;
    }

    for (auto& p : qcs) {
        auto& qc = p.second;
        if (qc.IsNull()) {
            continue;
        }

        evoDb.Erase(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(qc.llmqType, qc.quorumHash)));

        if (!qc.IsNull()) {
            // if a reorg happened, we should allow to mine this commitment later
            AddMinableCommitment(qc);
        }
    }

    return true;
}

bool CQuorumBlockProcessor::GetCommitmentsFromBlock(const CBlock& block, const CBlockIndex* pindexPrev, std::map<Consensus::LLMQType, CFinalCommitment>& ret, CValidationState& state)
{
    AssertLockHeld(cs_main);

    auto& consensus = Params().GetConsensus();
    bool fDIP0003Active = VersionBitsState(pindexPrev, consensus, Consensus::DEPLOYMENT_DIP0003, versionbitscache) == THRESHOLD_ACTIVE;

    ret.clear();

    for (const auto& tx : block.vtx) {
        if (tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            CFinalCommitment qc;
            if (!GetTxPayload(*tx, qc)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");
            }

            if (!consensus.llmqs.count((Consensus::LLMQType)qc.llmqType)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-qc-type");
            }

            // only allow one commitment per type and per block
            if (ret.count((Consensus::LLMQType)qc.llmqType)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-qc-dup");
            }

            ret.emplace((Consensus::LLMQType)qc.llmqType, std::move(qc));
        }
    }

    if (!fDIP0003Active && !ret.empty()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-premature");
    }

    return true;
}

bool CQuorumBlockProcessor::IsMiningPhase(Consensus::LLMQType llmqType, int nHeight)
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);
    int phaseIndex = nHeight % params.dkgInterval;
    if (phaseIndex >= params.dkgMiningWindowStart && phaseIndex <= params.dkgMiningWindowEnd) {
        return true;
    }
    return false;
}

bool CQuorumBlockProcessor::IsCommitmentRequired(Consensus::LLMQType llmqType, const CBlockIndex* pindexPrev)
{
    // BEGIN TEMPORARY CODE
    bool allowMissingQc = false;
    {
        // TODO We added the commitments code while DIP3 was already activated on testnet and we want
        // to avoid reverting the chain again, as people already had many MNs registered at that time.
        // So, we do a simple hardfork here at a fixed height, but only while we're on the original
        // DIP3 chain.
        // As we need to fork/revert the chain later to re-test all deployment stages of DIP3, we can
        // remove all this temporary code later.
        LOCK(cs_main);
        const auto& consensus = Params().GetConsensus();
        if (consensus.nTemporaryTestnetForkDIP3Height != 0 &&
            pindexPrev->nHeight + 1 >= consensus.nTemporaryTestnetForkDIP3Height &&
            pindexPrev->nHeight + 1 < consensus.nTemporaryTestnetForkHeight &&
            chainActive[consensus.nTemporaryTestnetForkDIP3Height]->GetBlockHash() == consensus.nTemporaryTestnetForkDIP3BlockHash) {
            allowMissingQc = true;
        }
    }
    // END TEMPORARY CODE

    uint256 quorumHash = GetQuorumBlockHash(llmqType, pindexPrev);

    // perform extra check for quorumHash.IsNull as the quorum hash is unknown for the first block of a session
    // this is because the currently processed block's hash will be the quorumHash of this session
    bool isMiningPhase = !quorumHash.IsNull() && IsMiningPhase(llmqType, pindexPrev->nHeight + 1);

    // did we already mine a non-null commitment for this session?
    bool hasMinedCommitment = !quorumHash.IsNull() && HasMinedCommitment(llmqType, quorumHash);

    return isMiningPhase && !hasMinedCommitment && !allowMissingQc;
}

// WARNING: This method returns uint256() on the first block of the DKG interval (because the block hash is not known yet)
uint256 CQuorumBlockProcessor::GetQuorumBlockHash(Consensus::LLMQType llmqType, const CBlockIndex* pindexPrev)
{
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    int nHeight = pindexPrev->nHeight + 1;
    int quorumStartHeight = nHeight - (nHeight % params.dkgInterval);
    if (quorumStartHeight >= pindexPrev->nHeight) {
        return uint256();
    }
    auto quorumIndex = pindexPrev->GetAncestor(quorumStartHeight);
    assert(quorumIndex);
    return quorumIndex->GetBlockHash();
}

bool CQuorumBlockProcessor::HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair((uint8_t)llmqType, quorumHash));
    return evoDb.Exists(key);
}

bool CQuorumBlockProcessor::GetMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash, CFinalCommitment& ret)
{
    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair((uint8_t)llmqType, quorumHash));
    return evoDb.Read(key, ret);
}

bool CQuorumBlockProcessor::HasMinableCommitment(const uint256& hash)
{
    LOCK(minableCommitmentsCs);
    return minableCommitments.count(hash) != 0;
}

void CQuorumBlockProcessor::AddMinableCommitment(const CFinalCommitment& fqc)
{
    bool relay = false;
    uint256 commitmentHash = ::SerializeHash(fqc);

    {
        LOCK(minableCommitmentsCs);

        auto k = std::make_pair((Consensus::LLMQType) fqc.llmqType, fqc.quorumHash);
        auto ins = minableCommitmentsByQuorum.emplace(k, commitmentHash);
        if (ins.second) {
            minableCommitments.emplace(commitmentHash, fqc);
            relay = true;
        } else {
            auto& oldFqc = minableCommitments.at(ins.first->second);
            if (fqc.CountSigners() > oldFqc.CountSigners()) {
                // new commitment has more signers, so override the known one
                ins.first->second = commitmentHash;
                minableCommitments.erase(ins.first->second);
                minableCommitments.emplace(commitmentHash, fqc);
                relay = true;
            }
        }
    }

    // We only relay the new commitment if it's new or better then the old one
    if (relay) {
        CInv inv(MSG_QUORUM_FINAL_COMMITMENT, commitmentHash);
        g_connman->RelayInv(inv, DMN_PROTO_VERSION);
    }
}

bool CQuorumBlockProcessor::GetMinableCommitmentByHash(const uint256& commitmentHash, llmq::CFinalCommitment& ret)
{
    LOCK(minableCommitmentsCs);
    auto it = minableCommitments.find(commitmentHash);
    if (it == minableCommitments.end()) {
        return false;
    }
    ret = it->second;
    return true;
}

// Will return false if no commitment should be mined
// Will return true and a null commitment if no minable commitment is known and none was mined yet
bool CQuorumBlockProcessor::GetMinableCommitment(Consensus::LLMQType llmqType, const CBlockIndex* pindexPrev, CFinalCommitment& ret)
{
    AssertLockHeld(cs_main);

    int nHeight = pindexPrev->nHeight + 1;

    if (!IsCommitmentRequired(llmqType, pindexPrev)) {
        // no commitment required
        return false;
    }

    uint256 quorumHash = GetQuorumBlockHash(llmqType, pindexPrev);
    if (quorumHash.IsNull()) {
        return false;
    }

    LOCK(minableCommitmentsCs);

    auto k = std::make_pair(llmqType, quorumHash);
    auto it = minableCommitmentsByQuorum.find(k);
    if (it == minableCommitmentsByQuorum.end()) {
        // null commitment required
        ret = CFinalCommitment(Params().GetConsensus().llmqs.at(llmqType), quorumHash);
        ret.quorumVvecHash = ::SerializeHash(std::make_pair(quorumHash, nHeight));
        return true;
    }

    ret = minableCommitments.at(it->second);

    return true;
}

bool CQuorumBlockProcessor::GetMinableCommitmentTx(Consensus::LLMQType llmqType, const CBlockIndex* pindexPrev, CTransactionRef& ret)
{
    AssertLockHeld(cs_main);

    CFinalCommitment qc;
    if (!GetMinableCommitment(llmqType, pindexPrev, qc)) {
        return false;
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_QUORUM_COMMITMENT;
    SetTxPayload(tx, qc);

    ret = MakeTransactionRef(tx);

    return true;
}

}
