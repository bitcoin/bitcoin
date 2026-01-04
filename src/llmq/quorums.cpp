// Copyright (c) 2018-2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <llmq/quorums.h>

#include <evo/deterministicmns.h>
#include <llmq/commitment.h>

#include <dbwrapper.h>

#include <memory>

namespace llmq {
const std::string DB_QUORUM_SK_SHARE = "q_Qsk";
const std::string DB_QUORUM_QUORUM_VVEC = "q_Qqvvec";

uint256 MakeQuorumKey(const CQuorum& q)
{
    CHashWriter hw(SER_NETWORK, 0);
    hw << q.params.type;
    hw << q.qc->quorumHash;
    for (const auto& dmn : q.members) {
        hw << dmn->proTxHash;
    }
    return hw.GetHash();
}

void DataCleanupHelper(CDBWrapper& db, const std::set<uint256>& skip_list, bool compact)
{
    const auto prefixes = {DB_QUORUM_QUORUM_VVEC, DB_QUORUM_SK_SHARE};

    CDBBatch batch(db);
    std::unique_ptr<CDBIterator> pcursor(db.NewIterator());

    for (const auto& prefix : prefixes) {
        auto start = std::make_tuple(prefix, uint256());
        pcursor->Seek(start);

        int count{0};
        while (pcursor->Valid()) {
            decltype(start) k;

            if (!pcursor->GetKey(k) || std::get<0>(k) != prefix) {
                break;
            }

            pcursor->Next();

            if (skip_list.find(std::get<1>(k)) != skip_list.end()) continue;

            ++count;
            batch.Erase(k);

            if (batch.SizeEstimate() >= (1 << 24)) {
                db.WriteBatch(batch);
                batch.Clear();
            }
        }

        db.WriteBatch(batch);

        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- %s removed %d\n", __func__, prefix, count);
    }

    pcursor.reset();

    if (compact) {
        // Avoid using this on regular cleanups, use on db migrations only
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- compact start\n", __func__);
        db.CompactFull();
        LogPrint(BCLog::LLMQ, "CQuorumManager::%s -- compact end\n", __func__);
    }
}

std::string CQuorumDataRequest::GetErrorString() const
{
    switch (nError) {
        case (Errors::NONE):
            return "NONE";
        case (Errors::QUORUM_TYPE_INVALID):
            return "QUORUM_TYPE_INVALID";
        case (Errors::QUORUM_BLOCK_NOT_FOUND):
            return "QUORUM_BLOCK_NOT_FOUND";
        case (Errors::QUORUM_NOT_FOUND):
            return "QUORUM_NOT_FOUND";
        case (Errors::MASTERNODE_IS_NO_MEMBER):
            return "MASTERNODE_IS_NO_MEMBER";
        case (Errors::QUORUM_VERIFICATION_VECTOR_MISSING):
            return "QUORUM_VERIFICATION_VECTOR_MISSING";
        case (Errors::ENCRYPTED_CONTRIBUTIONS_MISSING):
            return "ENCRYPTED_CONTRIBUTIONS_MISSING";
        case (Errors::UNDEFINED):
            return "UNDEFINED";
        default:
            return "UNDEFINED";
    }
    return "UNDEFINED";
}

CQuorum::CQuorum(const Consensus::LLMQParams& _params, CBLSWorker& _blsWorker) : params(_params), blsCache(_blsWorker)
{
}

void CQuorum::Init(CFinalCommitmentPtr _qc, const CBlockIndex* _pQuorumBaseBlockIndex, const uint256& _minedBlockHash, Span<CDeterministicMNCPtr> _members)
{
    qc = std::move(_qc);
    m_quorum_base_block_index = _pQuorumBaseBlockIndex;
    members = std::vector(_members.begin(), _members.end());
    minedBlockHash = _minedBlockHash;
}

bool CQuorum::SetVerificationVector(const std::vector<CBLSPublicKey>& quorumVecIn)
{
    const auto quorumVecInSerialized = ::SerializeHash(quorumVecIn);

    LOCK(cs_vvec_shShare);
    if (quorumVecInSerialized != qc->quorumVvecHash) {
        return false;
    }
    quorumVvec = std::make_shared<std::vector<CBLSPublicKey>>(quorumVecIn);
    return true;
}

bool CQuorum::SetSecretKeyShare(const CBLSSecretKey& secretKeyShare, const uint256& protx_hash)
{
    if (protx_hash.IsNull() || !secretKeyShare.IsValid() || (secretKeyShare.GetPublicKey() != GetPubKeyShare(GetMemberIndex(protx_hash)))) {
        return false;
    }
    LOCK(cs_vvec_shShare);
    skShare = secretKeyShare;
    return true;
}

bool CQuorum::IsMember(const uint256& proTxHash) const
{
    return ranges::any_of(members, [&proTxHash](const auto& dmn){
        return dmn->proTxHash == proTxHash;
    });
}

bool CQuorum::IsValidMember(const uint256& proTxHash) const
{
    for (const auto i : irange::range(members.size())) {
        // cppcheck-suppress useStlAlgorithm
        if (members[i]->proTxHash == proTxHash) {
            return qc->validMembers[i];
        }
    }
    return false;
}

CBLSPublicKey CQuorum::GetPubKeyShare(size_t memberIdx) const
{
    LOCK(cs_vvec_shShare);
    if (!HasVerificationVectorInternal() || memberIdx >= members.size() || !qc->validMembers[memberIdx]) {
        return CBLSPublicKey();
    }
    const auto& m = members[memberIdx];
    return blsCache.BuildPubKeyShare(m->proTxHash, quorumVvec, CBLSId(m->proTxHash));
}

bool CQuorum::HasVerificationVector() const {
    LOCK(cs_vvec_shShare);
    return HasVerificationVectorInternal();
}

bool CQuorum::HasVerificationVectorInternal() const {
    AssertLockHeld(cs_vvec_shShare);
    return quorumVvec != nullptr;
}

CBLSSecretKey CQuorum::GetSkShare() const
{
    LOCK(cs_vvec_shShare);
    return skShare;
}

int CQuorum::GetMemberIndex(const uint256& proTxHash) const
{
    for (const auto i : irange::range(members.size())) {
        // cppcheck-suppress useStlAlgorithm
        if (members[i]->proTxHash == proTxHash) {
            return int(i);
        }
    }
    return -1;
}

void CQuorum::WriteContributions(CDBWrapper& db) const
{
    uint256 dbKey = MakeQuorumKey(*this);

    LOCK(cs_vvec_shShare);
    if (HasVerificationVectorInternal()) {
        CDataStream s(SER_DISK, CLIENT_VERSION);
        WriteCompactSize(s, quorumVvec->size());
        for (auto& pubkey : *quorumVvec) {
            s << CBLSPublicKeyVersionWrapper(pubkey, false);
        }
        db.Write(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), s);
    }
    if (skShare.IsValid()) {
        db.Write(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);
    }
}

bool CQuorum::ReadContributions(const CDBWrapper& db)
{
    uint256 dbKey = MakeQuorumKey(*this);
    CDataStream s(SER_DISK, CLIENT_VERSION);

    if (!db.ReadDataStream(std::make_pair(DB_QUORUM_QUORUM_VVEC, dbKey), s)) {
        return false;
    }

    size_t vvec_size = ReadCompactSize(s);
    CBLSPublicKey pubkey;
    std::vector<CBLSPublicKey> qv;
    for ([[maybe_unused]] size_t _ : irange::range(vvec_size)) {
        s >> CBLSPublicKeyVersionWrapper(pubkey, false);
        qv.emplace_back(pubkey);
    }

    LOCK(cs_vvec_shShare);
    quorumVvec = std::make_shared<std::vector<CBLSPublicKey>>(std::move(qv));
    // We ignore the return value here as it is ok if this fails. If it fails, it usually means that we are not a
    // member of the quorum but observed the whole DKG process to have the quorum verification vector.
    db.Read(std::make_pair(DB_QUORUM_SK_SHARE, dbKey), skShare);

    return true;
}
} // namespace llmq
