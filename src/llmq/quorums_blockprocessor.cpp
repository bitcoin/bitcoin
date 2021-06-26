// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums_blockprocessor.h>
#include <llmq/quorums_commitment.h>
#include <llmq/quorums_debug.h>

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
#include <validation.h>
#include <saltedhasher.h>
#include <sync.h>

#include <map>

namespace llmq
{

CQuorumBlockProcessor* quorumBlockProcessor;

static const std::string DB_MINED_COMMITMENT = "q_mc";
static const std::string DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT = "q_mcih";

static const std::string DB_BEST_BLOCK_UPGRADE = "q_bbu2";

CQuorumBlockProcessor::CQuorumBlockProcessor(CEvoDB &_evoDb) :
    evoDb(_evoDb)
{
    CLLMQUtils::InitQuorumsCache(mapHasMinedCommitmentCache);
}

void CQuorumBlockProcessor::ProcessMessage(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv)
{
    if (strCommand == NetMsgType::QFCOMMITMENT) {
        CFinalCommitment qc;
        vRecv >> qc;

        auto hash = ::SerializeHash(qc);
        {
            LOCK(cs_main);
            EraseObjectRequest(pfrom->GetId(), CInv(MSG_QUORUM_FINAL_COMMITMENT, hash));
        }

        if (qc.IsNull()) {
            LOCK(cs_main);
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- null commitment from peer=%d\n", __func__, pfrom->GetId());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        if (!Params().GetConsensus().llmqs.count(qc.llmqType)) {
            LOCK(cs_main);
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- invalid commitment type %d from peer=%d\n", __func__,
                    qc.llmqType, pfrom->GetId());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }
        auto type = qc.llmqType;
        const auto& params = Params().GetConsensus().llmqs.at(type);

        // Verify that quorumHash is part of the active chain and that it's the first block in the DKG interval
        const CBlockIndex* pquorumIndex;
        {
            LOCK(cs_main);
            pquorumIndex = LookupBlockIndex(qc.quorumHash);
            if (!pquorumIndex) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- unknown block %s in commitment, peer=%d\n", __func__,
                        qc.quorumHash.ToString(), pfrom->GetId());
                // can't really punish the node here, as we might simply be the one that is on the wrong chain or not
                // fully synced
                return;
            }
            if (chainActive.Tip()->GetAncestor(pquorumIndex->nHeight) != pquorumIndex) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s not in active chain, peer=%d\n", __func__,
                          qc.quorumHash.ToString(), pfrom->GetId());
                // same, can't punish
                return;
            }
            int quorumHeight = pquorumIndex->nHeight - (pquorumIndex->nHeight % params.dkgInterval);
            if (quorumHeight != pquorumIndex->nHeight) {
                LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- block %s is not the first block in the DKG interval, peer=%d\n", __func__,
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
                if (jt != minableCommitments.end() && jt->second.CountSigners() <= qc.CountSigners()) {
                    return;
                }
            }
        }

        if (!qc.Verify(pquorumIndex, true)) {
            LOCK(cs_main);
            LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- commitment for quorum %s:%d is not valid, peer=%d\n", __func__,
                      qc.quorumHash.ToString(), qc.llmqType, pfrom->GetId());
            Misbehaving(pfrom->GetId(), 100);
            return;
        }

        LogPrint(BCLog::LLMQ, "CQuorumBlockProcessor::%s -- received commitment for quorum %s:%d, validMembers=%d, signers=%d, peer=%d\n", __func__,
                  qc.quorumHash.ToString(), qc.llmqType, qc.CountValidMembers(), qc.CountSigners(), pfrom->GetId());

        AddMineableCommitment(qc);
    }
}

bool CQuorumBlockProcessor::ProcessBlock(const CBlock& block, const CBlockIndex* pindex, CValidationState& state, bool fJustCheck)
{
    AssertLockHeld(cs_main);

    bool fDIP0003Active = pindex->nHeight >= Params().GetConsensus().DIP0003Height;
    if (!fDIP0003Active) {
        evoDb.Write(DB_BEST_BLOCK_UPGRADE, block.GetHash());
        return true;
    }

    std::map<Consensus::LLMQType, CFinalCommitment> qcs;
    if (!GetCommitmentsFromBlock(block, pindex, qcs, state)) {
        return false;
    }

    // The following checks make sure that there is always a (possibly null) commitment while in the mining phase
    // until the first non-null commitment has been mined. After the non-null commitment, no other commitments are
    // allowed, including null commitments.
    // Note: must only check quorums that were enabled at the _previous_ block height to match mining logic
    for (const auto& type : CLLMQUtils::GetEnabledQuorumTypes(pindex->pprev)) {
        // skip these checks when replaying blocks after the crash
        if (!chainActive.Tip()) {
            break;
        }

        // does the currently processed block contain a (possibly null) commitment for the current session?
        bool hasCommitmentInNewBlock = qcs.count(type) != 0;
        bool isCommitmentRequired = IsCommitmentRequired(type, pindex->nHeight);

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

    auto blockHash = block.GetHash();

    for (const auto& p : qcs) {
        const auto& qc = p.second;
        if (!ProcessCommitment(pindex->nHeight, blockHash, qc, state, fJustCheck)) {
            return false;
        }
    }

    evoDb.Write(DB_BEST_BLOCK_UPGRADE, blockHash);

    return true;
}

// We store a mapping from minedHeight->quorumHeight in the DB
// minedHeight is inversed so that entries are traversable in reversed order
static std::tuple<std::string, Consensus::LLMQType, uint32_t> BuildInversedHeightKey(Consensus::LLMQType llmqType, int nMinedHeight)
{
    // nMinedHeight must be converted to big endian to make it comparable when serialized
    return std::make_tuple(DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT, llmqType, htobe32(std::numeric_limits<uint32_t>::max() - nMinedHeight));
}

bool CQuorumBlockProcessor::ProcessCommitment(int nHeight, const uint256& blockHash, const CFinalCommitment& qc, CValidationState& state, bool fJustCheck)
{
    auto& params = Params().GetConsensus().llmqs.at(qc.llmqType);

    uint256 quorumHash = GetQuorumBlockHash(qc.llmqType, nHeight);

    // skip `bad-qc-block` checks below when replaying blocks after the crash
    if (!chainActive.Tip()) {
        quorumHash = qc.quorumHash;
    }

    if (quorumHash.IsNull()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-block");
    }
    if (quorumHash != qc.quorumHash) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-block");
    }

    if (qc.IsNull()) {
        if (!qc.VerifyNull()) {
            return state.DoS(100, false, REJECT_INVALID, "bad-qc-invalid-null");
        }
        return true;
    }

    if (HasMinedCommitment(params.type, quorumHash)) {
        // should not happen as it's already handled in ProcessBlock
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-dup");
    }

    if (!IsMiningPhase(params.type, nHeight)) {
        // should not happen as it's already handled in ProcessBlock
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-height");
    }

    auto quorumIndex = LookupBlockIndex(qc.quorumHash);

    if (!qc.Verify(quorumIndex, true)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-qc-invalid");
    }

    if (fJustCheck) {
        return true;
    }

    // Store commitment in DB
    auto cacheKey = std::make_pair(params.type, quorumHash);
    evoDb.Write(std::make_pair(DB_MINED_COMMITMENT, cacheKey), std::make_pair(qc, blockHash));
    evoDb.Write(BuildInversedHeightKey(params.type, nHeight), quorumIndex->nHeight);

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

        evoDb.Erase(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(qc.llmqType, qc.quorumHash)));
        evoDb.Erase(BuildInversedHeightKey(qc.llmqType, pindex->nHeight));
        {
            LOCK(minableCommitmentsCs);
            mapHasMinedCommitmentCache[qc.llmqType].erase(qc.quorumHash);
        }

        // if a reorg happened, we should allow to mine this commitment later
        AddMineableCommitment(qc);
    }

    evoDb.Write(DB_BEST_BLOCK_UPGRADE, pindex->pprev->GetBlockHash());

    return true;
}

// TODO remove this with 0.15.0
bool CQuorumBlockProcessor::UpgradeDB()
{
    LOCK(cs_main);

    if (chainActive.Tip() == nullptr) {
        // should have no records
        return evoDb.IsEmpty();
    }

    uint256 bestBlock;
    if (evoDb.GetRawDB().Read(DB_BEST_BLOCK_UPGRADE, bestBlock) && bestBlock == chainActive.Tip()->GetBlockHash()) {
        return true;
    }

    LogPrintf("CQuorumBlockProcessor::%s -- Upgrading DB...\n", __func__);

    if (chainActive.Height() >= Params().GetConsensus().DIP0003EnforcementHeight) {
        auto pindex = chainActive[Params().GetConsensus().DIP0003EnforcementHeight];
        while (pindex) {
            if (fPruneMode && !(pindex->nStatus & BLOCK_HAVE_DATA)) {
                // Too late, we already pruned blocks we needed to reprocess commitments
                return false;
            }
            CBlock block;
            bool r = ReadBlockFromDisk(block, pindex, Params().GetConsensus());
            assert(r);

            std::map<Consensus::LLMQType, CFinalCommitment> qcs;
            CValidationState dummyState;
            GetCommitmentsFromBlock(block, pindex, qcs, dummyState);

            for (const auto& p : qcs) {
                const auto& qc = p.second;
                if (qc.IsNull()) {
                    continue;
                }
                auto quorumIndex = LookupBlockIndex(qc.quorumHash);
                evoDb.GetRawDB().Write(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(qc.llmqType, qc.quorumHash)), std::make_pair(qc, pindex->GetBlockHash()));
                evoDb.GetRawDB().Write(BuildInversedHeightKey(qc.llmqType, pindex->nHeight), quorumIndex->nHeight);
            }

            evoDb.GetRawDB().Write(DB_BEST_BLOCK_UPGRADE, pindex->GetBlockHash());

            pindex = chainActive.Next(pindex);
        }
    }

    LogPrintf("CQuorumBlockProcessor::%s -- Upgrade done...\n", __func__);
    return true;
}

bool CQuorumBlockProcessor::GetCommitmentsFromBlock(const CBlock& block, const CBlockIndex* pindex, std::map<Consensus::LLMQType, CFinalCommitment>& ret, CValidationState& state)
{
    AssertLockHeld(cs_main);

    auto& consensus = Params().GetConsensus();
    bool fDIP0003Active = pindex->nHeight >= consensus.DIP0003Height;

    ret.clear();

    for (const auto& tx : block.vtx) {
        if (tx->nType == TRANSACTION_QUORUM_COMMITMENT) {
            CFinalCommitmentTxPayload qc;
            if (!GetTxPayload(*tx, qc)) {
                // should not happen as it was verified before processing the block
                return state.DoS(100, false, REJECT_INVALID, "bad-qc-payload");
            }

            // only allow one commitment per type and per block
            if (ret.count(qc.commitment.llmqType)) {
                return state.DoS(100, false, REJECT_INVALID, "bad-qc-dup");
            }

            ret.emplace(qc.commitment.llmqType, std::move(qc.commitment));
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
        return {};
    }
    return quorumBlockHash;
}

bool CQuorumBlockProcessor::HasMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash)
{
    bool fExists;
    {
        LOCK(minableCommitmentsCs);
        if (mapHasMinedCommitmentCache[llmqType].get(quorumHash, fExists)) {
            return fExists;
        }
    }

    fExists = evoDb.Exists(std::make_pair(DB_MINED_COMMITMENT, std::make_pair(llmqType, quorumHash)));

    LOCK(minableCommitmentsCs);
    mapHasMinedCommitmentCache[llmqType].insert(quorumHash, fExists);

    return fExists;
}

CFinalCommitmentPtr CQuorumBlockProcessor::GetMinedCommitment(Consensus::LLMQType llmqType, const uint256& quorumHash, uint256& retMinedBlockHash)
{
    auto key = std::make_pair(DB_MINED_COMMITMENT, std::make_pair(llmqType, quorumHash));
    std::pair<CFinalCommitment, uint256> p;
    if (!evoDb.Read(key, p)) {
        return nullptr;
    }
    retMinedBlockHash = p.second;
    return std::make_shared<CFinalCommitment>(p.first);
}

// The returned quorums are in reversed order, so the most recent one is at index 0
std::vector<const CBlockIndex*> CQuorumBlockProcessor::GetMinedCommitmentsUntilBlock(Consensus::LLMQType llmqType, const CBlockIndex* pindex, size_t maxCount)
{
    LOCK(evoDb.cs);

    auto dbIt = evoDb.GetCurTransaction().NewIteratorUniquePtr();

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
        if (std::get<0>(curKey) != DB_MINED_COMMITMENT_BY_INVERSED_HEIGHT || std::get<1>(curKey) != llmqType) {
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

// The returned quorums are in reversed order, so the most recent one is at index 0
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
        auto ins = minableCommitmentsByQuorum.emplace(k, commitmentHash);
        if (ins.second) {
            minableCommitments.emplace(commitmentHash, fqc);
            relay = true;
        } else {
            const auto& oldFqc = minableCommitments.at(ins.first->second);
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
// Will return true and a null commitment if no mineable commitment is known and none was mined yet
bool CQuorumBlockProcessor::GetMineableCommitment(Consensus::LLMQType llmqType, int nHeight, CFinalCommitment& ret)
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

bool CQuorumBlockProcessor::GetMineableCommitmentTx(Consensus::LLMQType llmqType, int nHeight, CTransactionRef& ret)
{
    AssertLockHeld(cs_main);

    CFinalCommitmentTxPayload qc;
    if (!GetMineableCommitment(llmqType, nHeight, qc.commitment)) {
        return false;
    }

    qc.nHeight = nHeight;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_QUORUM_COMMITMENT;
    SetTxPayload(tx, qc);

    ret = MakeTransactionRef(tx);

    return true;
}

} // namespace llmq
