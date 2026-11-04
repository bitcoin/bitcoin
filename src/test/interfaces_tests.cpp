// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <evo/evodb.h>
#include <evo/providertx.h>
#include <instantsend/instantsend.h>
#include <instantsend/lock.h>
#include <interfaces/chain.h>
#include <interfaces/node.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <script/standard.h>
#include <spork.h>
#include <streams.h>
#include <test/util/llmq_tests.h>
#include <test/util/setup_common.h>
#include <util/std23.h>
#include <util/strencodings.h>
#include <validation.h>
#include <version.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <future>
#include <thread>

#include <boost/test/unit_test.hpp>

using interfaces::FoundBlock;

BOOST_FIXTURE_TEST_SUITE(interfaces_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(findBlock)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();

    uint256 hash;
    BOOST_CHECK(chain->findBlock(active[10]->GetBlockHash(), FoundBlock().hash(hash)));
    BOOST_CHECK_EQUAL(hash, active[10]->GetBlockHash());

    int height = -1;
    BOOST_CHECK(chain->findBlock(active[20]->GetBlockHash(), FoundBlock().height(height)));
    BOOST_CHECK_EQUAL(height, active[20]->nHeight);

    CBlock data;
    BOOST_CHECK(chain->findBlock(active[30]->GetBlockHash(), FoundBlock().data(data)));
    BOOST_CHECK_EQUAL(data.GetHash(), active[30]->GetBlockHash());

    int64_t time = -1;
    BOOST_CHECK(chain->findBlock(active[40]->GetBlockHash(), FoundBlock().time(time)));
    BOOST_CHECK_EQUAL(time, active[40]->GetBlockTime());

    int64_t max_time = -1;
    BOOST_CHECK(chain->findBlock(active[50]->GetBlockHash(), FoundBlock().maxTime(max_time)));
    BOOST_CHECK_EQUAL(max_time, active[50]->GetBlockTimeMax());

    int64_t mtp_time = -1;
    BOOST_CHECK(chain->findBlock(active[60]->GetBlockHash(), FoundBlock().mtpTime(mtp_time)));
    BOOST_CHECK_EQUAL(mtp_time, active[60]->GetMedianTimePast());

    bool cur_active{false}, next_active{false};
    uint256 next_hash;
    BOOST_CHECK_EQUAL(active.Height(), 100);
    BOOST_CHECK(chain->findBlock(active[99]->GetBlockHash(), FoundBlock().inActiveChain(cur_active).nextBlock(FoundBlock().inActiveChain(next_active).hash(next_hash))));
    BOOST_CHECK(cur_active);
    BOOST_CHECK(next_active);
    BOOST_CHECK_EQUAL(next_hash, active[100]->GetBlockHash());
    cur_active = next_active = false;
    BOOST_CHECK(chain->findBlock(active[100]->GetBlockHash(), FoundBlock().inActiveChain(cur_active).nextBlock(FoundBlock().inActiveChain(next_active))));
    BOOST_CHECK(cur_active);
    BOOST_CHECK(!next_active);

    BOOST_CHECK(!chain->findBlock({}, FoundBlock()));
}

BOOST_AUTO_TEST_CASE(providerTxCapabilities)
{
    auto node{interfaces::MakeNode(m_node)};
    const auto capabilities{node->evo().getProviderTxCapabilities()};
    const uint16_t expected_version{
        WITH_LOCK(::cs_main, return DeploymentToProtxVersion(Assert(m_node.chainman)->ActiveChain().Tip(),
                                                             *Assert(m_node.chainman)))};
    BOOST_CHECK_EQUAL(capabilities.version, expected_version);
    BOOST_CHECK_EQUAL(capabilities.extended_addresses, expected_version >= ProTxVersion::ExtAddr);
    BOOST_CHECK(!capabilities.ipv4_optional); // Not active on regtest by default
}

struct IPv4OptionalChainSetup : public TestChain100Setup {
    IPv4OptionalChainSetup()
        : TestChain100Setup{CBaseChainParams::REGTEST, {"-testactivationheight=ipv4_optional@1"}} {}
};

BOOST_FIXTURE_TEST_CASE(providerTxCapabilitiesIPv4Optional, IPv4OptionalChainSetup)
{
    BOOST_CHECK(interfaces::MakeNode(m_node)->evo().getProviderTxCapabilities().ipv4_optional);
}

BOOST_AUTO_TEST_CASE(providerNetInfoValidationDoesNotWaitForChain)
{
    // Dialogs validate endpoints on every keystroke, so that must not wait for cs_main
    auto node{interfaces::MakeNode(m_node)};
    const interfaces::ProviderNetInfo net_info{.core_p2p = {"1.1.1.1:9998"},
                                               .platform_p2p = std::vector<std::string>{"1.1.1.1:22200"},
                                               .platform_https = std::vector<std::string>{"1.1.1.1:443"}};
    std::promise<void> locked;
    std::promise<void> release;
    bool lock_timed_out{false};
    std::thread holder{[&] {
        LOCK(::cs_main);
        locked.set_value();
        lock_timed_out = release.get_future().wait_for(std::chrono::seconds{10}) == std::future_status::timeout;
    }};
    locked.get_future().wait();
    const auto error{node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                         /*ipv4_optional=*/false, /*optional=*/false)};
    release.set_value();
    holder.join();
    BOOST_CHECK(!lock_timed_out);
    BOOST_CHECK(!error);
}

BOOST_AUTO_TEST_CASE(providerNetInfoValidation)
{
    auto node{interfaces::MakeNode(m_node)};
    interfaces::ProviderNetInfo net_info{
        .core_p2p = {"1.1.1.1:9998"},
        .platform_p2p = std::vector<std::string>{"1.1.1.2:22200"},
        .platform_https = std::vector<std::string>{"server.example.com:443"},
    };

    // Unless the caller's capabilities say masternodes may operate without IPv4, Platform must use the Core address
    auto error{node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                   /*ipv4_optional=*/false, /*optional=*/false)};
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->reject_reason, "bad-protx-netinfo-addr");

    net_info.platform_p2p = std::vector<std::string>{"1.1.1.1:22200"};
    net_info.platform_https = std::vector<std::string>{"1.1.1.1:443", "server.example.com:443"};
    BOOST_CHECK(!node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                     /*ipv4_optional=*/false, /*optional=*/false));

    net_info.platform_p2p = std::vector<std::string>{"server.example.com:22200"};
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->code, interfaces::ProviderTxErrorCode::INVALID_PARAMETER);
    BOOST_CHECK_EQUAL(error->message.original,
                      "Error setting platformP2PAddrs[0] to 'server.example.com:22200' (invalid input)");

    net_info.platform_p2p = std::vector<std::string>{"1.1.1.1:9998"};
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->message.original, "Error setting platformP2PAddrs[0] to '1.1.1.1:9998' (duplicate)");

    net_info.platform_p2p = std::vector<std::string>{"1.1.1.2:22200", "1.1.1.3:22200", "1.1.1.4:22200", "1.1.1.5:22200",
                                                     "1.1.1.6:22200"};
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->message.original,
                      "Error setting platformP2PAddrs[4] to '1.1.1.6:22200' (too many entries)");

    net_info.platform_p2p = uint16_t{22200};
    net_info.platform_https = uint16_t{22201};
    BOOST_CHECK(!node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                     /*ipv4_optional=*/false, /*optional=*/false));

    net_info.platform_p2p = std::vector<std::string>{"1.1.1.2:22200"};
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(error);
    BOOST_CHECK(error->message.original.find("only accepts a bare port number") != std::string::npos);

    net_info = {
        .core_p2p = {},
        .platform_p2p = std::vector<std::string>{"1.1.1.2:22200"},
        .platform_https = std::vector<std::string>{"server.example.com:443"},
    };
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/true);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->reject_reason, "bad-protx-netinfo-empty");

    net_info = {
        .core_p2p = {"1.1.1.1:9998"},
        .platform_p2p = std::monostate{},
        .platform_https = std::monostate{},
    };
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/true);
    BOOST_REQUIRE(error);
    BOOST_CHECK(error->message.original.find("platformP2PAddrs, cannot be empty if other fields populated") !=
                std::string::npos);
    BOOST_CHECK(!node->evo().validateProviderNetInfo(net_info, MnType::Regular, ProTxVersion::ExtAddr,
                                                     /*ipv4_optional=*/false, /*optional=*/true));

    net_info.platform_p2p = std::vector<std::string>{"1.1.1.2:22200"};
    error = node->evo().validateProviderNetInfo(net_info, MnType::Regular, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/true);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->message.original, "Platform endpoints are only valid for an EvoNode");

    net_info = {
        .core_p2p = {"1.1.1.1:9998"},
        .platform_p2p = std::vector<std::string>{"1.1.1.2:22200"},
        .platform_https = std::monostate{},
    };
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::ExtAddr,
                                                /*ipv4_optional=*/false, /*optional=*/true);
    BOOST_REQUIRE(error);
    BOOST_CHECK(error->message.original.find("platformHTTPSAddrs, cannot be empty if other fields populated") !=
                std::string::npos);

    net_info = {
        .core_p2p = {"1.1.1.1:9998"},
        .platform_p2p = uint16_t{22200},
        .platform_https = uint16_t{22200},
    };
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->reject_reason, "bad-protx-platform-dup-ports");

    net_info.platform_p2p = uint16_t{MainParams().GetDefaultPort()};
    net_info.platform_https = uint16_t{22201};
    error = node->evo().validateProviderNetInfo(net_info, MnType::Evo, ProTxVersion::BasicBLS,
                                                /*ipv4_optional=*/false, /*optional=*/false);
    BOOST_REQUIRE(error);
    BOOST_CHECK_EQUAL(error->reject_reason, "bad-protx-platform-p2p-port");
}

BOOST_AUTO_TEST_CASE(findFirstBlockWithTimeAndHeight)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();
    uint256 hash;
    int height;
    BOOST_CHECK(chain->findFirstBlockWithTimeAndHeight(/* min_time= */ 0, /* min_height= */ 5, FoundBlock().hash(hash).height(height)));
    BOOST_CHECK_EQUAL(hash, active[5]->GetBlockHash());
    BOOST_CHECK_EQUAL(height, 5);
    BOOST_CHECK(!chain->findFirstBlockWithTimeAndHeight(/* min_time= */ active.Tip()->GetBlockTimeMax() + 1, /* min_height= */ 0));
}

BOOST_AUTO_TEST_CASE(findAncestorByHeight)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();
    uint256 hash;
    BOOST_CHECK(chain->findAncestorByHeight(active[20]->GetBlockHash(), 10, FoundBlock().hash(hash)));
    BOOST_CHECK_EQUAL(hash, active[10]->GetBlockHash());
    BOOST_CHECK(!chain->findAncestorByHeight(active[10]->GetBlockHash(), 20));
}

BOOST_AUTO_TEST_CASE(findAncestorByHash)
{
    LOCK(Assert(m_node.chainman)->GetMutex());
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();
    int height = -1;
    BOOST_CHECK(chain->findAncestorByHash(active[20]->GetBlockHash(), active[10]->GetBlockHash(), FoundBlock().height(height)));
    BOOST_CHECK_EQUAL(height, 10);
    BOOST_CHECK(!chain->findAncestorByHash(active[10]->GetBlockHash(), active[20]->GetBlockHash()));
}

BOOST_AUTO_TEST_CASE(findCommonAncestor)
{
    auto& chain = m_node.chain;
    const CChain& active = *WITH_LOCK(Assert(m_node.chainman)->GetMutex(), return &Assert(m_node.chainman)->ActiveChain());
    auto* orig_tip = active.Tip();
    for (int i = 0; i < 10; ++i) {
        BlockValidationState state;
        m_node.chainman->ActiveChainstate().InvalidateBlock(state, active.Tip());
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight - 10);
    coinbaseKey.MakeNewKey(true);
    for (int i = 0; i < 20; ++i) {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight + 10);
    uint256 fork_hash;
    int fork_height;
    int orig_height;
    BOOST_CHECK(chain->findCommonAncestor(orig_tip->GetBlockHash(), active.Tip()->GetBlockHash(), FoundBlock().height(fork_height).hash(fork_hash), FoundBlock().height(orig_height)));
    BOOST_CHECK_EQUAL(orig_height, orig_tip->nHeight);
    BOOST_CHECK_EQUAL(fork_height, orig_tip->nHeight - 10);
    BOOST_CHECK_EQUAL(fork_hash, active[fork_height]->GetBlockHash());

    uint256 active_hash, orig_hash;
    BOOST_CHECK(!chain->findCommonAncestor(active.Tip()->GetBlockHash(), {}, {}, FoundBlock().hash(active_hash), {}));
    BOOST_CHECK(!chain->findCommonAncestor({}, orig_tip->GetBlockHash(), {}, {}, FoundBlock().hash(orig_hash)));
    BOOST_CHECK_EQUAL(active_hash, active.Tip()->GetBlockHash());
    BOOST_CHECK_EQUAL(orig_hash, orig_tip->GetBlockHash());
}

BOOST_AUTO_TEST_CASE(hasBlocks)
{
    LOCK(::cs_main);
    auto& chain = m_node.chain;
    const CChain& active = Assert(m_node.chainman)->ActiveChain();

    // Test ranges
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[5]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[95]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));
    active[50]->nStatus &= ~BLOCK_HAVE_DATA;
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 10, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, 90));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 0, {}));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), -1000, 1000));

    // Test edge cases
    BOOST_CHECK(chain->hasBlocks(active.Tip()->GetBlockHash(), 6, 49));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 5, 49));
    BOOST_CHECK(!chain->hasBlocks(active.Tip()->GetBlockHash(), 6, 50));
}

// getPlatformQuorums exports the retained-key window of mined commitments with basic-scheme
// serialized public keys, newest-first. Mined commitments are installed directly in EvoDB (the
// same shortcut evo_cbtx_tests uses) since a real DKG cannot run in a unit test.
BOOST_AUTO_TEST_CASE(getPlatformQuorums)
{
    auto node{interfaces::MakeNode(m_node)};
    constexpr auto llmq_type{Consensus::LLMQType::LLMQ_TEST};
    const auto llmq_params{Params().GetLLMQ(llmq_type)};
    BOOST_REQUIRE(llmq_params.has_value());

    // The exported window is the retained-key window, which on LLMQ_TEST is strictly larger than
    // the active signing set: older quorums may still have signed consensus-valid Platform state
    // roots, so exporting only the active set would be a regression.
    const auto window{static_cast<size_t>(std::max(llmq_params->signingActiveQuorumCount, llmq_params->keepOldKeys))};
    BOOST_REQUIRE_EQUAL(window, 4U);
    BOOST_REQUIRE(window > static_cast<size_t>(llmq_params->signingActiveQuorumCount));

    // No commitments yet.
    BOOST_CHECK(node->llmq().getPlatformQuorums(std23::to_underlying(llmq_type)).empty());
    // Unconfigured LLMQ type.
    BOOST_CHECK(node->llmq().getPlatformQuorums(0xEE).empty());

    // One more valid commitment than the window can hold, quorum bases at heights 10..50.
    constexpr std::array<int, 5> base_heights{10, 20, 30, 40, 50};
    std::vector<llmq::CFinalCommitment> qcs;
    {
        LOCK(Assert(m_node.chainman)->GetMutex());
        const CChain& active{m_node.chainman->ActiveChain()};
        auto db_tx{m_node.evodb->BeginTransaction()};
        for (const int base : base_heights) {
            auto qc{llmq::testutils::CreateValidCommitment(*llmq_params, active[base]->GetBlockHash())};
            llmq::testutils::WriteMinedCommitment(*m_node.evodb, qc, active[base + 1]->GetBlockHash(),
                                                  /*mined_height=*/base + 1, /*quorum_height=*/base);
            qcs.push_back(qc);
        }
        db_tx->Commit();
    }

    const auto check_quorums = [&](const std::vector<interfaces::LLMQ::PlatformQuorum>& quorums,
                                   const std::vector<size_t>& expected_qc_indexes) {
        BOOST_REQUIRE_EQUAL(quorums.size(), expected_qc_indexes.size());
        for (size_t i{0}; i < quorums.size(); ++i) {
            const auto& qc{qcs[expected_qc_indexes[i]]};
            BOOST_CHECK_EQUAL(quorums[i].m_quorum_hash, qc.quorumHash);
            BOOST_CHECK_EQUAL(quorums[i].m_height, base_heights[expected_qc_indexes[i]]);
            // Exact basic-scheme serialization, regardless of the active global BLS scheme.
            BOOST_CHECK(quorums[i].m_pubkey == qc.quorumPublicKey.ToByteVector(/*specificLegacyScheme=*/false));
            CBLSPublicKey roundtrip;
            roundtrip.SetBytes(quorums[i].m_pubkey, /*specificLegacyScheme=*/false);
            BOOST_CHECK(roundtrip.IsValid());
            BOOST_CHECK(roundtrip == qc.quorumPublicKey);
        }
    };

    // Newest-first and truncated to the window: bases 50, 40, 30, 20; base 10 falls out.
    check_quorums(node->llmq().getPlatformQuorums(std23::to_underlying(llmq_type)), {4, 3, 2, 1});

    // A commitment whose public key is invalid consumes its window slot but exports nothing.
    {
        LOCK(Assert(m_node.chainman)->GetMutex());
        const CChain& active{m_node.chainman->ActiveChain()};
        auto qc{llmq::testutils::CreateValidCommitment(*llmq_params, active[60]->GetBlockHash())};
        qc.quorumPublicKey = CBLSPublicKey{};
        auto db_tx{m_node.evodb->BeginTransaction()};
        llmq::testutils::WriteMinedCommitment(*m_node.evodb, qc, active[61]->GetBlockHash(),
                                              /*mined_height=*/61, /*quorum_height=*/60);
        db_tx->Commit();
    }
    check_quorums(node->llmq().getPlatformQuorums(std23::to_underlying(llmq_type)), {4, 3, 2});
}

// getInstantSendLock returns the network-serialized islock for a locked txid, and an empty vector
// both for unknown txids and while InstantSend is disabled (spork gate inside the manager).
BOOST_AUTO_TEST_CASE(getInstantSendLock)
{
    constexpr const char* REGTEST_SPORK_PRIVKEY{"cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK"};

    auto node{interfaces::MakeNode(m_node)};
    auto& isman{*Assert(m_node.isman)};

    auto islock{std::make_shared<instantsend::InstantSendLock>()};
    islock->inputs.emplace_back(uint256::ONE, 0);
    islock->txid = uint256::TWO;
    islock->cycleHash = InsecureRand256();
    const bool legacy{bls::bls_legacy_scheme.load()};
    CBLSSecretKey sk;
    sk.MakeNewKey();
    islock->sig.Set(sk.Sign(islock->GetRequestId(), legacy), legacy);

    isman.WriteNewISLock(::SerializeHash(*islock), islock, /*minedHeight=*/std::nullopt);

    // SPORK_2 defaults to OFF: the stored lock must not leak through the interface.
    BOOST_CHECK(node->llmq().getInstantSendLock(islock->txid).empty());

    // The fixture builds a bare CSporkManager, so wire up the regtest signer before setting the spork.
    BOOST_REQUIRE(m_node.sporkman->SetSporkAddress(Params().SporkAddress()));
    BOOST_REQUIRE(m_node.sporkman->SetPrivKey(REGTEST_SPORK_PRIVKEY));
    BOOST_REQUIRE(m_node.sporkman->UpdateSpork(SPORK_2_INSTANTSEND_ENABLED, 0).has_value());

    CDataStream expected{SER_NETWORK, PROTOCOL_VERSION};
    expected << *islock;
    const auto ret{node->llmq().getInstantSendLock(islock->txid)};
    BOOST_REQUIRE(!ret.empty());
    BOOST_CHECK_EQUAL(HexStr(ret), HexStr(expected));

    // The exported bytes deserialize back into the same lock.
    CDataStream ds{Span<const uint8_t>{ret}, SER_NETWORK, PROTOCOL_VERSION};
    instantsend::InstantSendLock roundtrip;
    ds >> roundtrip;
    BOOST_CHECK_EQUAL(roundtrip.txid, islock->txid);
    BOOST_CHECK(roundtrip.inputs == islock->inputs);
    BOOST_CHECK_EQUAL(roundtrip.GetRequestId(), islock->GetRequestId());

    // Unknown txid.
    BOOST_CHECK(node->llmq().getInstantSendLock(uint256::ONE).empty());
}

BOOST_AUTO_TEST_SUITE_END()
