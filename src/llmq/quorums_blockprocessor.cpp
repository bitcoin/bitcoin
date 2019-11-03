// Copyright (c) 2018-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_utils.h>

#include <special/specialtx.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <net.h>
#include <net_processing.h>
#include <primitives/block.h>
#include <validation.h>

namespace llmq
{

CQuorumBlockProcessor* quorumBlockProcessor;

static const std::string DB_MINED_COMMITMENT = "q_mc";
static const std::string DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT = "q_mcih";

static const std::string DB_BEST_BLOCK_UPGRADE = "q_bbu2";

void CQuorumBlockProcessor::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    if (strCommand == NetMsgType::QFCOMMITMENT) {
        CFinalCommitment qc;
        vRecv >> qc;

        auto hash = ::SerializeHash(qc);
        {
            LOCK(cs_main);
            RemoveDataRequest(-1, CInv(MSG_QUORUM_FINAL_COMMITMENT, hash));
        }

        if (qc.IsNull()) {
            LOCK(cs_main);
            LogPrintf("CQuorumBlockProcessor::%s -- null commitment from peer=%d\n", __func__, pfrom->GetId());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if (!Params().GetConsensus().llmqs.count((Consensus::LLMQType)qc.llmqType)) {
            LOCK(cs_main);
            LogPrintf("llmq""CQuorumBlockProcessor::%s -- invalid commitment type %d from peer=%d\n", __func__,
                    qc.llmqType, pfrom->GetId());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }
        auto type = (Consensus::LLMQType)qc.llmqType;
        const auto& params = Params().GetConsensus().llmqs.at(type);

        // Verify that quorumHash is part of the active chain and that it's the first block in the DKG interval
        const CBlockIndex* pquorumIndex;
        {
            LOCK(cs_main);
            if (!::BlockIndex().count(qc.quorumHash)) {
                LogPrintf("CQuorumBlockProcessor::%s -- unknown block %s in commitment, peer=%d\n", __func__,
                        qc.quorumHash.ToString(), pfrom->GetId());
                // can't really punish the node here, as we might simply be the one that is on the wrong chain or not
                // fully synced
                return;
            }
            pquorumIndex = ::BlockIndex()[qc.quorumHash];
            if (ChainActive().Tip()->GetAncestor(pquorumIndex->nHeight) != pquorumIndex) {
                LogPrintf("CQuorumBlockProcessor::%s -- block %s not in active chain, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->GetId());
                // same, can't punish
                return;
            }
            int quorumHeight = pquorumIndex->nHeight - (pquorumIndex->nHeight % params.dkgInterval);
            if (quorumHeight != pquorumIndex->nHeight) {
                LogPrintf("CQuorumBlockProcessor::%s -- block %s is not the first block in the DKG interval, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->GetId());
                Misbehaving(pfrom->GetId(), 100);
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

        auto members = CLLMQUtils::GetAllQuorumMembers(type, pquorumIndex);

        if (!qc.Verify(members, true)) {
            LOCK(cs_main);
            LogPrintf("CQuorumBlockProcessor::%s -- commitment for quorum %s:%d is not valid, peer=%d\n", __func__,
                      qc.quorumHash.ToString(), qc.llmqType, pfrom->GetId());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- received commitment for quorum %s:%d, validMembers=%d, signers=%d, peer=%d\n", __func__,
                  qc.quorumHash.ToString(), qc.llmqType, qc.CountValidMembers(), qc.CountSigners(), pfrom->GetId());

        AddMinableCommitment(qc);
    }
}

bool CQuorumBlockProcessor::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& state)
{
    AssertLockHeld(cs_main);

    if (pindex->nHeight < Params().GetConsensus().nLLMQActivationHeight) {
        specialDb.Write(DB_BEST_BLOCK_UPGRADE, block.GetHash());
        return true;
    }

    std::map<Consensus::LLMQType, CFinalCommitment> qcs;
    if (!GetCommitmentsFromBlock(block, pindex, qcs, state)) {
        return false;
    }

    // The following checks make sure that there is always a (possibly null) commitment while in the mining phase
    // until the first non-null commitment has been mined. After the non-null commitment, no other commitments are
    // allowed, including null commitments.
    for (const auto& p : Params().GetConsensus().llmqs) {
        auto type = p.first;

        // does the currently processed block contain a (possibly null) commitment for the current session?
        bool hasCommitmentInNewBlock = qcs.count(type) != 0;
        bool isCommitmentRequired = IsCommitmentRequired(type, pindex->nHeight);

        if (hasCommitmentInNewBlock && !isCommitmentRequired) {
            // If we're either not in the mining phase or a non-null commitment was mined already, reject the block
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-not-allowed");
        }

        if (!hasCommitmentInNewBlock && isCommitmentRequired) {
            // If no non-null commitment was mined for the mining phase yet and the new block does not include
            // a (possibly null) commitment, the block should be rejected.
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-missing");
        }
    }

    auto blockHash = block.GetHash();

    for (auto& p : qcs) {
        auto& qc = p.second;
        if (!ProcessCommitment(pindex->nHeight, blockHash, qc, state)) {
            return false;
        }
    }

    specialDb.Write(DB_BEST_BLOCK_UPGRADE, blockHash);

    return true;
}

// We store a mapping from minedHeight->quorumHeight in the DB
// minedHeight is inversed so that entries are traversable in reversed order
static std::tuple<std::string, uint8_t, uint32_t> BuildInversedHeightKey(Consensus::LLMQType llmqType, int nMinedHeight)
{
    // nMinedHeight must be converted to big endian to make it comparable when serialized
    return std::make_tuple(DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT, (uint8_t)llmqType, htobe32(std::numeric_limits<uint32_t>::max() - nMinedHeight));
}

bool CQuorumBlockProcessor::ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, CValidationState& state)
{
    auto& params = Params().GetConsensus().llmqs.at((Consensus::LLMQType)qc.llmqType);

    uint256 quorumHash = GetQuorumBlockHash((Consensus::LLMQType)qc.llmqType, nHeight);
    if (quorumHash.IsNull()) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-block");
    }
    if (quorumHash != qc.quorumHash) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-block");
    }

    if (qc.IsNull()) {
        if (!qc.VerifyNull()) {
            return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-invalid-null");
        }
        return true;
    }

    if (HasMinedCommitment(params.type, quorumHash)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-dup");
    }

    if (!IsMiningPhase(params.type, nHeight)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-height");
    }

    auto quorumIndex = ::BlockIndex().at(qc.quorumHash);
    auto members = CLLMQUtils::GetAllQuorumMembers(params.type, quorumIndex);

    if (!qc.Verify(members, true)) {
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-invalid");
    }

    // Store commitment in DB
    specialDb.Write(std::make_pair(DB_MINED_COMMITMENT, std::make_pair((uint8_t)params.type, quorumHash)), std::make_pair(qc, blockHash));
    specialDb.Write(BuildInversedHeightKey(params.type, nHeight), quorumIndex->nHeight);

    {
        LOCK(minableCommitmentsCs);
        hasMinedCommitmentCache.erase(std::make_pair(params.type, quorumHash));
    }

    LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- processed commitment from block. type=%d, quorumHash=%s, signers=%s, validMembers=%d, quorumPublicKey=%s\n", __func__,
              qc.llmqType, quorumHash.ToString(), qc.CountSigners(), qc.CountValidMembers(), qc.quorumPublicKey.ToString());

    return true;
}

bool CQuorumBlockProcessor::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    AssertLockHeld(cs_main);

    std::map<Consensus::LLMQType, CFinalCommitment> qcs;
    CValidationState dummy;
    if (!GetCommitmentsFromBlock(block, pindex, qcs, dummy)) {
        return false;
    }

    for (auto& p : qcs) {
        auto& qc = p.second;
        if (qc.IsNull()) {
            continue;
        }

        specialDb.Erase(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(qc.llmqType, qc.quorumHash)));
        specialDb.Erase(BuildInversedHeightKey((Consensus::LLMQType)qc.llmqType, pindex->nHeight));
        {
            LOCK(minableCommitmentsCs);
            hasMinedCommitmentCache.erase(std::make_pair((Consensus::LLMQType)qc.llmqType, qc.quorumHash));
        }

        // if a reorg happened, we should allow to mine this commitment later
        AddMinableCommitment(qc);
    }

    specialDb.Write(DB_BEST_BLOCK_UPGRADE, pindex->pprev->GetBlockHash());

    return true;
}

bool CQuorumBlockProcessor::GetCommitmentsFromBlock(const CBlock& block, const CBlockIndex* pindex, std::map<Consensus::LLMQType, CFinalCommitment>& ret, CValidationState& state)
{
    AssertLockHeld(cs_main);

    ret.clear();

    for (const auto& tx : block.vtx) {
        if (tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            CFinalCommitmentTxPayload qc;
            if (!GetTxPayload(*tx, qc)) {
                // should not happen as it was verified before processing the block
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-tx-payload");
            }

            // only allow one commitment per type and per block
            if (ret.count((Consensus::LLMQType)qc.commitment.llmqType)) {
                return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-dup");
            }

            ret.emplace((Consensus::LLMQType)qc.commitment.llmqType, std::move(qc.commitment));
        }
    }

    if (pindex->nHeight < Params().GetConsensus().nLLMQActivationHeight && !ret.empty())
        return state.Invalid(ValidationInvalidReason::CONSENSUS, false, REJECT_INVALID, "bad-qc-premature");

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

bool CQuorumBlockProcessor::IsCommitmentRequired(Consensus::LLMQType llmqType, int nHeight)
{
    uint256 quorumHash = GetQuorumBlockHash(llmqType, nHeight);

    // perform extra check for quorumHash.IsNull as the quorum hash is unknown for the first block of a session
    // this is because the currently processed block's hash will be the quorumHash of this session
    bool isMiningPhase = !quorumHash.IsNull() && IsMiningPhase(llmqType, nHeight);

    // did we already mine a non-null commitment for this session?
    bool hasMinedCommitment = !quorumHash.IsNull() && HasMinedCommitment(llmqType, quorumHash);

    return isMiningPhase && !hasMinedCommitment;
}

// WARNING: This method returns uint256() on the first block of the DKG interval (because the block hash is not known yet)
uint256 CQuorumBlockProcessor::GetQuorumBlockHash(Consensus::LLMQType llmqType, int nHeight)
{
    AssertLockHeld(cs_main);

    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    int quorumStartHeight = nHeight - (nHeight % params.dkgInterval);
    uint256 quorumBlockHash;
    if (!GetBlockHash(quorumBlockHash, quorumStartHeight)) {
        return uint256();
    }
    return quorumBlockHash;
}

bool CQuorumBlockProcessor::HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    auto cacheKey = std::make_pair(llmqType, quorumHash);
    {
        LOCK(minableCommitmentsCs);
        auto cacheIt = hasMinedCommitmentCache.find(cacheKey);
        if (cacheIt != hasMinedCommitmentCache.end()) {
            return cacheIt->second;
        }
    }

    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair((uint8_t)llmqType, quorumHash));
    bool ret = specialDb.Exists(key);

    LOCK(minableCommitmentsCs);
    hasMinedCommitmentCache.emplace(cacheKey, ret);
    return ret;
}

bool CQuorumBlockProcessor::GetMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash, CFinalCommitment& retQc, uint256& retMinedBlockHash)
{
    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair((uint8_t)llmqType, quorumHash));
    std::pair<CFinalCommitment, uint256> p;
    if (!specialDb.Read(key, p)) {
        return false;
    }
    retQc = std::move(p.first);
    retMinedBlockHash = p.second;
    return true;
}

std::vector<const CBlockIndex*> CQuorumBlockProcessor::GetMinedCommitmentsUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t maxCount)
{
    auto dbIt = specialDb.GetCurTransaction().NewIteratorUniquePtr();

    auto firstKey = BuildInversedHeightKey(llmqType, pindex->nHeight);
    auto lastKey = BuildInversedHeightKey(llmqType, 0);

    dbIt->Seek(firstKey);

    std::vector<const CBlockIndex*> ret;
    ret.reserve(maxCount);

    while (dbIt->Valid() && ret.size() < maxCount) {
        decltype(firstKey) curKey;
        int quorumHeight;
        if (!dbIt->GetKey(curKey) || curKey >= lastKey) {
            break;
        }
        if (std::get<0>(curKey) != DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT || std::get<1>(curKey) != (uint8_t)llmqType) {
            break;
        }

        uint32_t nMinedHeight = std::numeric_limits<uint32_t>::max() - be32toh(std::get<2>(curKey));
        if (nMinedHeight > pindex->nHeight) {
            break;
        }

        if (!dbIt->GetValue(quorumHeight)) {
            break;
        }

        auto quorumIndex = pindex->GetAncestor(quorumHeight);
        assert(quorumIndex);
        ret.emplace_back(quorumIndex);

        dbIt->Next();
    }

    return ret;
}

std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> CQuorumBlockProcessor::GetMinedAndActiveCommitmentsUntilBlock(const CBlockIndex* pindex)
{
    std::map<Consensus::LLMQType, std::vector<const CBlockIndex*>> ret;

    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& v = ret[p.second.type];
        v.reserve(p.second.signingActiveQuorumCount);
        auto commitments = GetMinedCommitmentsUntilBlock(p.second.type, pindex, p.second.signingActiveQuorumCount);
        for (auto& c : commitments) {
            v.emplace_back(c);
        }
    }

    return ret;
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
        g_connman->RelayInv(inv);
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
bool CQuorumBlockProcessor::GetMinableCommitment(Consensus::LLMQType llmqType, int nHeight, CFinalCommitment& ret)
{
    AssertLockHeld(cs_main);

    if (!IsCommitmentRequired(llmqType, nHeight)) {
        // no commitment required
        return false;
    }

    uint256 quorumHash = GetQuorumBlockHash(llmqType, nHeight);
    if (quorumHash.IsNull()) {
        return false;
    }

    LOCK(minableCommitmentsCs);

    auto k = std::make_pair(llmqType, quorumHash);
    auto it = minableCommitmentsByQuorum.find(k);
    if (it == minableCommitmentsByQuorum.end()) {
        // null commitment required
        ret = CFinalCommitment(Params().GetConsensus().llmqs.at(llmqType), quorumHash);
        return true;
    }

    ret = minableCommitments.at(it->second);

    return true;
}

bool CQuorumBlockProcessor::GetMinableCommitmentTx(Consensus::LLMQType llmqType, int nHeight, CTransactionRef& ret)
{
    AssertLockHeld(cs_main);

    CFinalCommitmentTxPayload qc;
    if (!GetMinableCommitment(llmqType, nHeight, qc.commitment)) {
        return false;
    }

    qc.nHeight = nHeight;

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nType = TRANSACTION_QUORUM_COMMITMENT;
    SetTxPayload(tx, qc);

    ret = MakeTransactionRef(tx);

    return true;
}

}
