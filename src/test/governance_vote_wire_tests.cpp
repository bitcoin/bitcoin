// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <governance/vote.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

#include <ios>
#include <limits>
#include <vector>

BOOST_FIXTURE_TEST_SUITE(governance_vote_wire_tests, BasicTestingSetup)

namespace {
void WriteVoteHeader(CDataStream& ss)
{
    ss << COutPoint{uint256::ONE, 0} << uint256::ONE
       << int{1} /*outcome*/ << int{1} /*signal*/ << int64_t{1'700'000'000};
}

CDataStream MakeVoteWire(size_t sig_len)
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    WriteVoteHeader(ss);
    ss << std::vector<unsigned char>(sig_len, 0xAA);
    return ss;
}
} // namespace

// Reject invalid signature lengths, including a maximal CompactSize prefix.
BOOST_AUTO_TEST_CASE(rejects_invalid_sizes)
{
    for (size_t bad : {size_t{0}, size_t{64}, size_t{66}, size_t{95}, size_t{97}, size_t{128}}) {
        CDataStream ss = MakeVoteWire(bad);
        CGovernanceVote vote;
        BOOST_CHECK_THROW(ss >> vote, std::ios_base::failure);
    }

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    WriteVoteHeader(ss);
    WriteCompactSize(ss, std::numeric_limits<uint64_t>::max());
    CGovernanceVote vote;
    BOOST_CHECK_THROW(ss >> vote, std::ios_base::failure);
}

// Truncated element bytes must surface as ios_base::failure so the govobjvote
// handler scores the peer.
BOOST_AUTO_TEST_CASE(truncated_signature_throws_ios_failure)
{
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    WriteVoteHeader(ss);
    ss << uint8_t{CGovernanceVote::BLS_SIG_SIZE};
    ss.write(MakeByteSpan(std::vector<unsigned char>(10, 0xBB)));

    CGovernanceVote vote;
    BOOST_CHECK_THROW(ss >> vote, std::ios_base::failure);
}

// 65-byte ECDSA and 96-byte BLS round-trip cleanly over the network.
BOOST_AUTO_TEST_CASE(accepts_legitimate_boundary_sizes)
{
    for (size_t sig_len : {CGovernanceVote::COMPACT_SIG_SIZE, CGovernanceVote::BLS_SIG_SIZE}) {
        CDataStream ss = MakeVoteWire(sig_len);
        const size_t wire_bytes = ss.size();

        CGovernanceVote vote;
        BOOST_REQUIRE_NO_THROW(ss >> vote);
        BOOST_CHECK_EQUAL(ss.size(), 0U);

        CDataStream out(SER_NETWORK, PROTOCOL_VERSION);
        out << vote;
        BOOST_CHECK_EQUAL(out.size(), wire_bytes);
    }
}

// SER_DISK reads stay unbounded — existing on-disk data must load unchanged.
BOOST_AUTO_TEST_CASE(ser_disk_deserialization_unaffected)
{
    CDataStream ss(SER_DISK, PROTOCOL_VERSION);
    WriteVoteHeader(ss);
    ss << std::vector<unsigned char>(128, 0xCD);

    CGovernanceVote vote;
    BOOST_REQUIRE_NO_THROW(ss >> vote);
    BOOST_CHECK_EQUAL(ss.size(), 0U);
}

BOOST_AUTO_TEST_SUITE_END()
