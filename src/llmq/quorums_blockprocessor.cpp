// Copyright (c) 2018-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_debug.h>
#include <llmq/quorums_utils.h>

#include <evo/evodb.h>
#include <evo/specialtx.h>

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <net.h>
#include <net_processing.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <node/blockstorage.h>
#include <validation.h>
#include <saltedhasher.h>
#include <sync.h>

#include <map>
namespace llmq
{

CQuorumBlockProcessor* quorumBlockProcessor;

static const std::string DB_MINED_COMMITMENT = "q_mc";
static const std::string DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT = "q_mcih";


CQuorumBlockProcessor::CQuorumBlockProcessor(CConnman &_connman, ChainstateManager& _chainman) : connman(_connman), chainman(_chainman)
{
    CLLMQUtils::InitQuorumsCache(mapHasMinedCommitmentCache);
}

void CQuorumBlockProcessor::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, PeerManager& peerman)
{
    if (strCommand == NetMsgType::QFCOMMITMENT) {
        CFinalCommitment qc;
        vRecv >> qc;

        const uint256& hash = ::SerializeHash(qc);
        {
            LOCK(cs_main);
            pfrom->AddKnownTx(hash);
            peerman.ReceivedResponse(pfrom->GetId(), hash);
        }

        if (qc.IsNull()) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- null commitment from peer=%d\n", __func__, pfrom->GetId());
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            peerman.Misbehaving(pfrom->GetId(), 100, "null commitment from peer");
            return;
        }

        if (!Params().GetConsensus().llmqs.count(qc.llmqType)) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- invalid commitment type %d from peer=%d\n", __func__,
                    qc.llmqType, pfrom->GetId());
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }
            peerman.Misbehaving(pfrom->GetId(), 100, "invalid commitment type");
            return;
        }
        auto type = qc.llmqType;
        const auto& params = Params().GetConsensus().llmqs.at(type);
        // Verify that quorumHash is part of the active chain and that it's the first block in the DKG interval
        const CBlockIndex* pquorumIndex;
        {
            LOCK(cs_main);
            pquorumIndex = chainman.m_blockman.LookupBlockIndex(qc.quorumHash);
            if (!pquorumIndex) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- unknown block %s in commitment, peer=%d\n", __func__,
                        qc.quorumHash.ToString(), pfrom->GetId());
                // can't really punish the node here, as we might simply be the one that is on the wrong chain or not
                // fully synced
                return;
            }
            if (chainman.ActiveTip()->GetAncestor(pquorumIndex->nHeight) != pquorumIndex) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s not in active chain, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->GetId());
                // same, can't punish
                return;
            }
            int quorumHeight = pquorumIndex->nHeight - (pquorumIndex->nHeight % params.dkgInterval);
            if (quorumHeight != pquorumIndex->nHeight) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s is not the first block in the DKG interval, peer=%d\n", __func__,
                            qc.quorumHash.ToString(), pfrom->GetId());
                peerman.ForgetTxHash(pfrom->GetId(), hash);
                peerman.Misbehaving(pfrom->GetId(), 100, "not in first block of DKG interval");
                return;
            }
        }
        bool bReturn = false;
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
                        bReturn = true;
                    }
                }
            }
        }
        if(bReturn) {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }

       if (!qc.Verify(pquorumIndex, true)) {
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- commitment for quorum %s:%d is not valid, peer=%d\n", __func__,
                    qc.quorumHash.ToString(), qc.llmqType, pfrom->GetId());
            {
                LOCK(cs_main);
                peerman.ForgetTxHash(pfrom->GetId(), hash);
            }                     
            peerman.Misbehaving(pfrom->GetId(), 100, "invalid commitment for quorum");
            return;
        }

        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- received commitment for quorum %s:%d, validMembers=%d, signers=%d, peer=%d\n", __func__,
                qc.quorumHash.ToString(), qc.llmqType, qc.CountValidMembers(), qc.CountSigners(), pfrom->GetId());
        {
            LOCK(cs_main);
            peerman.ForgetTxHash(pfrom->GetId(), hash);
        }
        AddMineableCommitment(qc);
    }
}

bool CQuorumBlockProcessor::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, BlockValidationState& state, bool fJustCheck)
{
    AssertLockHeld(cs_main);
    bool fDIP0003Active = pindex->nHeight >= Params().GetConsensus().DIP0003Height;
    if (!fDIP0003Active) {
        return true;
    }


    std::map<uint8_t, CFinalCommitment> qcs;
    if (!GetCommitmentsFromBlock(block, pindex->nHeight, qcs, state)) {
        return false;
    }

    // The following checks make sure that there is always a (possibly null) commitment while in the mining phase
    // until the first non-null commitment has been mined. After the non-null commitment, no other commitments are
    // allowed, including null commitments.
    for (const auto& p : Params().GetConsensus().llmqs) {
        // skip these checks when replaying blocks after the crash
        if (!chainman.ActiveTip()) {
            break;
        }

        auto type = p.first;

        // does the currently processed block contain a (possibly null) commitment for the current session?
        bool hasCommitmentInNewBlock = qcs.count(type) != 0;
        bool isCommitmentRequired = IsCommitmentRequired(type, pindex->nHeight);

        if (hasCommitmentInNewBlock && !isCommitmentRequired) {
            // If we're either not in the mining phase or a non-null commitment was mined already, reject the block
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-not-allowed");
        }

        if (!hasCommitmentInNewBlock && isCommitmentRequired) {
            // If no non-null commitment was mined for the mining phase yet and the new block does not include
            // a (possibly null) commitment, the block should be rejected.
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-missing");
        }
    }

    const auto &blockHash = block.GetHash();

    for (auto& p : qcs) {
        auto& qc = p.second;
        if (!ProcessCommitment(pindex->nHeight, blockHash, qc, state, fJustCheck)) {
            return false;
        }
    }
    return true;
}

// We store a mapping from minedHeight->quorumHeight in the DB
// minedHeight is inversed so that entries are traversable in reversed order
static std::tuple<std::string, uint8_t, uint32_t> BuildInversedHeightKey(uint8_t llmqType, int nMinedHeight)
{
    // nMinedHeight must be converted to big endian to make it comparable when serialized
    return std::make_tuple(DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT, llmqType, htobe32(std::numeric_limits<uint32_t>::max() - (uint32_t)nMinedHeight));
}

bool CQuorumBlockProcessor::ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, BlockValidationState& state, bool fJustCheck)
{
    if(!evoDb)
        return false;
    AssertLockHeld(cs_main);
    auto& params = Params().GetConsensus().llmqs.at(qc.llmqType);

    uint256 quorumHash = GetQuorumBlockHash(chainman, qc.llmqType, nHeight);

    // skip `bad-qc-block` checks below when replaying blocks after the crash
    if (!chainman.ActiveTip()) {
        quorumHash = qc.quorumHash;
    }

    if (quorumHash.IsNull()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block");
    }
    if (quorumHash != qc.quorumHash) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block");
    }

    if (qc.IsNull()) {
        if (!qc.VerifyNull()) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid-null");
        }
        return true;
    }

    if (HasMinedCommitment(params.type, quorumHash)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-dup");
    }

    if (!IsMiningPhase(params.type, nHeight)) {
        // should not happen as it's already handled in ProcessBlock
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-height");
    }

    auto quorumIndex = chainman.m_blockman.LookupBlockIndex(qc.quorumHash);
    if(!quorumIndex) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-block-index");
    }
    if (!qc.Verify(quorumIndex, true)) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-invalid");
    }

    if (fJustCheck) {
        return true;
    }

    // Store commitment in DB
    auto cacheKey = std::make_pair(params.type, quorumHash);
    evoDb->Write(std::make_pair(DB_MINED_COMMITMENT, cacheKey), std::make_pair(qc, blockHash));
    evoDb->Write(BuildInversedHeightKey(params.type, nHeight), quorumIndex->nHeight);

    {
        LOCK(minableCommitmentsCs);
        mapHasMinedCommitmentCache[qc.llmqType].erase(qc.quorumHash);
        minableCommitmentsByQuorum.erase(cacheKey);
        minableCommitments.erase(::SerializeHash(qc));
    }

    LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- processed commitment from block. type=%d, quorumHash=%s, signers=%s, validMembers=%d, quorumPublicKey=%s\n", __func__,
              qc.llmqType, quorumHash.ToString(), qc.CountSigners(), qc.CountValidMembers(), qc.quorumPublicKey.ToString());

    return true;
}

bool CQuorumBlockProcessor::UndoBlock(const CBlock& block, const CBlockIndex* pindex)
{
    if(!evoDb)
        return false;
    AssertLockHeld(cs_main);

    std::map<uint8_t, CFinalCommitment> qcs;
    BlockValidationState dummy;
    if (!GetCommitmentsFromBlock(block, pindex->nHeight, qcs, dummy)) {
        return false;
    }

    for (auto& p : qcs) {
        auto& qc = p.second;
        if (qc.IsNull()) {
            continue;
        }

        evoDb->Erase(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(qc.llmqType, qc.quorumHash)));
        evoDb->Erase(BuildInversedHeightKey(qc.llmqType, pindex->nHeight));
        {
            LOCK(minableCommitmentsCs);
            mapHasMinedCommitmentCache[qc.llmqType].erase(qc.quorumHash);
        }

        // if a reorg happened, we should allow to mine this commitment later
        AddMineableCommitment(qc);
    }
    return true;
}

bool CQuorumBlockProcessor::GetCommitmentsFromBlock(const CBlock& block, const uint32_t& nHeight, std::map<uint8_t, CFinalCommitment>& ret, BlockValidationState& state)
{
    auto& consensus = Params().GetConsensus();
    bool fDIP0003Active = nHeight >= (uint32_t)consensus.DIP0003Height;

    ret.clear();
    
    if (block.vtx[0]->nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT) {
        CFinalCommitmentTxPayload qc;
        if (!GetTxPayload(*block.vtx[0], qc)) {
            // should not happen as it was verified before processing the block
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-payload");
        }
        for(const auto& commitment: qc.commitments) {
            // only allow one commitment per type and per block
            if (ret.count(commitment.llmqType)) {
                return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-dup");
            }

            ret.try_emplace(commitment.llmqType, std::move(commitment));
        }
    }
    

    if (!fDIP0003Active && !ret.empty()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-qc-premature");
    }

    return true;
}

bool CQuorumBlockProcessor::IsMiningPhase(uint8_t llmqType, int nHeight)
{
    const auto& params = Params().GetConsensus().llmqs.at(llmqType);
    int phaseIndex = nHeight % params.dkgInterval;
    if (phaseIndex >= params.dkgMiningWindowStart && phaseIndex <= params.dkgMiningWindowEnd) {
        return true;
    }
    return false;
}

bool CQuorumBlockProcessor::IsCommitmentRequired(uint8_t llmqType, int nHeight)
{
    AssertLockHeld(cs_main);
    uint256 quorumHash = GetQuorumBlockHash(chainman, llmqType, nHeight);

    // perform extra check for quorumHash.IsNull as the quorum hash is unknown for the first block of a session
    // this is because the currently processed block's hash will be the quorumHash of this session
    bool isMiningPhase = !quorumHash.IsNull() && IsMiningPhase(llmqType, nHeight);

    // did we already mine a non-null commitment for this session?
    bool hasMinedCommitment = !quorumHash.IsNull() && HasMinedCommitment(llmqType, quorumHash);

    return isMiningPhase && !hasMinedCommitment;
}

// WARNING: This method returns uint256() on the first block of the DKG interval (because the block hash is not known yet)
uint256 CQuorumBlockProcessor::GetQuorumBlockHash(ChainstateManager& chainman, uint8_t llmqType, int nHeight)
{
    AssertLockHeld(cs_main);
    auto& params = Params().GetConsensus().llmqs.at(llmqType);

    int quorumStartHeight = nHeight - (nHeight % params.dkgInterval);
    uint256 quorumBlockHash;
    if (!GetBlockHash(chainman, quorumBlockHash, quorumStartHeight)) {
        return {};
    }
    return quorumBlockHash;
}

bool CQuorumBlockProcessor::HasMinedCommitment(uint8_t llmqType, const uint256& quorumHash)
{
    if(!evoDb)
        return false;
    bool fExists;
    {
        LOCK(minableCommitmentsCs);
        if (mapHasMinedCommitmentCache[llmqType].get(quorumHash, fExists)) {
            return fExists;
        }
    }

    fExists = evoDb->Exists(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(llmqType, quorumHash)));

    LOCK(minableCommitmentsCs);
    mapHasMinedCommitmentCache[llmqType].insert(quorumHash, fExists);

    return fExists;
}

CFinalCommitmentPtr CQuorumBlockProcessor::GetMinedCommitment(uint8_t llmqType, const uint256& quorumHash, uint256& retMinedBlockHash)
{
    if(!evoDb)
        return nullptr;
    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair(llmqType, quorumHash));
    std::pair<CFinalCommitment, uint256> p;
    if (!evoDb->Read(key, p)) {
        return nullptr;
    }
    retMinedBlockHash = p.second;
    return std::make_shared<CFinalCommitment>(p.first);
}

// The returned quorums are in reversed order, so the most recent one is at index 0
void CQuorumBlockProcessor::GetMinedCommitmentsUntilBlock(uint8_t llmqType, const CBlockIndex* pindex, size_t maxCount, std::vector<const CBlockIndex*> &ret)
{
    if(!evoDb)
        return;
    LOCK(evoDb->cs);
    ret.clear();
    auto dbIt = evoDb->GetCurTransaction().NewIteratorUniquePtr();

    auto firstKey = BuildInversedHeightKey(llmqType, pindex->nHeight);
    auto lastKey = BuildInversedHeightKey(llmqType, 0);

    dbIt->Seek(firstKey);

    ret.reserve(maxCount);
    while (dbIt->Valid() && ret.size() < maxCount) {
        decltype(firstKey) curKey;
        int quorumHeight;
        if (!dbIt->GetKey(curKey) || curKey >= lastKey) {
            break;
        }
        if (std::get<0>(curKey) != DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT || std::get<1>(curKey) != llmqType) {
            break;
        }
        uint32_t nMinedHeight = std::numeric_limits<uint32_t>::max() - be32toh(std::get<2>(curKey));
        if (nMinedHeight > (uint32_t)pindex->nHeight) {
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
}

// The returned quorums are in reversed order, so the most recent one is at index 0
void CQuorumBlockProcessor::GetMinedAndActiveCommitmentsUntilBlock(const CBlockIndex* pindex, std::map<uint8_t, std::vector<const CBlockIndex*>>& ret)
{
    ret.clear();
    for (const auto& p : Params().GetConsensus().llmqs) {
        auto& v = ret[p.second.type];
        v.reserve(p.second.signingActiveQuorumCount);
        std::vector<const CBlockIndex*> commitments;
        GetMinedCommitmentsUntilBlock(p.second.type, pindex, p.second.signingActiveQuorumCount, commitments);
        for (auto& c : commitments) {
            v.emplace_back(c);
        }
    }

}

bool CQuorumBlockProcessor::HasMineableCommitment(const uint256& hash)
{
    LOCK(minableCommitmentsCs);
    return minableCommitments.count(hash) != 0;
}

void CQuorumBlockProcessor::AddMineableCommitment(const CFinalCommitment& fqc)
{
    bool relay = false;
    uint256 commitmentHash = ::SerializeHash(fqc);

    {
        LOCK(minableCommitmentsCs);

        auto k = std::make_pair(fqc.llmqType, fqc.quorumHash);
        auto ins = minableCommitmentsByQuorum.try_emplace(k, commitmentHash);
        if (ins.second) {
            minableCommitments.try_emplace(commitmentHash, fqc);
            relay = true;
        } else {
            auto& oldFqc = minableCommitments.at(ins.first->second);
            if (fqc.CountSigners() > oldFqc.CountSigners()) {
                // new commitment has more signers, so override the known one
                ins.first->second = commitmentHash;
                minableCommitments.erase(ins.first->second);
                minableCommitments.try_emplace(commitmentHash, fqc);
                relay = true;
            }
        }
    }

    // We only relay the new commitment if it's new or better then the old one
    if (relay) {
        CInv inv(MSG_QUORUM_FINAL_COMMITMENT, commitmentHash);
        connman.RelayOtherInv(inv);
    }
}

bool CQuorumBlockProcessor::GetMineableCommitmentByHash(const uint256& commitmentHash, llmq::CFinalCommitment& ret)
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
bool CQuorumBlockProcessor::GetMinableCommitment(uint8_t llmqType, int nHeight, CFinalCommitment& ret)
{
    AssertLockHeld(cs_main);
    if (!IsCommitmentRequired(llmqType, nHeight)) {
        // no commitment required
        return false;
    }

    uint256 quorumHash = GetQuorumBlockHash(chainman, llmqType, nHeight);
    if (quorumHash.IsNull()) {
        return false;
    }
    {
        LOCK(minableCommitmentsCs);

        auto k = std::make_pair(llmqType, quorumHash);
        auto it = minableCommitmentsByQuorum.find(k);
        if (it == minableCommitmentsByQuorum.end()) {
            // null commitment required
            ret = CFinalCommitment(Params().GetConsensus().llmqs.at(llmqType), quorumHash);
            return true;
        }

        ret = minableCommitments.at(it->second);
    }

    return true;
}

} // namespace llmq
