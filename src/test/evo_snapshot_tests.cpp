// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <clientversion.h>
#include <consensus/amount.h>
#include <consensus/merkle.h>
#include <evo/cbtx.h>
#include <evo/chainhelper.h>
#include <evo/creditpool.h>
#include <evo/evodb.h>
#include <evo/mnhftx.h>
#include <evo/netinfo.h>
#include <evo/snapshot.h>
#include <hash.h>
#include <llmq/blockprocessor.h>
#include <llmq/context.h>
#include <llmq/quorumsman.h>
#include <llmq/signhash.h>
#include <llmq/snapshot.h>
#include <llmq/utils.h>
#include <masternode/meta.h>
#include <node/context.h>
#include <streams.h>
#include <validation.h>
#include <versionbits.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

uint256 H(uint8_t value)
{
    uint256 hash;
    hash.begin()[0] = value;
    return hash;
}

uint256 CollidingH(uint8_t suffix)
{
    uint256 hash;
    std::fill_n(hash.begin(), 8, 0xa5);
    hash.begin()[8] = suffix;
    return hash;
}

uint160 H160(uint8_t value)
{
    uint160 hash;
    hash.begin()[0] = value;
    return hash;
}

CDeterministicMNCPtr MN(uint64_t internal_id, uint8_t hash_suffix, MnType type, int version, uint8_t address_tag)
{
    auto state{std::make_shared<CDeterministicMNState>()};
    state->nVersion = version;
    state->nRegisteredHeight = 10 + internal_id;
    state->nLastPaidHeight = 20 + internal_id;
    state->nPoSePenalty = internal_id;
    state->keyIDOwner = CKeyID{H160(address_tag)};
    state->keyIDVoting = CKeyID{H160(address_tag + 20)};
    state->scriptPayout = CScript{} << OP_RETURN << std::vector<unsigned char>{address_tag, 1};
    state->scriptOperatorPayout = CScript{} << OP_RETURN << std::vector<unsigned char>{address_tag, 2};
    state->netInfo = NetInfoInterface::MakeNetInfo(version);
    BOOST_REQUIRE_EQUAL(state->netInfo->AddEntry(NetInfoPurpose::CORE_P2P,
                                                strprintf("1.1.1.%d:%d", address_tag, Params().GetDefaultPort())),
                        NetInfoStatus::Success);
    if (type == MnType::Evo) {
        state->platformNodeID = H160(address_tag + 40);
        // Platform on another address, with a domain first for HTTPS, as DEPLOYMENT_IPV4_OPTIONAL allows, on the
        // standard ports, which mainnet pins. Such states load on every network: the IPv4 rule applies to ProTxs,
        // at the height they are mined at.
        BOOST_REQUIRE_EQUAL(state->netInfo->AddEntry(NetInfoPurpose::PLATFORM_P2P,
                                                    strprintf("2.2.2.%d:26656", address_tag)),
                            NetInfoStatus::Success);
        BOOST_REQUIRE_EQUAL(state->netInfo->AddEntry(NetInfoPurpose::PLATFORM_HTTPS,
                                                    strprintf("evo%d.example.org:443", address_tag)),
                            NetInfoStatus::Success);
    }

    auto dmn{std::make_shared<CDeterministicMN>(internal_id, type)};
    dmn->proTxHash = CollidingH(hash_suffix);
    dmn->collateralOutpoint = COutPoint(H(address_tag + 80), internal_id);
    dmn->nOperatorReward = address_tag * 10;
    state->UpdateConfirmedHash(dmn->proTxHash, H(address_tag + 100));
    dmn->pdmnState = std::move(state);
    return dmn;
}

CDeterministicMNList MNList(const uint256& block_hash, int height, bool reverse)
{
    CDeterministicMNList list{block_hash, height, 10};
    std::vector<CDeterministicMNCPtr> mns{
        MN(2, 3, MnType::Regular, ProTxVersion::LegacyBLS, 3),
        MN(5, 1, MnType::Evo, ProTxVersion::ExtAddr, 5),
        MN(7, 2, MnType::Regular, ProTxVersion::LegacyBLS, 7),
    };
    if (reverse) std::reverse(mns.begin(), mns.end());
    for (const auto& dmn : mns) list.AddMN(dmn, /*fBumpTotalCount=*/false);
    return list;
}

evo::MinedQuorumCommitment Commitment(Consensus::LLMQType type, uint8_t quorum, uint8_t mined, bool rotated,
                                      int16_t index = 0)
{
    llmq::CFinalCommitment commitment;
    commitment.nVersion = rotated ? llmq::CFinalCommitment::BASIC_BLS_INDEXED_QUORUM_VERSION
                                  : llmq::CFinalCommitment::BASIC_BLS_NON_INDEXED_QUORUM_VERSION;
    commitment.llmqType = type;
    commitment.quorumHash = H(quorum);
    commitment.quorumIndex = index;
    const auto& params{evo::SnapshotLLMQParams(type)};
    commitment.signers.resize(params.size);
    commitment.validMembers.resize(params.size);
    return {H(quorum), H(quorum + 120), std::move(commitment), H(mined)};
}

evo::EvoSnapshot SyntheticSnapshot(bool reverse_representation = false)
{
    evo::EvoSnapshot snapshot;
    snapshot.base_block_hash = H(42);
    snapshot.mn_list = MNList(snapshot.base_block_hash, 500, reverse_representation);
    snapshot.credit_pool.locked = 123456;
    snapshot.credit_pool.currentLimit = 700;
    snapshot.credit_pool.latelyUnlocked = 11;
    if (reverse_representation) {
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(15));
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(8));
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(7));
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(9));
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Remove(9));
        snapshot.mnhf_signals.emplace(9, 30);
        snapshot.mnhf_signals.emplace(2, 12);
    } else {
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(7));
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(8));
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(15));
        snapshot.mnhf_signals.emplace(2, 12);
        snapshot.mnhf_signals.emplace(9, 30);
    }

    evo::QuorumSnapshotData plain;
    plain.llmq_type = Consensus::LLMQType::LLMQ_TEST;
    plain.active_commitments = {Commitment(plain.llmq_type, 11, 51, false), Commitment(plain.llmq_type, 12, 52, false)};
    plain.safety_commitments = {Commitment(plain.llmq_type, 10, 50, false)};

    evo::QuorumSnapshotData rotated;
    rotated.llmq_type = Consensus::LLMQType::LLMQ_TEST_DIP0024;
    rotated.rotation_enabled = true;
    rotated.active_commitments = {Commitment(rotated.llmq_type, 31, 71, true, 0),
                                  Commitment(rotated.llmq_type, 32, 72, true, 1)};
    rotated.safety_commitments = {Commitment(rotated.llmq_type, 21, 61, true, 0),
                                  Commitment(rotated.llmq_type, 22, 62, true, 1)};
    for (uint8_t i{1}; i <= evo::EVO_SNAPSHOT_ROTATION_CYCLES; ++i) {
        const auto mode{i == 2 ? SnapshotSkipMode::MODE_SKIPPING_ENTRIES : SnapshotSkipMode::MODE_NO_SKIPPING};
        rotated.rotation_snapshots.push_back(
            {H(40 + i), H(100 + i), llmq::CQuorumSnapshot{{true, false, true, false}, mode, i == 2 ? std::vector<int>{1} : std::vector<int>{}}});
    }

    snapshot.quorums = {std::move(plain), std::move(rotated)};
    std::set<uint256> work_hashes;
    std::set<std::pair<Consensus::LLMQType, uint256>> modifier_keys;
    for (const auto& data : snapshot.quorums) {
        for (const auto* commitments : {&data.active_commitments, &data.safety_commitments}) {
            for (const auto& entry : *commitments) {
                work_hashes.insert(entry.work_block_hash);
                modifier_keys.emplace(data.llmq_type, entry.work_block_hash);
            }
        }
        for (const auto& entry : data.rotation_snapshots) {
            work_hashes.insert(entry.work_block_hash);
            modifier_keys.emplace(data.llmq_type, entry.work_block_hash);
        }
    }
    CDeterministicMNList previous{snapshot.mn_list};
    uint256 previous_hash{snapshot.base_block_hash};
    int height{499};
    for (const auto& work_hash : work_hashes) {
        auto list{MNList(work_hash, height--, reverse_representation)};
        snapshot.historical_mn_list_diffs.push_back({previous_hash, work_hash, list.GetHeightForSnapshotCodec(),
                                                     list.GetTotalRegisteredCount(), evo::CanonicalMNListHash(list),
                                                     previous.BuildDiff(list)});
        previous_hash = work_hash;
        previous = std::move(list);
    }
    for (const auto& [type, work_hash] : modifier_keys) {
        snapshot.quorum_modifiers.push_back({type, work_hash, H(static_cast<uint8_t>(150 + snapshot.quorum_modifiers.size()))});
    }
    if (reverse_representation) {
        std::reverse(snapshot.quorums.begin(), snapshot.quorums.end());
        std::reverse(snapshot.historical_mn_list_diffs.begin(), snapshot.historical_mn_list_diffs.end());
        std::reverse(snapshot.quorum_modifiers.begin(), snapshot.quorum_modifiers.end());
        for (auto& data : snapshot.quorums) {
            std::reverse(data.active_commitments.begin(), data.active_commitments.end());
            std::reverse(data.safety_commitments.begin(), data.safety_commitments.end());
            std::reverse(data.rotation_snapshots.begin(), data.rotation_snapshots.end());
        }
    }
    return snapshot;
}

CDataStream SerializeSnapshot(const evo::EvoSnapshot& snapshot)
{
    CDataStream stream{SER_DISK, CLIENT_VERSION};
    stream << snapshot;
    return stream;
}

void CheckInvalid(evo::EvoSnapshot snapshot) { BOOST_CHECK_THROW(snapshot.Validate(), std::ios_base::failure); }

// The snapshot decoder inserts attacker-chosen update IDs straight into this
// map, so its hash must not be the identity an adversary can drive into one
// bucket.
static_assert(std::is_same_v<decltype(CDeterministicMNListDiff::updatedMNs)::hasher, StaticSaltedHasher>);

} // namespace

BOOST_AUTO_TEST_SUITE(evo_snapshot_tests)

BOOST_FIXTURE_TEST_CASE(populated_roundtrip_and_representation_independence, BasicTestingSetup)
{
    const auto forward{SyntheticSnapshot()};
    const auto reverse{SyntheticSnapshot(/*reverse_representation=*/true)};
    const auto forward_bytes{SerializeSnapshot(forward)};
    const auto reverse_bytes{SerializeSnapshot(reverse)};
    BOOST_CHECK_EQUAL_COLLECTIONS(forward_bytes.begin(), forward_bytes.end(), reverse_bytes.begin(), reverse_bytes.end());
    BOOST_CHECK(evo::CanonicalMNListHash(forward.mn_list) == evo::CanonicalMNListHash(reverse.mn_list));
    BOOST_CHECK(GetEvoSnapshotHash(forward) == GetEvoSnapshotHash(reverse));

    CDataStream input{forward_bytes};
    evo::EvoSnapshot decoded;
    input >> decoded;
    BOOST_CHECK(input.empty());
    const auto decoded_bytes{SerializeSnapshot(decoded)};
    BOOST_CHECK_EQUAL_COLLECTIONS(forward_bytes.begin(), forward_bytes.end(), decoded_bytes.begin(), decoded_bytes.end());
    BOOST_CHECK(evo::CanonicalMNListHash(decoded.mn_list) == evo::CanonicalMNListHash(forward.mn_list));
    BOOST_CHECK_EQUAL(decoded.mn_list.GetCounts().total(), 3U);
    BOOST_CHECK_EQUAL(decoded.historical_mn_list_diffs.size(), forward.historical_mn_list_diffs.size());
    BOOST_CHECK(decoded.credit_pool.indexes.Contains(7));
    BOOST_CHECK(decoded.credit_pool.indexes.Contains(8));
    BOOST_CHECK(decoded.credit_pool.indexes.Contains(15));
    BOOST_CHECK(decoded.mnhf_signals == forward.mnhf_signals);

    for (const auto internal_id : {2U, 5U, 7U}) {
        const auto original{forward.mn_list.GetMNByInternalId(internal_id)};
        BOOST_REQUIRE(original);
        const auto by_hash{decoded.mn_list.GetMN(original->proTxHash)};
        const auto by_id{decoded.mn_list.GetMNByInternalId(internal_id)};
        const auto by_collateral{decoded.mn_list.GetUniquePropertyMN(original->collateralOutpoint)};
        const auto by_owner{decoded.mn_list.GetUniquePropertyMN(original->pdmnState->keyIDOwner)};
        const auto by_service{decoded.mn_list.GetMNByService(original->pdmnState->netInfo->GetPrimary())};
        BOOST_REQUIRE(by_hash);
        BOOST_REQUIRE(by_id);
        BOOST_REQUIRE(by_collateral);
        BOOST_REQUIRE(by_owner);
        BOOST_REQUIRE(by_service);
        BOOST_CHECK(by_hash->proTxHash == original->proTxHash);
        BOOST_CHECK(by_id->proTxHash == original->proTxHash);
        BOOST_CHECK(by_collateral->proTxHash == original->proTxHash);
        BOOST_CHECK(by_owner->proTxHash == original->proTxHash);
        BOOST_CHECK(by_service->proTxHash == original->proTxHash);
    }
}

BOOST_AUTO_TEST_CASE(reconstruction_horizon_height_enumeration)
{
    const auto rotated{evo::SnapshotLLMQParams(Consensus::LLMQType::LLMQ_TEST_DIP0024)};
    const auto plain{evo::SnapshotLLMQParams(Consensus::LLMQType::LLMQ_TEST)};
    const int base_height{20 * rotated.dkgInterval + 7};
    const auto heights{evo::EvoSnapshotReconstructionHeights(base_height, {rotated, plain})};
    BOOST_REQUIRE_EQUAL(heights.size(), evo::EVO_SNAPSHOT_ROTATION_CYCLES +
                                             evo::SnapshotCommitmentCount(plain, false));
    const int rotated_h{base_height - base_height % rotated.dkgInterval};
    for (size_t i{0}; i < evo::EVO_SNAPSHOT_ROTATION_CYCLES; ++i) {
        const int expected_cycle{rotated_h - static_cast<int>(i + 1) * rotated.dkgInterval};
        BOOST_CHECK(heights[i].rotation);
        BOOST_CHECK_EQUAL(heights[i].quorum_height, expected_cycle);
        BOOST_CHECK_EQUAL(heights[i].work_height, expected_cycle - llmq::WORK_DIFF_DEPTH);
    }
    const int plain_h{base_height - base_height % plain.dkgInterval};
    for (size_t i{0}; i < evo::SnapshotCommitmentCount(plain, false); ++i) {
        const auto& height{heights[evo::EVO_SNAPSHOT_ROTATION_CYCLES + i]};
        BOOST_CHECK(!height.rotation);
        BOOST_CHECK_EQUAL(height.quorum_height, plain_h - static_cast<int>(i) * plain.dkgInterval);
        BOOST_CHECK_EQUAL(height.work_height, height.quorum_height - llmq::WORK_DIFF_DEPTH);
    }
}

BOOST_AUTO_TEST_CASE(rotation_bitset_larger_than_quorum_roundtrips)
{
    const auto& params{evo::SnapshotLLMQParams(Consensus::LLMQType::LLMQ_TEST_DIP0024)};
    evo::QuorumSnapshotEntry entry;
    entry.cycle_base_block_hash = H(1);
    entry.work_block_hash = H(2);
    entry.snapshot.activeQuorumMembers.resize(params.size + 3);
    entry.snapshot.activeQuorumMembers[params.size + 1] = true;
    entry.snapshot.mnSkipListMode = SnapshotSkipMode::MODE_NO_SKIPPING;

    CDataStream stream{SER_DISK, CLIENT_VERSION};
    evo::WriteRotationSnapshot(stream, entry);
    const auto decoded{evo::ReadRotationSnapshot(stream, params)};
    BOOST_CHECK(stream.empty());
    BOOST_CHECK_EQUAL(decoded.snapshot.activeQuorumMembers.size(), params.size + 3U);
    BOOST_CHECK(decoded.snapshot.activeQuorumMembers[params.size + 1]);
}

BOOST_FIXTURE_TEST_CASE(populated_v3_golden_value, BasicTestingSetup)
{
    BOOST_CHECK_EQUAL(GetEvoSnapshotHash(SyntheticSnapshot()).ToString(),
                      "8a2883207069e92589ffcce1f9867736881ecdf04ccfc89b492570740e7d3f31");
}

BOOST_FIXTURE_TEST_CASE(canonical_mn_reader_rejects_order_and_counter, BasicTestingSetup)
{
    BOOST_CHECK(evo::CanonicalMNListHash(CDeterministicMNList{}) ==
                evo::CanonicalMNListHash(CDeterministicMNList{}));
    const auto write_raw = [](uint32_t total, std::vector<CDeterministicMNCPtr> mns) {
        CDataStream stream{SER_DISK, CLIENT_VERSION};
        stream << H(42) << 42 << total;
        WriteCompactSize(stream, mns.size());
        for (const auto& dmn : mns) stream << *dmn;
        return stream;
    };
    auto unsorted{write_raw(10, {MN(2, 2, MnType::Regular, ProTxVersion::LegacyBLS, 2),
                                 MN(1, 1, MnType::Regular, ProTxVersion::LegacyBLS, 1)})};
    BOOST_CHECK_THROW(evo::UnserializeCanonicalMNList(unsorted), std::ios_base::failure);
    auto bad_counter{write_raw(2, {MN(2, 1, MnType::Regular, ProTxVersion::LegacyBLS, 1)})};
    BOOST_CHECK_THROW(evo::UnserializeCanonicalMNList(bad_counter), std::ios_base::failure);
}

BOOST_FIXTURE_TEST_CASE(diff_chain_roundtrip_and_canonical_determinism, BasicTestingSetup)
{
    const auto base{MNList(H(10), 100, false)};
    auto target{base};
    target.RemoveMN(base.GetMNByInternalId(2)->proTxHash);
    target.AddMN(MN(8, 8, MnType::Regular, ProTxVersion::LegacyBLS, 8));
    for (const uint64_t id : {5, 7}) {
        const auto dmn{target.GetMNByInternalId(id)};
        auto state{std::make_shared<CDeterministicMNState>(*dmn->pdmnState)};
        state->nLastPaidHeight += static_cast<int>(id);
        target.UpdateMN(*dmn, state);
    }
    const auto diff{base.BuildDiff(target)};
    auto permuted{diff};
    std::reverse(permuted.addedMNs.begin(), permuted.addedMNs.end());
    std::vector<std::pair<uint64_t, CDeterministicMNStateDiff>> updates(permuted.updatedMNs.begin(),
                                                                       permuted.updatedMNs.end());
    std::reverse(updates.begin(), updates.end());
    permuted.updatedMNs.clear();
    for (auto& update : updates) permuted.updatedMNs.emplace(std::move(update));

    CDataStream canonical{SER_DISK, CLIENT_VERSION};
    CDataStream reordered{SER_DISK, CLIENT_VERSION};
    evo::SerializeCanonicalMNListDiff(canonical, diff);
    evo::SerializeCanonicalMNListDiff(reordered, permuted);
    BOOST_CHECK_EQUAL_COLLECTIONS(canonical.begin(), canonical.end(), reordered.begin(), reordered.end());

    auto decoded{evo::UnserializeCanonicalMNListDiff(canonical)};
    auto reconstructed{base};
    reconstructed.ApplyDiffForSnapshot(H(11), 99, target.GetTotalRegisteredCount(), decoded);
    target.ApplyDiffForSnapshot(H(11), 99, target.GetTotalRegisteredCount(), CDeterministicMNListDiff{});
    BOOST_CHECK(evo::CanonicalMNListHash(reconstructed) == evo::CanonicalMNListHash(target));
    BOOST_CHECK(canonical.empty());
}

BOOST_FIXTURE_TEST_CASE(inactive_mn_roundtrip, BasicTestingSetup)
{
    for (const int version : {ProTxVersion::LegacyBLS, ProTxVersion::ExtAddr}) {
        auto inactive{std::make_shared<CDeterministicMN>(*MN(2, 3, MnType::Regular, version, 3))};
        auto state{std::make_shared<CDeterministicMNState>(*inactive->pdmnState)};
        state->netInfo = NetInfoInterface::MakeNetInfo(version);
        state->BanIfNotBanned(100);
        inactive->pdmnState = state;
        BOOST_REQUIRE(state->netInfo->IsEmpty());

        evo::EvoSnapshot snapshot;
        snapshot.base_block_hash = H(42);
        snapshot.mn_list = CDeterministicMNList{snapshot.base_block_hash, 500, 10};
        snapshot.mn_list.AddMN(inactive, /*fBumpTotalCount=*/false);
        BOOST_CHECK_NO_THROW(snapshot.Validate());
        auto bytes{SerializeSnapshot(snapshot)};
        evo::EvoSnapshot decoded;
        BOOST_REQUIRE_NO_THROW(bytes >> decoded);
        BOOST_CHECK(bytes.empty());
        BOOST_CHECK(GetEvoSnapshotHash(snapshot) == GetEvoSnapshotHash(decoded));

        // Cover both historical additions and updates to empty network info.
        for (const bool addition : {false, true}) {
            CDeterministicMNList base{H(41), 501, 10};
            if (!addition) base.AddMN(MN(2, 3, MnType::Regular, version, 3), /*fBumpTotalCount=*/false);
            const auto diff{base.BuildDiff(snapshot.mn_list)};
            CDataStream encoded{SER_DISK, CLIENT_VERSION};
            evo::SerializeCanonicalMNListDiff(encoded, diff);
            const auto decoded_diff{evo::UnserializeCanonicalMNListDiff(encoded)};
            evo::EvoSnapshot historical;
            historical.base_block_hash = base.GetBlockHash();
            historical.mn_list = base;
            historical.historical_mn_list_diffs = {{base.GetBlockHash(), snapshot.base_block_hash, 500, 10,
                                                    evo::CanonicalMNListHash(snapshot.mn_list), decoded_diff}};
            std::map<uint256, CDeterministicMNList> lists;
            std::string error;
            BOOST_REQUIRE_MESSAGE(evo::ReconstructHistoricalMNLists(historical, lists, error), error);
            BOOST_REQUIRE_EQUAL(lists.size(), 1U);
            BOOST_CHECK(evo::CanonicalMNListHash(lists.begin()->second) == evo::CanonicalMNListHash(snapshot.mn_list));
        }
    }
}

BOOST_FIXTURE_TEST_CASE(historical_diff_applies_exchanged_unique_properties, BasicTestingSetup)
{
    // Between two work blocks a surviving MN can take an address another
    // survivor still holds at the first endpoint, and a new registration can
    // take an address an updated MN released. Both endpoint lists are valid,
    // so the diff between them must apply regardless of update order.
    const auto older{MNList(H(10), 100, false)};
    const auto with_net_info = [](const CDeterministicMN& dmn, std::shared_ptr<NetInfoInterface> net_info) {
        auto state{std::make_shared<CDeterministicMNState>(*dmn.pdmnState)};
        state->netInfo = std::move(net_info);
        auto copy{std::make_shared<CDeterministicMN>(dmn)};
        copy->pdmnState = std::move(state);
        return copy;
    };
    const auto a{older.GetMNByInternalId(2)};
    const auto b{older.GetMNByInternalId(7)};
    auto fresh{NetInfoInterface::MakeNetInfo(b->pdmnState->nVersion)};
    BOOST_REQUIRE_EQUAL(fresh->AddEntry(NetInfoPurpose::CORE_P2P, strprintf("1.1.1.9:%d", Params().GetDefaultPort())),
                        NetInfoStatus::Success);
    CDeterministicMNList newer{H(11), 101, 10};
    newer.AddMN(with_net_info(*a, b->pdmnState->netInfo), /*fBumpTotalCount=*/false);
    newer.AddMN(with_net_info(*b, fresh), /*fBumpTotalCount=*/false);
    newer.AddMN(older.GetMNByInternalId(5), /*fBumpTotalCount=*/false);
    newer.AddMN(with_net_info(*MN(8, 8, MnType::Regular, ProTxVersion::LegacyBLS, 8), a->pdmnState->netInfo),
                /*fBumpTotalCount=*/false);

    const CDeterministicMNList* endpoints[]{&older, &newer};
    for (const auto* from : endpoints) {
        const auto& to{from == &older ? newer : older};
        CDataStream encoded{SER_DISK, CLIENT_VERSION};
        evo::SerializeCanonicalMNListDiff(encoded, from->BuildDiff(to));
        const auto diff{evo::UnserializeCanonicalMNListDiff(encoded)};
        BOOST_REQUIRE_EQUAL(diff.updatedMNs.size(), 2U);
        BOOST_REQUIRE_EQUAL(diff.addedMNs.size() + diff.removedMns.size(), 1U);
        auto applied{*from};
        applied.ApplyDiffForSnapshot(to.GetBlockHash(), to.GetHeightForSnapshotCodec(), to.GetTotalRegisteredCount(), diff);
        BOOST_CHECK(evo::CanonicalMNListHash(applied) == evo::CanonicalMNListHash(to));

        // A diff whose result would hold one address twice is still rejected.
        CDeterministicMNListDiff conflicting;
        conflicting.updatedMNs.emplace(2, diff.updatedMNs.at(2));
        auto rejected{*from};
        BOOST_CHECK_THROW(rejected.ApplyDiffForSnapshot(to.GetBlockHash(), to.GetHeightForSnapshotCodec(),
                                                        to.GetTotalRegisteredCount(), conflicting),
                          std::runtime_error);
    }

    // The full reconstruction path, which re-checks every encoding around the
    // apply, accepts the exchange as well.
    evo::EvoSnapshot snapshot;
    snapshot.base_block_hash = newer.GetBlockHash();
    snapshot.mn_list = newer;
    snapshot.historical_mn_list_diffs = {{newer.GetBlockHash(), older.GetBlockHash(), older.GetHeightForSnapshotCodec(),
                                          older.GetTotalRegisteredCount(), evo::CanonicalMNListHash(older),
                                          newer.BuildDiff(older)}};
    std::map<uint256, CDeterministicMNList> lists;
    std::string error;
    BOOST_REQUIRE_MESSAGE(evo::ReconstructHistoricalMNLists(snapshot, lists, error), error);
    BOOST_REQUIRE_EQUAL(lists.size(), 1U);
    BOOST_CHECK(evo::CanonicalMNListHash(lists.begin()->second) == evo::CanonicalMNListHash(older));
}

BOOST_FIXTURE_TEST_CASE(historical_diff_additions_require_new_identities, BasicTestingSetup)
{
    const auto original{MNList(H(11), 101, false)};
    const auto original_hash{evo::CanonicalMNListHash(original)};
    const auto existing{original.GetMNByInternalId(2)};
    auto same_hash{std::make_shared<CDeterministicMN>(*MN(8, 8, MnType::Regular, ProTxVersion::LegacyBLS, 8))};
    same_hash->proTxHash = existing->proTxHash;
    const std::vector<CDeterministicMNCPtr> additions{
        existing,
        MN(2, 8, MnType::Regular, ProTxVersion::LegacyBLS, 8),
        same_hash,
    };
    for (const bool remove_existing : {false, true}) {
        for (const auto& addition : additions) {
            auto list{original};
            CDeterministicMNListDiff diff;
            if (remove_existing) diff.removedMns.emplace(existing->GetInternalId());
            diff.addedMNs.push_back(addition);
            BOOST_CHECK_EXCEPTION(list.ApplyDiffForSnapshot(H(10), 100, original.GetTotalRegisteredCount(), diff),
                                  std::runtime_error, [](const auto& e) {
                                      return std::string{e.what()} ==
                                             "historical MN-diff addition reuses an existing identity";
                                  });
            BOOST_CHECK(evo::CanonicalMNListHash(list) == original_hash);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(malformed_lazy_operator_keys_are_rejected, BasicTestingSetup)
{
    const auto noncanonical = [](const auto& e) {
        return std::string{e.what()}.find("noncanonical MN object encoding") != std::string::npos;
    };
    const std::vector<std::byte> undecodable(CBLSPublicKey::SerSize, std::byte{0xff});
    for (const int version : {ProTxVersion::LegacyBLS, ProTxVersion::BasicBLS}) {
        const bool legacy{version == ProTxVersion::LegacyBLS};
        auto mn{std::make_shared<CDeterministicMN>(*MN(2, 3, MnType::Regular, version, 3))};
        auto state{std::make_shared<CDeterministicMNState>(*mn->pdmnState)};
        CDataStream key_bytes{SER_DISK, CLIENT_VERSION};
        key_bytes.write(undecodable);
        key_bytes >> CBLSLazyPublicKeyVersionWrapper(state->pubKeyOperator, legacy);
        mn->pdmnState = state;

        // The lazy wrapper re-emits undecodable bytes verbatim until the key is
        // first read, after which it emits the empty key: two encodings of one
        // object, so the decoder must reject the bytes outright.
        CDataStream lazy{SER_DISK, CLIENT_VERSION};
        lazy << *mn;
        BOOST_CHECK(!mn->pdmnState->pubKeyOperator.Get().IsValid());
        CDataStream materialized{SER_DISK, CLIENT_VERSION};
        materialized << *mn;
        BOOST_REQUIRE(lazy.str() != materialized.str());

        CDataStream list{SER_DISK, CLIENT_VERSION};
        list << H(42) << 500 << uint32_t{10};
        WriteCompactSize(list, 1);
        list.write(Span{lazy});
        BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNList(list), std::ios_base::failure, noncanonical);

        CDataStream addition{SER_DISK, CLIENT_VERSION};
        WriteCompactSize(addition, 1);
        addition.write(Span{lazy});
        WriteCompactSize(addition, 0);
        WriteCompactSize(addition, 0);
        BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNListDiff(addition), std::ios_base::failure, noncanonical);

        CDeterministicMNStateDiff state_diff;
        state_diff.fields = CDeterministicMNStateDiff::Field_nVersion | CDeterministicMNStateDiff::Field_pubKeyOperator;
        state_diff.state.nVersion = version;
        key_bytes.write(undecodable);
        key_bytes >> CBLSLazyPublicKeyVersionWrapper(state_diff.state.pubKeyOperator, legacy);
        CDataStream update{SER_DISK, CLIENT_VERSION};
        WriteCompactSize(update, 0);
        WriteCompactSize(update, 1);
        WriteVarInt<CDataStream, VarIntMode::DEFAULT, uint64_t>(update, 2);
        update << state_diff;
        WriteCompactSize(update, 0);
        BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNListDiff(update), std::ios_base::failure, noncanonical);

        // An in-memory snapshot holding the unread bytes must not hash either.
        lazy.clear();
        lazy << *MN(2, 3, MnType::Regular, version, 3);
        auto unread{std::make_shared<CDeterministicMN>(deserialize, lazy)};
        BOOST_REQUIRE(lazy.empty());
        auto unread_state{std::make_shared<CDeterministicMNState>(*unread->pdmnState)};
        key_bytes.write(undecodable);
        key_bytes >> CBLSLazyPublicKeyVersionWrapper(unread_state->pubKeyOperator, legacy);
        unread->pdmnState = unread_state;
        evo::EvoSnapshot snapshot;
        snapshot.base_block_hash = H(42);
        snapshot.mn_list = CDeterministicMNList{snapshot.base_block_hash, 500, 10};
        snapshot.mn_list.AddMN(unread, /*fBumpTotalCount=*/false);
        BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, noncanonical);
        BOOST_CHECK_THROW(GetEvoSnapshotHash(snapshot), std::ios_base::failure);

        // A decodable key keeps one encoding whether or not it has been read.
        CBLSSecretKey secret;
        BOOST_REQUIRE(secret.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", legacy));
        auto keyed_state{std::make_shared<CDeterministicMNState>(*unread_state)};
        keyed_state->pubKeyOperator.Set(secret.GetPublicKey(), legacy);
        unread->pdmnState = keyed_state;
        BOOST_CHECK_NO_THROW(snapshot.Validate());
        const auto before_read{GetEvoSnapshotHash(snapshot)};
        auto bytes{SerializeSnapshot(snapshot)};
        evo::EvoSnapshot decoded;
        BOOST_REQUIRE_NO_THROW(bytes >> decoded);
        BOOST_CHECK(decoded.mn_list.GetMNByInternalId(2)->pdmnState->pubKeyOperator.Get().IsValid());
        BOOST_CHECK(GetEvoSnapshotHash(decoded) == before_read);
    }
}

BOOST_FIXTURE_TEST_CASE(unset_mn_id_is_rejected, BasicTestingSetup)
{
    const auto mn{MN(2, 3, MnType::Regular, ProTxVersion::LegacyBLS, 3)};
    CDataStream malformed{SER_DISK, CLIENT_VERSION};
    malformed << mn->proTxHash;
    WriteVarInt<CDataStream, VarIntMode::DEFAULT, uint64_t>(malformed, std::numeric_limits<uint64_t>::max());
    malformed << mn->collateralOutpoint << mn->nOperatorReward << mn->pdmnState << mn->nType;
    const auto invalid_id = [](const auto& e) {
        return std::string{e.what()}.find("invalid canonical MN internalId") != std::string::npos;
    };
    CDataStream base{SER_DISK, CLIENT_VERSION};
    base << H(42) << 500 << uint32_t{10};
    WriteCompactSize(base, 1);
    base.write(Span{malformed});
    BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNList(base), std::ios_base::failure, invalid_id);

    CDataStream addition{SER_DISK, CLIENT_VERSION};
    WriteCompactSize(addition, 1);
    addition.write(Span{malformed});
    WriteCompactSize(addition, 0);
    WriteCompactSize(addition, 0);
    BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNListDiff(addition), std::ios_base::failure, invalid_id);
}

BOOST_FIXTURE_TEST_CASE(commitment_bls_wire_encoding_is_canonical, BasicTestingSetup)
{
    CBLSSecretKey key;
    BOOST_REQUIRE(key.SetHexStr("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f", false));
    for (const bool legacy : {false, true}) {
        for (const bool indexed : {false, true}) {
            auto entry{Commitment(Consensus::LLMQType::LLMQ_TEST, 11, 51, indexed)};
            if (legacy) {
                entry.commitment.nVersion = indexed ? llmq::CFinalCommitment::LEGACY_BLS_INDEXED_QUORUM_VERSION
                                                    : llmq::CFinalCommitment::LEGACY_BLS_NON_INDEXED_QUORUM_VERSION;
            }
            entry.commitment.quorumPublicKey = key.GetPublicKey();
            entry.commitment.quorumSig = key.Sign(H(1), legacy);
            entry.commitment.membersSig = key.Sign(H(2), legacy);
            const auto encode = [&](int opposite_field) {
                auto& qc{entry.commitment};
                CDataStream bytes{SER_DISK, CLIENT_VERSION};
                bytes << entry.quorum_base_block_hash << entry.work_block_hash << qc.nVersion << qc.llmqType
                      << qc.quorumHash;
                if (indexed) bytes << qc.quorumIndex;
                bytes << DYNBITSET(qc.signers) << DYNBITSET(qc.validMembers)
                      << CBLSPublicKeyVersionWrapper(qc.quorumPublicKey, opposite_field == 0 ? !legacy : legacy)
                      << qc.quorumVvecHash
                      << CBLSSignatureVersionWrapper(qc.quorumSig, opposite_field == 1 ? !legacy : legacy)
                      << CBLSSignatureVersionWrapper(qc.membersSig, opposite_field == 2 ? !legacy : legacy)
                      << entry.mined_block_hash;
                return bytes;
            };
            auto canonical{encode(-1)};
            BOOST_CHECK_NO_THROW(evo::ReadMinedQuorumCommitment(canonical));
            BOOST_CHECK(canonical.empty());
            if (legacy) continue;
            for (int field{0}; field < 3; ++field) {
                auto bytes{encode(field)};
                auto ordinary_input{bytes};
                evo::MinedQuorumCommitment ordinary;
                BOOST_REQUIRE_NO_THROW(ordinary_input >> ordinary);
                CDataStream normalized{SER_DISK, CLIENT_VERSION};
                normalized << ordinary;
                BOOST_REQUIRE(bytes.str() != normalized.str());
                BOOST_CHECK_EXCEPTION(evo::ReadMinedQuorumCommitment(bytes), std::ios_base::failure, [](const auto& e) {
                    return std::string{e.what()}.find("noncanonical evo snapshot commitment encoding") != std::string::npos;
                });
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(historical_diff_decode_has_cumulative_operation_budget, BasicTestingSetup)
{
    CDeterministicMNListDiff one_removal;
    one_removal.removedMns.emplace(1);
    CDataStream first{SER_DISK, CLIENT_VERSION};
    CDataStream second{SER_DISK, CLIENT_VERSION};
    evo::SerializeCanonicalMNListDiff(first, one_removal);
    evo::SerializeCanonicalMNListDiff(second, one_removal);

    size_t remaining_operations{1};
    const auto decoded{evo::UnserializeCanonicalMNListDiff(first, remaining_operations)};
    BOOST_CHECK_EQUAL(decoded.removedMns.size(), 1U);
    BOOST_CHECK_EQUAL(remaining_operations, 0U);
    BOOST_CHECK_THROW(evo::UnserializeCanonicalMNListDiff(second, remaining_operations),
                      std::ios_base::failure);
    BOOST_CHECK(evo::EvoSnapshotMaxHistoricalMNLists() < 2'048U);
}

BOOST_FIXTURE_TEST_CASE(context_free_validation_matrix, BasicTestingSetup)
{
    auto snapshot{SyntheticSnapshot()};
    snapshot.version++;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.base_block_hash = H(1);
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    std::reverse(snapshot.quorums.begin(), snapshot.quorums.end());
    BOOST_CHECK_THROW(snapshot.Validate(/*require_canonical_order=*/true), std::ios_base::failure);
    snapshot = SyntheticSnapshot();
    std::reverse(snapshot.historical_mn_list_diffs.begin(), snapshot.historical_mn_list_diffs.end());
    BOOST_CHECK_THROW(snapshot.Validate(/*require_canonical_order=*/true), std::ios_base::failure);
    snapshot = SyntheticSnapshot();
    std::reverse(snapshot.quorums[0].active_commitments.begin(), snapshot.quorums[0].active_commitments.end());
    BOOST_CHECK_THROW(snapshot.Validate(/*require_canonical_order=*/true), std::ios_base::failure);
    snapshot = SyntheticSnapshot();
    std::reverse(snapshot.quorums[1].rotation_snapshots.begin(), snapshot.quorums[1].rotation_snapshots.end());
    BOOST_CHECK_THROW(snapshot.Validate(/*require_canonical_order=*/true), std::ios_base::failure);
    snapshot = SyntheticSnapshot();
    snapshot.historical_mn_list_diffs[0].block_hash = H(1);
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[1].rotation_snapshots[0].work_block_hash = H(1);
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[0].llmq_type = Consensus::LLMQType::LLMQ_NONE;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[0].rotation_enabled = true;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[0].active_commitments.pop_back();
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[0].safety_commitments.clear();
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[1].rotation_snapshots.pop_back();
    CheckInvalid(snapshot);

    // Parameter-derived history counts are ceilings. Young chains and newly
    // activated quorum types legitimately carry fewer commitments and cycles;
    // chain-aware validation and the base CbTx establish completeness later.
    snapshot = SyntheticSnapshot();
    snapshot.quorums.clear();
    snapshot.historical_mn_list_diffs.clear();
    snapshot.quorum_modifiers.clear();
    evo::QuorumSnapshotData partial;
    partial.llmq_type = Consensus::LLMQType::LLMQ_TEST;
    snapshot.quorums.emplace_back(std::move(partial));
    BOOST_CHECK_NO_THROW(snapshot.Validate());

    snapshot = SyntheticSnapshot();
    snapshot.credit_pool.locked = -1;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.credit_pool.currentLimit = snapshot.credit_pool.locked + 1;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.credit_pool.latelyUnlocked = MAX_MONEY + 1;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.mnhf_signals.emplace(VERSIONBITS_NUM_BITS, 10);
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.mnhf_signals.emplace(11, -1);
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.mnhf_signals.emplace(11, snapshot.mn_list.GetHeightForSnapshotCodec() + 1);
    CheckInvalid(snapshot);

    const auto mutate_commitment = [](auto mutation) {
        auto value{SyntheticSnapshot()};
        mutation(value.quorums[0].active_commitments[0]);
        CheckInvalid(std::move(value));
    };
    mutate_commitment([](auto& e) { e.commitment.validMembers.resize(e.commitment.signers.size() + 1); });
    mutate_commitment([](auto& e) {
        e.commitment.signers.clear();
        e.commitment.validMembers.clear();
    });
    mutate_commitment([](auto& e) { e.quorum_base_block_hash.SetNull(); });
    mutate_commitment([](auto& e) { e.mined_block_hash.SetNull(); });
    mutate_commitment([](auto& e) { e.commitment.llmqType = Consensus::LLMQType::LLMQ_TEST_PLATFORM; });
    mutate_commitment([](auto& e) { e.commitment.quorumHash = H(99); });
    mutate_commitment([](auto& e) { e.commitment.nVersion = llmq::CFinalCommitment::BASIC_BLS_INDEXED_QUORUM_VERSION; });
    mutate_commitment([](auto& e) { e.commitment.nVersion = 99; });
    snapshot = SyntheticSnapshot();
    snapshot.quorums[1].active_commitments[1].commitment.quorumIndex = 0;
    CheckInvalid(snapshot);
    snapshot = SyntheticSnapshot();
    snapshot.quorums[1].active_commitments[1].commitment.quorumIndex = 2;
    CheckInvalid(snapshot);

    const auto mutate_rotation = [](auto mutation) {
        auto value{SyntheticSnapshot()};
        mutation(value.quorums[1].rotation_snapshots[0]);
        CheckInvalid(std::move(value));
    };
    mutate_rotation([](auto& e) { e.cycle_base_block_hash.SetNull(); });
    mutate_rotation([](auto& e) { e.work_block_hash.SetNull(); });
    mutate_rotation([](auto& e) { e.snapshot.mnSkipListMode = static_cast<SnapshotSkipMode>(9); });
    mutate_rotation([](auto& e) { e.snapshot.activeQuorumMembers.resize(evo::EVO_SNAPSHOT_MAX_MNS + 1); });
    mutate_rotation([](auto& e) { e.snapshot.mnSkipList = {-1}; });

    // A cycle's skip list accumulates across every quorum index, so lengths
    // beyond a single quorum's size and negative wraparound deltas after the
    // first (absolute) entry are legitimate.
    auto aggregate_skips{SyntheticSnapshot()};
    auto& rotation_entry{aggregate_skips.quorums[1].rotation_snapshots[0]};
    const auto& rotation_params{evo::SnapshotLLMQParams(aggregate_skips.quorums[1].llmq_type)};
    rotation_entry.snapshot.mnSkipListMode = SnapshotSkipMode::MODE_SKIPPING_ENTRIES;
    rotation_entry.snapshot.mnSkipList.assign(static_cast<size_t>(rotation_params.size) + 2, 1);
    rotation_entry.snapshot.mnSkipList.front() = 3;
    rotation_entry.snapshot.mnSkipList.back() = -2;
    BOOST_CHECK_NO_THROW(aggregate_skips.Validate());
}

BOOST_FIXTURE_TEST_CASE(bounded_readers_reject_claimed_sizes_first, BasicTestingSetup)
{
    CDataStream mn_stream{SER_DISK, CLIENT_VERSION};
    mn_stream << H(1) << 1 << uint32_t{0};
    WriteCompactSize(mn_stream, evo::EVO_SNAPSHOT_MAX_MNS + 1);
    BOOST_CHECK_THROW(evo::UnserializeCanonicalMNList(mn_stream), std::ios_base::failure);
    BOOST_CHECK(mn_stream.empty());

    const auto decode_quorum = [](CDataStream stream) {
        evo::QuorumSnapshotData data;
        stream >> data;
    };
    const auto& plain_params{evo::SnapshotLLMQParams(Consensus::LLMQType::LLMQ_TEST)};
    const size_t commitment_limit{evo::SnapshotCommitmentCount(plain_params, false)};
    CDataStream active{SER_DISK, CLIENT_VERSION};
    active << Consensus::LLMQType::LLMQ_TEST << false;
    WriteCompactSize(active, plain_params.signingActiveQuorumCount + 1);
    BOOST_CHECK_THROW(decode_quorum(active), std::ios_base::failure);
    CDataStream safety{SER_DISK, CLIENT_VERSION};
    safety << Consensus::LLMQType::LLMQ_TEST << false;
    WriteCompactSize(safety, 0);
    WriteCompactSize(safety, commitment_limit - plain_params.signingActiveQuorumCount + 1);
    BOOST_CHECK_THROW(decode_quorum(safety), std::ios_base::failure);
    CDataStream rotations{SER_DISK, CLIENT_VERSION};
    rotations << Consensus::LLMQType::LLMQ_TEST_DIP0024 << true;
    WriteCompactSize(rotations, 0);
    WriteCompactSize(rotations, 0);
    WriteCompactSize(rotations, evo::EVO_SNAPSHOT_ROTATION_CYCLES + 1);
    BOOST_CHECK_THROW(decode_quorum(rotations), std::ios_base::failure);

    const auto& rotated_params{evo::SnapshotLLMQParams(Consensus::LLMQType::LLMQ_TEST_DIP0024)};
    CDataStream bitset{SER_DISK, CLIENT_VERSION};
    bitset << H(1) << H(2) << SnapshotSkipMode::MODE_NO_SKIPPING;
    WriteCompactSize(bitset, evo::EVO_SNAPSHOT_MAX_MNS + 1);
    BOOST_CHECK_THROW(evo::ReadRotationSnapshot(bitset, rotated_params), std::ios_base::failure);
    CDataStream skip_list{SER_DISK, CLIENT_VERSION};
    skip_list << H(1) << H(2) << SnapshotSkipMode::MODE_NO_SKIPPING;
    WriteCompactSize(skip_list, 0);
    WriteCompactSize(skip_list, rotated_params.size + 1);
    BOOST_CHECK_THROW(evo::ReadRotationSnapshot(skip_list, rotated_params), std::ios_base::failure);

    CDataStream commitment_bits{SER_DISK, CLIENT_VERSION};
    auto oversized_commitment{Commitment(Consensus::LLMQType::LLMQ_TEST, 1, 2, false)};
    oversized_commitment.commitment.signers.resize(plain_params.size + 1);
    commitment_bits << Consensus::LLMQType::LLMQ_TEST << false;
    WriteCompactSize(commitment_bits, 1);
    commitment_bits << oversized_commitment;
    evo::QuorumSnapshotData oversized_data;
    BOOST_CHECK_EXCEPTION(commitment_bits >> oversized_data, std::ios_base::failure, [](const auto& e) {
        return std::string{e.what()}.find("inconsistent evo snapshot commitment bitset sizes") != std::string::npos;
    });
    // The valid-members bitset payload, BLS material, and mined-block hash
    // remain unread: rejection occurs at the mismatched claimed size.
    BOOST_CHECK_GT(commitment_bits.size(), uint256::size());

    const auto commitment_prefix = [](CDataStream& s) {
        s << Consensus::LLMQType::LLMQ_TEST << false;
        WriteCompactSize(s, 1);
        s << H(1) << H(2) << uint16_t{llmq::CFinalCommitment::BASIC_BLS_NON_INDEXED_QUORUM_VERSION}
          << Consensus::LLMQType::LLMQ_TEST << H(1);
    };
    CDataStream over_ceiling{SER_DISK, CLIENT_VERSION};
    commitment_prefix(over_ceiling);
    WriteCompactSize(over_ceiling, evo::EVO_SNAPSHOT_MAX_QUORUM_SIZE + 1);
    BOOST_CHECK_THROW(decode_quorum(over_ceiling), std::ios_base::failure);
    CDataStream empty_bits{SER_DISK, CLIENT_VERSION};
    commitment_prefix(empty_bits);
    WriteCompactSize(empty_bits, 0);
    BOOST_CHECK_THROW(decode_quorum(empty_bits), std::ios_base::failure);

    auto oversized_payout_mn{std::const_pointer_cast<CDeterministicMN>(
        MN(8, 8, MnType::Regular, ProTxVersion::ExtAddr, 8))};
    auto payout_state{std::make_shared<CDeterministicMNState>(*oversized_payout_mn->pdmnState)};
    payout_state->payouts.resize(evo::EVO_SNAPSHOT_MAX_PAYOUT_SHARES + 1);
    oversized_payout_mn->pdmnState = std::move(payout_state);
    CDataStream payouts{SER_DISK, CLIENT_VERSION};
    payouts << H(42) << 42 << uint32_t{10};
    WriteCompactSize(payouts, 1);
    payouts << *oversized_payout_mn;
    BOOST_CHECK_THROW(evo::UnserializeCanonicalMNList(payouts), std::ios_base::failure);

    CDataStream wrapped_string{SER_DISK, CLIENT_VERSION};
    WriteCompactSize(wrapped_string, evo::EVO_SNAPSHOT_MAX_MN_COMPACT_ITEMS + 1);
    wrapped_string << uint8_t{0x42};
    evo::SnapshotBoundedInput bounded_string{wrapped_string, evo::EVO_SNAPSHOT_MAX_MN_COMPACT_ITEMS};
    OverrideStream bounded_override{&bounded_string, SER_DISK, CLIENT_VERSION};
    std::string decoded_string;
    BOOST_CHECK_EXCEPTION(bounded_override >> decoded_string, std::ios_base::failure,
                          [](const auto& e) { return std::string{e.what()}.find("CompactSize budget exceeded") != std::string::npos; });
    BOOST_CHECK(decoded_string.empty());
    BOOST_REQUIRE_EQUAL(wrapped_string.size(), 1U);
    BOOST_CHECK_EQUAL(std::to_integer<uint8_t>(wrapped_string.data()[0]), 0x42);

    const auto snapshot_prefix = [](CDataStream& stream) {
        stream << evo::EVO_SNAPSHOT_VERSION << H(42);
        evo::SerializeCanonicalMNList(stream, CDeterministicMNList{H(42), 1, 0});
    };
    CDataStream quorum_types{SER_DISK, CLIENT_VERSION};
    snapshot_prefix(quorum_types);
    WriteCompactSize(quorum_types, Consensus::available_llmqs.size() + 1);
    evo::EvoSnapshot decoded;
    BOOST_CHECK_THROW(quorum_types >> decoded, std::ios_base::failure);

    CDataStream history{SER_DISK, CLIENT_VERSION};
    snapshot_prefix(history);
    WriteCompactSize(history, 0);
    WriteCompactSize(history, evo::EvoSnapshotMaxHistoricalMNLists() + 1);
    BOOST_CHECK_THROW(history >> decoded, std::ios_base::failure);

    CDataStream signals{SER_DISK, CLIENT_VERSION};
    snapshot_prefix(signals);
    WriteCompactSize(signals, 0);
    WriteCompactSize(signals, 0);
    WriteCompactSize(signals, 0);
    signals << CCreditPool{};
    WriteCompactSize(signals, VERSIONBITS_NUM_BITS + 1);
    BOOST_CHECK_THROW(signals >> decoded, std::ios_base::failure);

    CDataStream ranges{SER_DISK, CLIENT_VERSION};
    snapshot_prefix(ranges);
    WriteCompactSize(ranges, 0);
    WriteCompactSize(ranges, 0);
    WriteCompactSize(ranges, 0);
    ranges << CAmount{0} << CAmount{0} << CAmount{0};
    WriteCompactSize(ranges, evo::EVO_SNAPSHOT_MAX_RANGES + 1);
    BOOST_CHECK_THROW(ranges >> decoded, std::ios_base::failure);
    BOOST_CHECK(ranges.empty());

    const auto snapshot_bytes{SerializeSnapshot(SyntheticSnapshot())};
    DomainPort domain;
    BOOST_REQUIRE_EQUAL(domain.Set("evo5.example.org", 443), DomainPort::Status::Success);
    CDataStream encoded_domain{SER_DISK, CLIENT_VERSION};
    encoded_domain << domain;
    const auto domain_pos{std::search(snapshot_bytes.begin(), snapshot_bytes.end(),
                                      encoded_domain.begin(), encoded_domain.end())};
    BOOST_REQUIRE(domain_pos != snapshot_bytes.end());

    CDataStream oversized_domain{SER_DISK, CLIENT_VERSION};
    const size_t domain_offset{static_cast<size_t>(std::distance(snapshot_bytes.begin(), domain_pos))};
    oversized_domain.write(Span{snapshot_bytes}.first(domain_offset));
    constexpr size_t MAX_DOMAIN_LENGTH{253};
    WriteCompactSize(oversized_domain, MAX_DOMAIN_LENGTH + 1);
    const std::string oversized_addr(MAX_DOMAIN_LENGTH + 1, 'a');
    oversized_domain.write(MakeByteSpan(oversized_addr));
    const size_t serialized_addr_size{encoded_domain.size() - sizeof(uint16_t)};
    oversized_domain.write(Span{snapshot_bytes}.subspan(domain_offset + serialized_addr_size));
    BOOST_CHECK_EXCEPTION(oversized_domain >> decoded, std::ios_base::failure,
                          [](const auto& e) { return std::string{e.what()}.find("String length limit exceeded") != std::string::npos; });
}

BOOST_FIXTURE_TEST_CASE(commitment_sizes_are_format_bounded_not_param_exact, BasicTestingSetup)
{
    // -llmqtestparams and -llmqdevnetparams change the effective quorum size at
    // runtime, so commitments whose bitsets differ from the static default must
    // pass this layer; exact sizing is established by chain-aware validation.
    auto snapshot{SyntheticSnapshot()};
    const auto& params{evo::SnapshotLLMQParams(snapshot.quorums[0].llmq_type)};
    for (auto& entry : snapshot.quorums[0].active_commitments) {
        entry.commitment.signers.assign(params.size + 5, false);
        entry.commitment.validMembers.assign(params.size + 5, true);
    }
    BOOST_CHECK_NO_THROW(snapshot.Validate(/*require_canonical_order=*/true));
    const auto bytes{SerializeSnapshot(snapshot)};
    CDataStream input{bytes};
    evo::EvoSnapshot decoded;
    BOOST_CHECK_NO_THROW(input >> decoded);
    BOOST_CHECK(input.empty());
    const auto reencoded{SerializeSnapshot(decoded)};
    BOOST_CHECK_EQUAL_COLLECTIONS(bytes.begin(), bytes.end(), reencoded.begin(), reencoded.end());
}

BOOST_FIXTURE_TEST_CASE(canonical_sort_preserves_input, BasicTestingSetup)
{
    const auto snapshot{SyntheticSnapshot(/*reverse_representation=*/true)};
    const auto original{SerializeSnapshot(snapshot)};
    const auto first_type{snapshot.quorums.front().llmq_type};
    const auto sorted{evo::CanonicallySortedCopy(snapshot.quorums)};
    BOOST_CHECK(evo::IsCanonicallySorted(sorted));
    BOOST_CHECK(!evo::IsCanonicallySorted(snapshot.quorums));
    BOOST_CHECK(snapshot.quorums.front().llmq_type == first_type);
    const auto after{SerializeSnapshot(snapshot)};
    BOOST_CHECK_EQUAL_COLLECTIONS(original.begin(), original.end(), after.begin(), after.end());
}

BOOST_FIXTURE_TEST_CASE(rotation_flag_wire_encoding_is_canonical, BasicTestingSetup)
{
    for (const uint8_t flag : {0, 1, 2, 255}) {
        CDataStream input{SER_DISK, CLIENT_VERSION};
        input << Consensus::LLMQType::LLMQ_TEST_DIP0024 << flag;
        for (int i{0}; i < 3; ++i) WriteCompactSize(input, 0);
        evo::QuorumSnapshotData decoded;
        if (flag <= 1) {
            BOOST_CHECK_NO_THROW(input >> decoded);
            BOOST_CHECK_EQUAL(decoded.rotation_enabled, flag != 0);
        } else {
            BOOST_CHECK_EXCEPTION(input >> decoded, std::ios_base::failure, [](const auto& e) {
                return std::string{e.what()}.find("noncanonical evo quorum rotation flag") != std::string::npos;
            });
        }
    }
}

BOOST_FIXTURE_TEST_CASE(nested_mn_map_wire_order_is_canonical, BasicTestingSetup)
{
    const auto mn{MN(5, 1, MnType::Evo, ProTxVersion::ExtAddr, 5)};
    auto net_info{mn->pdmnState->netInfo};
    CDataStream canonical_info{SER_DISK, CLIENT_VERSION};
    canonical_info << NetInfoSerWrapper(net_info, /*is_extended=*/true);
    auto info_input{canonical_info};
    uint8_t version;
    info_input >> version;
    const auto count{ReadCompactSize(info_input)};
    std::vector<std::pair<NetInfoPurpose, NetInfoList>> entries(count);
    for (auto& entry : entries) info_input >> entry;
    BOOST_REQUIRE(entries.size() > 1);
    BOOST_REQUIRE(info_input.empty());

    const auto is_noncanonical = [](const auto& e) {
        return std::string{e.what()}.find("ExtNetInfo:") != std::string::npos;
    };
    for (const bool duplicate : {false, true}) {
        auto changed_entries{entries};
        if (duplicate) {
            changed_entries.insert(changed_entries.begin(), entries.front());
        } else {
            std::reverse(changed_entries.begin(), changed_entries.end());
        }
        CDataStream changed_info{SER_DISK, CLIENT_VERSION};
        changed_info << version << changed_entries;
        const auto replace_info = [&](const auto& obj) {
            CDataStream encoded{SER_DISK, CLIENT_VERSION};
            encoded << obj;
            const auto pos{std::search(encoded.begin(), encoded.end(), canonical_info.begin(), canonical_info.end())};
            BOOST_REQUIRE(pos != encoded.end());
            const size_t offset{static_cast<size_t>(pos - encoded.begin())};
            CDataStream changed{SER_DISK, CLIENT_VERSION};
            changed.write(Span{encoded}.first(offset));
            changed.write(Span{changed_info});
            changed.write(Span{encoded}.subspan(offset + canonical_info.size()));
            return changed;
        };
        const auto changed_mn{replace_info(*mn)};
        auto ordinary_input{changed_mn};
        BOOST_CHECK_THROW(CDeterministicMN(deserialize, ordinary_input), std::ios_base::failure);

        CDataStream base{SER_DISK, CLIENT_VERSION};
        base << H(42) << 42 << uint32_t{10};
        WriteCompactSize(base, 1);
        base.write(Span{changed_mn});
        BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNList(base), std::ios_base::failure, is_noncanonical);

        CDataStream addition{SER_DISK, CLIENT_VERSION};
        WriteCompactSize(addition, 1);
        addition.write(Span{changed_mn});
        WriteCompactSize(addition, 0);
        WriteCompactSize(addition, 0);
        BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNListDiff(addition), std::ios_base::failure, is_noncanonical);

        const auto old_mn{MN(5, 1, MnType::Evo, ProTxVersion::ExtAddr, 6)};
        const CDeterministicMNStateDiff state_diff{*old_mn->pdmnState, *mn->pdmnState};
        const auto changed_diff{replace_info(state_diff)};
        CDataStream update{SER_DISK, CLIENT_VERSION};
        WriteCompactSize(update, 0);
        WriteCompactSize(update, 1);
        WriteVarInt<CDataStream, VarIntMode::DEFAULT, uint64_t>(update, 5);
        update.write(Span{changed_diff});
        WriteCompactSize(update, 0);
        BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNListDiff(update), std::ios_base::failure, is_noncanonical);
    }
}

BOOST_FIXTURE_TEST_CASE(validation_matches_height_and_range_decode_bounds, BasicTestingSetup)
{
    evo::EvoSnapshot snapshot;
    snapshot.base_block_hash = H(42);
    snapshot.mn_list.SetBlockHash(snapshot.base_block_hash);
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, [](const auto& e) {
        return std::string{e.what()}.find("negative canonical MN-list height") != std::string::npos;
    });
    BOOST_CHECK_THROW(GetEvoSnapshotHash(snapshot), std::ios_base::failure);
    snapshot.mn_list = CDeterministicMNList{snapshot.base_block_hash, 0, 0};
    for (size_t i{0}; i < evo::EVO_SNAPSHOT_MAX_RANGES; ++i) {
        BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(2 * i));
    }
    BOOST_CHECK_EQUAL(snapshot.credit_pool.indexes.RangeCount(), evo::EVO_SNAPSHOT_MAX_RANGES);
    BOOST_CHECK_NO_THROW(snapshot.Validate());
    auto bytes{SerializeSnapshot(snapshot)};
    evo::EvoSnapshot decoded;
    BOOST_CHECK_NO_THROW(bytes >> decoded);
    BOOST_REQUIRE(snapshot.credit_pool.indexes.Add(2 * evo::EVO_SNAPSHOT_MAX_RANGES));
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, [](const auto& e) {
        return std::string{e.what()}.find("oversized CRangesSet range count") != std::string::npos;
    });
    BOOST_CHECK_THROW(GetEvoSnapshotHash(snapshot), std::ios_base::failure);

    CDataStream continuous{SER_DISK, CLIENT_VERSION};
    WriteCompactSize(continuous, 1);
    continuous << uint64_t{0} << uint64_t{evo::EVO_SNAPSHOT_MAX_RANGES + 1};
    continuous >> snapshot.credit_pool.indexes;
    BOOST_CHECK_EQUAL(snapshot.credit_pool.indexes.RangeCount(), 1U);
    BOOST_CHECK_GT(snapshot.credit_pool.indexes.Size(), evo::EVO_SNAPSHOT_MAX_RANGES);
    BOOST_CHECK_NO_THROW(snapshot.Validate());
}

BOOST_FIXTURE_TEST_CASE(validation_enforces_nested_mn_decode_budget, BasicTestingSetup)
{
    const auto is_budget_error = [](const auto& e) {
        return std::string{e.what()}.find("CompactSize budget exceeded") != std::string::npos;
    };
    auto mn{std::const_pointer_cast<CDeterministicMN>(MN(2, 3, MnType::Regular, ProTxVersion::LegacyBLS, 3))};
    auto state{std::make_shared<CDeterministicMNState>(*mn->pdmnState)};
    // Legacy net info has no CompactSizes; the two scripts share the budget.
    state->scriptPayout.assign(evo::EVO_SNAPSHOT_MAX_MN_COMPACT_ITEMS / 2, OP_TRUE);
    state->scriptOperatorPayout = state->scriptPayout;
    mn->pdmnState = state;
    evo::EvoSnapshot snapshot;
    snapshot.base_block_hash = H(42);
    snapshot.mn_list = CDeterministicMNList{snapshot.base_block_hash, 42, 10};
    snapshot.mn_list.AddMN(mn, /*fBumpTotalCount=*/false);
    BOOST_CHECK_NO_THROW(snapshot.Validate());
    auto bytes{SerializeSnapshot(snapshot)};
    evo::EvoSnapshot decoded;
    BOOST_CHECK_NO_THROW(bytes >> decoded);

    state = std::make_shared<CDeterministicMNState>(*state);
    state->scriptOperatorPayout.push_back(OP_TRUE);
    snapshot.mn_list.UpdateMN(mn->proTxHash, state);
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, is_budget_error);
    BOOST_CHECK_THROW(GetEvoSnapshotHash(snapshot), std::ios_base::failure);
    bytes = SerializeSnapshot(snapshot);
    BOOST_CHECK_EXCEPTION(bytes >> decoded, std::ios_base::failure, is_budget_error);

    snapshot = SyntheticSnapshot();
    mn->pdmnState = state;
    snapshot.historical_mn_list_diffs.front().diff.addedMNs.push_back(mn);
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, is_budget_error);

    // A diff can carry a legacy payout field even when the resulting extended
    // MN's full encoding omits that field. Check the diff's own budget as well.
    snapshot = SyntheticSnapshot();
    const auto extended{MN(5, 1, MnType::Evo, ProTxVersion::ExtAddr, 5)};
    CDeterministicMNState changed{*extended->pdmnState};
    changed.scriptPayout.assign(evo::EVO_SNAPSHOT_MAX_MN_COMPACT_ITEMS + 1, OP_TRUE);
    snapshot.historical_mn_list_diffs.front().diff.updatedMNs.emplace(
        5, CDeterministicMNStateDiff{*extended->pdmnState, changed});
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, is_budget_error);

    // Both the base MN and diff fit separately, but the update combines two
    // scripts whose total exceeds the full-MN budget.
    snapshot = SyntheticSnapshot();
    const auto base_mn{snapshot.mn_list.GetMNByInternalId(2)};
    auto base_state{std::make_shared<CDeterministicMNState>(*base_mn->pdmnState)};
    base_state->scriptPayout.assign(evo::EVO_SNAPSHOT_MAX_MN_COMPACT_ITEMS / 2, OP_TRUE);
    snapshot.mn_list.UpdateMN(base_mn->proTxHash, base_state);
    CDeterministicMNState combined{*base_state};
    combined.scriptOperatorPayout.assign(evo::EVO_SNAPSHOT_MAX_MN_COMPACT_ITEMS / 2 + 1, OP_TRUE);
    snapshot.historical_mn_list_diffs.front().diff.updatedMNs.emplace(
        2, CDeterministicMNStateDiff{*base_state, combined});
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, is_budget_error);
}

BOOST_FIXTURE_TEST_CASE(mnhf_signal_wire_order_is_canonical, BasicTestingSetup)
{
    const auto bytes{SerializeSnapshot(SyntheticSnapshot())};
    // The MNHF signal section is the encoding's tail: a count followed by
    // (bit, height) pairs. SyntheticSnapshot carries (2, 12) and (9, 30).
    const size_t tail_size{1 + 2 * (sizeof(uint8_t) + sizeof(int32_t))};
    const auto with_signals = [&](const std::vector<std::pair<uint8_t, int>>& signals) {
        CDataStream stream{SER_DISK, CLIENT_VERSION};
        stream.write(Span{bytes}.first(bytes.size() - tail_size));
        WriteCompactSize(stream, signals.size());
        for (const auto& signal : signals) stream << signal;
        return stream;
    };
    const auto expect_noncanonical = [](CDataStream stream) {
        evo::EvoSnapshot decoded;
        BOOST_CHECK_EXCEPTION(stream >> decoded, std::ios_base::failure, [](const auto& e) {
            return std::string{e.what()}.find("noncanonical MNHF signal order") != std::string::npos;
        });
    };
    auto canonical{with_signals({{2, 12}, {9, 30}})};
    evo::EvoSnapshot decoded;
    BOOST_CHECK_NO_THROW(canonical >> decoded);
    BOOST_CHECK(canonical.empty());
    BOOST_CHECK_EQUAL(decoded.mnhf_signals.size(), 2U);
    expect_noncanonical(with_signals({{9, 30}, {2, 12}}));
    expect_noncanonical(with_signals({{2, 12}, {2, 30}}));
}

BOOST_FIXTURE_TEST_CASE(mnhf_signals_outnumbering_deployments_are_valid, BasicTestingSetup)
{
    // CMNHFManager drops a signal only when a current deployment reuses its
    // bit, so signals of buried EHF forks (bits 10, 11 and 12) stay in the
    // map alongside V24's bit 13.
    auto snapshot{SyntheticSnapshot()};
    snapshot.mnhf_signals = {{10, 100}, {11, 200}, {12, 300}, {13, 400}};
    BOOST_REQUIRE_GT(snapshot.mnhf_signals.size(), size_t{Consensus::MAX_VERSION_BITS_DEPLOYMENTS});

    BOOST_CHECK_NO_THROW(snapshot.Validate());
    BOOST_CHECK_NO_THROW(GetEvoSnapshotHash(snapshot));

    CDataStream stream{SerializeSnapshot(snapshot)};
    evo::EvoSnapshot decoded;
    BOOST_CHECK_NO_THROW(stream >> decoded);
    BOOST_CHECK(decoded.mnhf_signals == snapshot.mnhf_signals);
}

BOOST_FIXTURE_TEST_CASE(unserialize_replaces_previous_contents, BasicTestingSetup)
{
    const auto populated_bytes{SerializeSnapshot(SyntheticSnapshot())};
    evo::EvoSnapshot minimal;
    minimal.base_block_hash = H(42);
    minimal.mn_list = CDeterministicMNList{H(42), 500, 0};
    const auto minimal_bytes{SerializeSnapshot(minimal)};

    evo::EvoSnapshot decoded;
    CDataStream populated{populated_bytes};
    populated >> decoded;
    BOOST_REQUIRE(!decoded.mnhf_signals.empty());
    CDataStream empty{minimal_bytes};
    empty >> decoded;
    BOOST_CHECK(decoded.quorums.empty());
    BOOST_CHECK(decoded.historical_mn_list_diffs.empty());
    BOOST_CHECK(decoded.quorum_modifiers.empty());
    BOOST_CHECK(decoded.mnhf_signals.empty());
    const auto reencoded{SerializeSnapshot(decoded)};
    BOOST_CHECK_EQUAL_COLLECTIONS(minimal_bytes.begin(), minimal_bytes.end(), reencoded.begin(), reencoded.end());
}

BOOST_FIXTURE_TEST_CASE(hash_prefix_collision_runs_are_bounded, BasicTestingSetup)
{
    // Every MN() proTxHash shares CollidingH's 64-bit prefix, so run length
    // equals list size here.
    const auto write_list = [](size_t count) {
        CDataStream stream{SER_DISK, CLIENT_VERSION};
        stream << H(42) << 42 << uint32_t{200};
        WriteCompactSize(stream, count);
        for (size_t i{0}; i < count; ++i) {
            stream << *MN(i + 1, static_cast<uint8_t>(i + 1), MnType::Regular, ProTxVersion::LegacyBLS,
                          static_cast<uint8_t>(i + 1));
        }
        return stream;
    };
    auto at_bound{write_list(evo::EVO_SNAPSHOT_MAX_HASH_PREFIX_RUN)};
    BOOST_CHECK_NO_THROW(evo::UnserializeCanonicalMNList(at_bound));
    auto over_bound{write_list(evo::EVO_SNAPSHOT_MAX_HASH_PREFIX_RUN + 1)};
    BOOST_CHECK_EXCEPTION(evo::UnserializeCanonicalMNList(over_bound), std::ios_base::failure, [](const auto& e) {
        return std::string{e.what()}.find("collision bound") != std::string::npos;
    });

    // A diff addition that would grow an at-bound collision group is rejected
    // before the HAMT performs the inserts.
    evo::EvoSnapshot snapshot;
    snapshot.base_block_hash = H(42);
    snapshot.mn_list = CDeterministicMNList{H(42), 500, 200};
    for (size_t i{0}; i < evo::EVO_SNAPSHOT_MAX_HASH_PREFIX_RUN; ++i) {
        snapshot.mn_list.AddMN(MN(i + 1, static_cast<uint8_t>(i + 1), MnType::Regular, ProTxVersion::LegacyBLS,
                                  static_cast<uint8_t>(i + 1)),
                               /*fBumpTotalCount=*/false);
    }
    CDeterministicMNListDiff diff;
    diff.addedMNs.push_back(MN(30, 30, MnType::Regular, ProTxVersion::LegacyBLS, 30));
    snapshot.historical_mn_list_diffs.push_back({H(42), H(43), 499, 200, H(1), std::move(diff)});
    std::map<uint256, CDeterministicMNList> lists;
    std::string reconstruction_error;
    BOOST_CHECK(!evo::ReconstructHistoricalMNLists(snapshot, lists, reconstruction_error));
    BOOST_CHECK(reconstruction_error.find("collision bound") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(quorum_data_unserialize_replaces_previous_contents, BasicTestingSetup)
{
    evo::QuorumSnapshotData data;
    data.llmq_type = Consensus::LLMQType::LLMQ_TEST;
    data.active_commitments = {Commitment(data.llmq_type, 11, 51, false)};
    CDataStream encoded{SER_DISK, CLIENT_VERSION};
    encoded << data;

    // Every vector the decoder clears starts non-empty and the payload leaves
    // two of them empty, so dropping any single clear() leaves stale entries.
    evo::QuorumSnapshotData reused;
    reused.llmq_type = Consensus::LLMQType::LLMQ_TEST;
    reused.active_commitments = {Commitment(reused.llmq_type, 21, 61, false)};
    reused.safety_commitments = {Commitment(reused.llmq_type, 22, 62, false)};
    reused.rotation_snapshots = {
        {H(23), H(63), llmq::CQuorumSnapshot{{true, false}, SnapshotSkipMode::MODE_NO_SKIPPING, {}}}};

    encoded >> reused;
    BOOST_CHECK_EQUAL(reused.active_commitments.size(), 1U);
    BOOST_CHECK(reused.active_commitments.at(0).quorum_base_block_hash ==
                data.active_commitments.at(0).quorum_base_block_hash);
    BOOST_CHECK_EQUAL(reused.safety_commitments.size(), 0U);
    BOOST_CHECK_EQUAL(reused.rotation_snapshots.size(), 0U);
}

BOOST_FIXTURE_TEST_CASE(validation_enforces_the_decode_operation_budget, BasicTestingSetup)
{
    auto snapshot{SyntheticSnapshot()};
    const size_t budget{evo::EvoSnapshotMaxHistoricalMNOperations()};
    size_t operations{evo::EvoSnapshotHistoricalMNOperations(snapshot.historical_mn_list_diffs)};
    BOOST_REQUIRE(operations < budget);
    BOOST_CHECK_NO_THROW(snapshot.Validate());

    // Removals are the cheapest chargeable operation and are bounded per diff
    // by EVO_SNAPSHOT_MAX_MNS, so spread them over entries until the
    // cumulative ceiling is passed by exactly one. Ids start above anything
    // BuildDiff() produced so every insertion counts.
    uint64_t next_id{1'000'000};
    for (auto& entry : snapshot.historical_mn_list_diffs) {
        while (operations <= budget && entry.diff.removedMns.size() < evo::EVO_SNAPSHOT_MAX_MNS) {
            entry.diff.removedMns.emplace(next_id++);
            ++operations;
        }
        if (operations > budget) break;
    }
    BOOST_REQUIRE_EQUAL(operations, budget + 1);
    BOOST_CHECK_EXCEPTION(snapshot.Validate(), std::ios_base::failure, [](const auto& e) {
        return std::string{e.what()}.find("operation budget") != std::string::npos;
    });
}

BOOST_FIXTURE_TEST_CASE(historical_chain_rejects_revisited_blocks_and_counter_growth, BasicTestingSetup)
{
    const auto broken_chain = [](const evo::EvoSnapshot& snapshot) {
        std::map<uint256, CDeterministicMNList> lists;
        std::string error;
        BOOST_CHECK(!evo::ReconstructHistoricalMNLists(snapshot, lists, error));
        BOOST_CHECK_MESSAGE(error.find("broken historical MN-list diff chain") != std::string::npos, error);
    };

    // A transition must leave its predecessor: neither the first entry nor a
    // later one may target the base block, and no target may repeat.
    auto snapshot{SyntheticSnapshot()};
    auto history{evo::CanonicallySortedCopy(snapshot.historical_mn_list_diffs)};
    BOOST_REQUIRE(history.size() >= 3);
    snapshot.historical_mn_list_diffs = history;
    snapshot.historical_mn_list_diffs[0].block_hash = snapshot.base_block_hash;
    broken_chain(snapshot);
    snapshot.historical_mn_list_diffs = history;
    snapshot.historical_mn_list_diffs[1].block_hash = snapshot.base_block_hash;
    snapshot.historical_mn_list_diffs[2].previous_block_hash = snapshot.base_block_hash;
    broken_chain(snapshot);
    snapshot.historical_mn_list_diffs = history;
    snapshot.historical_mn_list_diffs[2].block_hash = history[0].block_hash;
    broken_chain(snapshot);

    // Registrations only ever raise the counter, so replaying backwards it can
    // never grow. A forged entry that also recomputes its canonical hash would
    // otherwise pass every remaining check.
    snapshot.historical_mn_list_diffs = history;
    auto& entry{snapshot.historical_mn_list_diffs[0]};
    std::map<uint256, CDeterministicMNList> lists;
    std::string error;
    BOOST_REQUIRE_MESSAGE(evo::ReconstructHistoricalMNLists(snapshot, lists, error), error);
    const auto& reconstructed{lists.at(entry.block_hash)};
    ++entry.total_registered_count;
    CDeterministicMNList inflated{entry.block_hash, entry.height, entry.total_registered_count};
    reconstructed.ForEachMNShared(/*onlyValid=*/false,
                                  [&](const auto& dmn) { inflated.AddMN(dmn, /*fBumpTotalCount=*/false); });
    entry.canonical_list_hash = evo::CanonicalMNListHash(inflated);
    broken_chain(snapshot);
}

BOOST_FIXTURE_TEST_CASE(reconstruction_record_budget_is_cumulative, BasicTestingSetup)
{
    const auto snapshot{SyntheticSnapshot()};
    std::map<uint256, CDeterministicMNList> lists;
    std::string error;
    BOOST_REQUIRE(evo::ReconstructHistoricalMNLists(snapshot, lists, error));
    // Every historical entry here carries the same 3-MN list with no
    // additions, so the cumulative charge is exactly 3 records per entry.
    const size_t total_records{3 * snapshot.historical_mn_list_diffs.size()};
    BOOST_CHECK(evo::ReconstructHistoricalMNLists(snapshot, lists, error, total_records));
    BOOST_CHECK(!evo::ReconstructHistoricalMNLists(snapshot, lists, error, total_records - 1));
    BOOST_CHECK(error.find("record budget") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(cbtx_cross_checks, BasicTestingSetup)
{
    const auto snapshot{SyntheticSnapshot()};
    CCbTx cbtx;
    cbtx.nVersion = CCbTx::Version::CLSIG_AND_BALANCE;
    cbtx.nHeight = snapshot.mn_list.GetHeightForSnapshotCodec();
    cbtx.merkleRootMNList = snapshot.mn_list.to_sml()->CalcMerkleRoot();
    std::vector<uint256> quorum_hashes;
    for (const auto& data : snapshot.quorums) {
        for (const auto& entry : data.active_commitments) quorum_hashes.emplace_back(SerializeHash(entry.commitment));
    }
    std::sort(quorum_hashes.begin(), quorum_hashes.end());
    cbtx.merkleRootQuorums = ComputeMerkleRoot(quorum_hashes);
    cbtx.creditPoolBalance = snapshot.credit_pool.locked;

    std::string error;
    BOOST_CHECK(evo::VerifyEvoSnapshotCbTx(snapshot, cbtx, error));
    cbtx.merkleRootMNList = H(1);
    BOOST_CHECK(!evo::VerifyEvoSnapshotCbTx(snapshot, cbtx, error));
    cbtx.merkleRootMNList = snapshot.mn_list.to_sml()->CalcMerkleRoot();
    cbtx.merkleRootQuorums = H(2);
    BOOST_CHECK(!evo::VerifyEvoSnapshotCbTx(snapshot, cbtx, error));
    cbtx.merkleRootQuorums = ComputeMerkleRoot(quorum_hashes);
    cbtx.creditPoolBalance++;
    BOOST_CHECK(!evo::VerifyEvoSnapshotCbTx(snapshot, cbtx, error));
    cbtx.creditPoolBalance = snapshot.credit_pool.locked;
    cbtx.nHeight++;
    BOOST_CHECK(!evo::VerifyEvoSnapshotCbTx(snapshot, cbtx, error));
    BOOST_CHECK(error.find("coinbase height") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(rejects_unknown_wire_version, BasicTestingSetup)
{
    auto bytes{SerializeSnapshot(SyntheticSnapshot())};
    bytes.data()[0] = std::byte{4};
    evo::EvoSnapshot decoded;
    BOOST_CHECK_THROW(bytes >> decoded, std::ios_base::failure);
}

BOOST_AUTO_TEST_SUITE_END()
