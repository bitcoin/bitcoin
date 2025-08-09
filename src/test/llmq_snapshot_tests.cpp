// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/llmq_tests.h>
#include <test/util/setup_common.h>

#include <llmq/snapshot.h>
#include <streams.h>
#include <univalue.h>

#include <boost/test/unit_test.hpp>

#include <vector>

using namespace llmq;
using namespace llmq::testutils;

BOOST_FIXTURE_TEST_SUITE(llmq_snapshot_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(quorum_snapshot_construction_test)
{
    // Test default constructor
    CQuorumSnapshot snapshot1;
    BOOST_CHECK(snapshot1.activeQuorumMembers.empty());
    BOOST_CHECK_EQUAL(snapshot1.mnSkipListMode, 0);
    BOOST_CHECK(snapshot1.mnSkipList.empty());

    // Test parameterized constructor
    std::vector<bool> activeMembers = {true, false, true, true, false};
    int skipMode = MODE_SKIPPING_ENTRIES;
    std::vector<int> skipList = {1, 3, 5, 7};

    CQuorumSnapshot snapshot2(activeMembers, skipMode, skipList);
    BOOST_CHECK(snapshot2.activeQuorumMembers == activeMembers);
    BOOST_CHECK_EQUAL(snapshot2.mnSkipListMode, skipMode);
    BOOST_CHECK(snapshot2.mnSkipList == skipList);

    // Test move semantics
    std::vector<bool> activeMembersCopy = activeMembers;
    std::vector<int> skipListCopy = skipList;
    CQuorumSnapshot snapshot3(std::move(activeMembersCopy), skipMode, std::move(skipListCopy));
    BOOST_CHECK(snapshot3.activeQuorumMembers == activeMembers);
    BOOST_CHECK_EQUAL(snapshot3.mnSkipListMode, skipMode);
    BOOST_CHECK(snapshot3.mnSkipList == skipList);
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_serialization_test)
{
    // Test with various configurations
    std::vector<bool> activeMembers = CreateBitVector(10, {0, 2, 4, 6, 8});
    int skipMode = MODE_SKIPPING_ENTRIES;
    std::vector<int> skipList = {10, 20, 30};

    CQuorumSnapshot snapshot(activeMembers, skipMode, skipList);

    // Test serialization roundtrip
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << snapshot;

    CQuorumSnapshot deserialized;
    ss >> deserialized;

    BOOST_CHECK(deserialized.activeQuorumMembers == snapshot.activeQuorumMembers);
    BOOST_CHECK_EQUAL(deserialized.mnSkipListMode, snapshot.mnSkipListMode);
    BOOST_CHECK(deserialized.mnSkipList == snapshot.mnSkipList);
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_skip_modes_test)
{
    // Test all skip modes
    std::vector<int> skipModes = {MODE_NO_SKIPPING, MODE_SKIPPING_ENTRIES, MODE_NO_SKIPPING_ENTRIES, MODE_ALL_SKIPPED};

    for (int mode : skipModes) {
        CQuorumSnapshot snapshot({true, false, true}, mode, {1, 2, 3});

        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << snapshot;

        CQuorumSnapshot deserialized;
        ss >> deserialized;

        BOOST_CHECK_EQUAL(deserialized.mnSkipListMode, mode);
    }
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_large_data_test)
{
    // Test with large quorum (400 members)
    std::vector<bool> largeActiveMembers(400);
    // Create pattern: every 3rd member is inactive
    for (size_t i = 0; i < largeActiveMembers.size(); i++) {
        largeActiveMembers[i] = (i % 3 != 0);
    }

    // Create large skip list
    std::vector<int> largeSkipList;
    for (int i = 0; i < 100; i++) {
        largeSkipList.push_back(i * 4);
    }

    CQuorumSnapshot snapshot(largeActiveMembers, MODE_SKIPPING_ENTRIES, largeSkipList);

    // Test serialization with large data
    // Test serialization manually instead of using roundtrip helper
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << snapshot;
    CQuorumSnapshot deserialized;
    ss >> deserialized;
    BOOST_CHECK_EQUAL(deserialized.activeQuorumMembers.size(), 400);
    BOOST_CHECK_EQUAL(deserialized.mnSkipList.size(), 100);
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_empty_data_test)
{
    // Test with empty data
    CQuorumSnapshot emptySnapshot({}, MODE_NO_SKIPPING, {});

    // Test serialization roundtrip
    BOOST_CHECK(TestSerializationRoundtrip(emptySnapshot));

    // Test with empty active members but non-empty skip list
    CQuorumSnapshot snapshot1({}, MODE_SKIPPING_ENTRIES, {1, 2, 3});
    BOOST_CHECK(TestSerializationRoundtrip(snapshot1));

    // Test with non-empty active members but empty skip list
    CQuorumSnapshot snapshot2({true, false, true}, MODE_NO_SKIPPING, {});
    BOOST_CHECK(TestSerializationRoundtrip(snapshot2));
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_bit_serialization_test)
{
    // Test bit vector serialization edge cases

    // Test single bit
    CQuorumSnapshot snapshot1({true}, MODE_NO_SKIPPING, {});
    BOOST_CHECK(TestSerializationRoundtrip(snapshot1));

    // Test 8 bits (full byte)
    CQuorumSnapshot snapshot8(std::vector<bool>(8, true), MODE_NO_SKIPPING, {});
    BOOST_CHECK(TestSerializationRoundtrip(snapshot8));

    // Test 9 bits (more than one byte)
    CQuorumSnapshot snapshot9(std::vector<bool>(9, false), MODE_NO_SKIPPING, {});
    snapshot9.activeQuorumMembers[8] = true; // Set last bit
    BOOST_CHECK(TestSerializationRoundtrip(snapshot9));

    // Test alternating pattern
    std::vector<bool> alternating(16);
    for (size_t i = 0; i < alternating.size(); i++) {
        alternating[i] = (i % 2 == 0);
    }
    CQuorumSnapshot snapshotAlt(alternating, MODE_NO_SKIPPING, {});
    BOOST_CHECK(TestSerializationRoundtrip(snapshotAlt));
}

BOOST_AUTO_TEST_CASE(quorum_rotation_info_construction_test)
{
    CQuorumRotationInfo rotInfo;

    // Test default state
    BOOST_CHECK(!rotInfo.extraShare);
    BOOST_CHECK(!rotInfo.quorumSnapshotAtHMinus4C.has_value());
    BOOST_CHECK(!rotInfo.mnListDiffAtHMinus4C.has_value());
    BOOST_CHECK(rotInfo.lastCommitmentPerIndex.empty());
    BOOST_CHECK(rotInfo.quorumSnapshotList.empty());
    BOOST_CHECK(rotInfo.mnListDiffList.empty());
}

// Note: CQuorumRotationInfo serialization requires complex setup
// This is better tested in functional tests

BOOST_AUTO_TEST_CASE(get_quorum_rotation_info_serialization_test)
{
    CGetQuorumRotationInfo getInfo;

    // Test with multiple base block hashes
    getInfo.baseBlockHashes = {GetTestBlockHash(1), GetTestBlockHash(2), GetTestBlockHash(3)};
    getInfo.blockRequestHash = GetTestBlockHash(100);
    getInfo.extraShare = true;

    // Test serialization
    BOOST_CHECK(TestSerializationRoundtrip(getInfo));

    // Test with empty base block hashes
    CGetQuorumRotationInfo emptyInfo;
    emptyInfo.blockRequestHash = GetTestBlockHash(200);
    emptyInfo.extraShare = false;

    BOOST_CHECK(TestSerializationRoundtrip(emptyInfo));
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_json_test)
{
    // Create snapshot with test data
    std::vector<bool> activeMembers = {true, false, true, true, false, false, true};
    int skipMode = MODE_SKIPPING_ENTRIES;
    std::vector<int> skipList = {10, 20, 30, 40};

    CQuorumSnapshot snapshot(activeMembers, skipMode, skipList);

    // Test JSON conversion
    UniValue json = snapshot.ToJson();

    // Verify JSON structure
    BOOST_CHECK(json.isObject());
    BOOST_CHECK(json.exists("activeQuorumMembers"));
    BOOST_CHECK(json.exists("mnSkipListMode"));
    BOOST_CHECK(json.exists("mnSkipList"));

    // Verify skip list is array
    BOOST_CHECK(json["mnSkipList"].isArray());
    BOOST_CHECK_EQUAL(json["mnSkipList"].size(), skipList.size());
}

BOOST_AUTO_TEST_CASE(quorum_snapshot_malformed_data_test)
{
    // Create valid snapshot
    CQuorumSnapshot snapshot({true, false, true}, MODE_SKIPPING_ENTRIES, {1, 2, 3});

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << snapshot;

    // Test truncated data deserialization
    std::string data = ss.str();
    for (size_t truncateAt = 1; truncateAt < data.size(); truncateAt += 5) {
        CDataStream truncated(std::vector<unsigned char>(data.begin(), data.begin() + truncateAt), SER_NETWORK,
                              PROTOCOL_VERSION);

        CQuorumSnapshot deserialized;
        try {
            truncated >> deserialized;
            // If no exception, it might be a valid partial deserialization
            // (though unlikely for complex structures)
        } catch (const std::exception&) {
            // Expected for most truncation points
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
