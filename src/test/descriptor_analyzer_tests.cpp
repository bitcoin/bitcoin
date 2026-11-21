// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <key_io.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <script/descriptor_analyzer.h>
#include <script/signingprovider.h>
#include <test/util/setup_common.h>
#include <util/bip32.h>

#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using Shape = DescriptorAnalysis::Shape;
using KeyPath = DescriptorAnalysis::KeyPath;
using IssueKind = DescriptorAnalysis::Issue::Kind;

namespace {

const std::string A{"xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL"};
const std::string B{"xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y"};
const std::string C{"xpub661MyMwAqRbcGDZQUKLqmWodYLcoBQnQH33yYkkF3jjxeLvY8qr2wWGEWkiKFaaQfJCoi3HeEq3Dc5DptfbCyjD38fNhSqtKc1UHaP4ba3t"};
const std::string X{"xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB"};
//! BIP 341 NUMS point H, as an x-only key and as an xpub.
const std::string NUMS{"50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0"};
const std::string NUMS_XPUB{"xpub661MyMwAqRbcEYS8w7XLSVeEsBXy79zSzH1J8vCdxAZningWLdN3zgtU6QgnecKFpJFPpdzxKrwoaZoV44qAJewsc4kX9vGaCaBExuvJH57"};
const std::string R{"/<0;1>/*"};

DescriptorAnalysis Analyze(const std::string& descriptor)
{
    FlatSigningProvider keys;
    std::string error;
    const auto descriptors{Parse(descriptor, keys, error)};
    BOOST_REQUIRE_MESSAGE(!descriptors.empty(), descriptor + ": " + error);
    return AnalyzeDescriptor(descriptors);
}

bool HasIssue(const DescriptorAnalysis& analysis, IssueKind kind)
{
    return std::ranges::any_of(analysis.issues, [&](const auto& issue) { return issue.kind == kind; });
}

//! An unrecognized result claims no setup.
void CheckNoSetup(const DescriptorAnalysis& analysis)
{
    BOOST_CHECK(!analysis.IsRecognized());
    BOOST_CHECK(!analysis.sorted);
    BOOST_CHECK(analysis.key_path == KeyPath::NONE);
    BOOST_CHECK_EQUAL(analysis.threshold, 0U);
    BOOST_CHECK(analysis.cosigners.empty());
    BOOST_CHECK(!analysis.issues.empty());
}

//! Same key as `xpub` derived at `child`, serialized as its own xpub.
std::string ChildXpub(const std::string& xpub, uint32_t child)
{
    CExtPubKey key{DecodeExtPubKey(xpub)};
    BOOST_REQUIRE(key.Derive(key, child));
    return EncodeExtPubKey(key);
}

//! `other` with metadata claiming it is the hardened child 0h of `parent`.
std::string AsHardenedChild(const std::string& other, const std::string& parent)
{
    CExtPubKey key{DecodeExtPubKey(other)};
    const CExtPubKey parent_key{DecodeExtPubKey(parent)};
    key.nDepth = parent_key.nDepth + 1;
    key.fingerprint = parent_key.id_key_fingerprint();
    key.nChild = BIP32_HARDENED_FLAG;
    return EncodeExtPubKey(key);
}

} // namespace

BOOST_FIXTURE_TEST_SUITE(descriptor_analyzer_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(recognized_wsh)
{
    const auto sorted{Analyze("wsh(sortedmulti(2,[aabbccdd/87h/0h/0h]" + A + R + "," + B + R + "," + C + R + "))")};
    BOOST_CHECK(sorted.IsRecognized());
    BOOST_CHECK(sorted.shape == Shape::WSH_MULTI);
    BOOST_CHECK(sorted.sorted);
    BOOST_CHECK(sorted.key_path == KeyPath::NONE);
    BOOST_CHECK_EQUAL(sorted.threshold, 2U);
    BOOST_CHECK_EQUAL(sorted.cosigners.size(), 3U);
    BOOST_CHECK(sorted.issues.empty());

    const auto unsorted{Analyze("wsh(multi(1," + A + "/0/*," + B + "/0/*))")};
    BOOST_CHECK(unsorted.IsRecognized());
    BOOST_CHECK(!unsorted.sorted);
    BOOST_CHECK_EQUAL(unsorted.threshold, 1U);
}

BOOST_AUTO_TEST_CASE(recognized_taproot)
{
    const std::string leaf{"sortedmulti_a(2," + A + R + "," + B + R + "," + C + R + ")"};

    // Key path: musig() of all cosigners, as built by the multisig wizard.
    const auto musig{Analyze("tr(musig(" + A + "," + B + "," + C + ")" + R + "," + leaf + ")")};
    BOOST_CHECK(musig.IsRecognized());
    BOOST_CHECK(musig.shape == Shape::TR_MULTI_A);
    BOOST_CHECK(musig.key_path == KeyPath::MUSIG);
    BOOST_CHECK_EQUAL(musig.threshold, 2U);
    BOOST_CHECK_EQUAL(musig.cosigners.size(), 3U);

    // The order of musig() participants doesn't matter.
    BOOST_CHECK(Analyze("tr(musig(" + C + "," + A + "," + B + ")" + R + "," + leaf + ")").key_path == KeyPath::MUSIG);

    // Any `threshold` cosigners can already spend through the leaf.
    BOOST_CHECK(Analyze("tr(musig(" + A + "," + B + ")" + R + "," + leaf + ")").IsRecognized());

    // Key path: NUMS point, as an x-only key or as an xpub.
    const auto nums{Analyze("tr(" + NUMS + "," + leaf + ")")};
    BOOST_CHECK(nums.IsRecognized());
    BOOST_CHECK(nums.key_path == KeyPath::NUMS);
    BOOST_CHECK(Analyze("tr(" + NUMS_XPUB + R + "," + leaf + ")").key_path == KeyPath::NUMS);

    BOOST_CHECK(!Analyze("tr(" + NUMS + ",multi_a(1," + A + "/0/*," + B + "/0/*))").sorted);
}

BOOST_AUTO_TEST_CASE(key_path_spendable)
{
    const std::string leaf{"sortedmulti_a(2," + A + R + "," + B + R + "," + C + R + ")"};
    const std::vector<std::string> internal_keys{
        A + R,                                   // one cosigner alone
        X + R,                                   // an outsider
        "musig(" + A + ")" + R,                  // fewer cosigners than the threshold
        "musig(" + A + "," + X + ")" + R,        // an outsider in the aggregate
    };
    for (const auto& internal : internal_keys) {
        const auto analysis{Analyze("tr(" + internal + "," + leaf + ")")};
        BOOST_TEST_CONTEXT(internal) CheckNoSetup(analysis);
        const auto issue{std::ranges::find(analysis.issues, IssueKind::KEY_PATH_SPENDABLE, &DescriptorAnalysis::Issue::kind)};
        BOOST_REQUIRE(issue != analysis.issues.end());
        // The internal key is the first key expression.
        BOOST_CHECK(issue->keys == std::vector<size_t>({0}));
    }
}

BOOST_AUTO_TEST_CASE(musig_participants)
{
    const std::string cosigners{A + R + "," + B + R + "," + C + R};
    const auto check = [](const DescriptorAnalysis& analysis, const std::vector<DescriptorAnalysis::Issue>& expected) {
        CheckNoSetup(analysis);
        BOOST_REQUIRE_EQUAL(analysis.issues.size(), expected.size());
        for (size_t i{0}; i < expected.size(); ++i) {
            BOOST_CHECK(analysis.issues[i].kind == expected[i].kind);
            BOOST_CHECK(analysis.issues[i].keys == expected[i].keys);
        }
    };

    // Only A's holder can derive the participant. The key path still needs
    // all three cosigners, so it isn't spendable by fewer.
    check(Analyze("tr(musig(" + A + "/0h," + B + "," + C + ")" + R + ",sortedmulti_a(2," + cosigners + "))"),
          {{IssueKind::HARDENED_DERIVATION, {1}}});

    // A repeated participant. A and B can already spend through a 2-of-3 leaf,
    // but not through a 3-of-3 one.
    check(Analyze("tr(musig(" + A + "," + A + "," + B + ")" + R + ",sortedmulti_a(2," + cosigners + "))"),
          {{IssueKind::REPEATED_KEY, {1, 2}}});
    check(Analyze("tr(musig(" + A + "," + A + "," + B + ")" + R + ",sortedmulti_a(3," + cosigners + "))"),
          {{IssueKind::REPEATED_KEY, {1, 2}}, {IssueKind::KEY_PATH_SPENDABLE, {0}}});
}

BOOST_AUTO_TEST_CASE(repeated_keys)
{
    // Same xpub in two slots: A alone meets the threshold.
    // It is not a 2-of-3, so no threshold is claimed.
    auto analysis{Analyze("wsh(sortedmulti(2," + A + "/0/*," + A + "/0/*," + C + "/0/*))")};
    CheckNoSetup(analysis);
    BOOST_REQUIRE_EQUAL(analysis.issues.size(), 1U);
    BOOST_CHECK(analysis.issues[0].kind == IssueKind::REPEATED_KEY);
    BOOST_CHECK(analysis.issues[0].keys == std::vector<size_t>({0, 1}));

    // Same xpub with different paths is still the same holder.
    BOOST_CHECK(HasIssue(Analyze("wsh(sortedmulti(2," + A + "/0/*," + A + "/1/*," + C + "/0/*))"), IssueKind::REPEATED_KEY));

    // An xpub and its own child xpub derive the same keys.
    BOOST_CHECK(HasIssue(Analyze("wsh(sortedmulti(2," + A + "/0/*," + ChildXpub(A, 0) + "/*," + C + "/0/*))"), IssueKind::REPEATED_KEY));

    // A direct child xpub is found even when the two slots derive different keys.
    analysis = Analyze("wsh(sortedmulti(2," + A + "/0/*," + ChildXpub(A, 0) + "/1/*," + C + "/0/*))");
    CheckNoSetup(analysis);
    BOOST_REQUIRE_EQUAL(analysis.issues.size(), 1U);
    BOOST_CHECK(analysis.issues[0].keys == std::vector<size_t>({0, 1}));

    // A hardened child can't be derived from the xpub, so it is not found,
    // even when its metadata names A as the parent.
    BOOST_CHECK(Analyze("wsh(sortedmulti(2," + A + "/0/*," + AsHardenedChild(B, A) + "/0/*," + C + "/0/*))").IsRecognized());

    // In the Taproot leaf too.
    analysis = Analyze("tr(" + NUMS + ",sortedmulti_a(2," + A + R + "," + A + R + "," + B + R + "))");
    CheckNoSetup(analysis);
    BOOST_CHECK(HasIssue(analysis, IssueKind::REPEATED_KEY));

    // Three slots, one holder: reported once, with all three.
    analysis = Analyze("wsh(sortedmulti(2," + A + "/0/*," + A + "/1/*," + A + "/2/*))");
    BOOST_REQUIRE_EQUAL(analysis.issues.size(), 1U);
    BOOST_CHECK_EQUAL(analysis.issues[0].keys.size(), 3U);

    // Slots linked through a chain are one group: the first and last share a
    // root, the last two derive the same key, the first two have nothing in common.
    analysis = Analyze("wsh(sortedmulti(2," + A + "/5/*," + ChildXpub(ChildXpub(A, 1), 2) + "/*," + A + "/1/2/*))");
    BOOST_REQUIRE_EQUAL(analysis.issues.size(), 1U);
    BOOST_CHECK(analysis.issues[0].keys == std::vector<size_t>({0, 1, 2}));
}

BOOST_AUTO_TEST_CASE(cosigner_keys)
{
    const auto check = [](const std::string& key, IssueKind kind) {
        const auto analysis{Analyze("wsh(sortedmulti(1," + key + "," + B + "/0/*))")};
        BOOST_TEST_CONTEXT(key) CheckNoSetup(analysis);
        BOOST_REQUIRE_EQUAL(analysis.issues.size(), 1U);
        BOOST_CHECK(analysis.issues[0].kind == kind);
        BOOST_CHECK(analysis.issues[0].keys == std::vector<size_t>({0}));
    };
    check("02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9", IssueKind::NOT_EXTENDED_KEY);
    check(A + "/0/5", IssueKind::NOT_RANGED);
    check(A, IssueKind::NOT_RANGED);
    check(A + "/0h/*", IssueKind::HARDENED_DERIVATION);
    check(A + "/0/*h", IssueKind::HARDENED_DERIVATION);
}

BOOST_AUTO_TEST_CASE(unsupported_shapes)
{
    for (const std::string& descriptor : {
             "pkh(" + A + "/0/*)",
             "sh(sortedmulti(1," + A + "/0/*," + B + "/0/*))",
             "sh(wsh(sortedmulti(1," + A + "/0/*," + B + "/0/*)))",
             "wsh(or_d(pk(" + A + "/0/*),and_v(v:pk(" + B + "/0/*),older(144))))",
             "tr(" + NUMS + ",{sortedmulti_a(1," + A + "/0/*," + B + "/0/*),pk(" + C + "/0/*)})",
             "tr(" + NUMS + ",and_v(v:pk(" + A + "/0/*),pk(" + B + "/0/*)))",
             "tr(" + A + "/0/*)",
         }) {
        const auto analysis{Analyze(descriptor)};
        BOOST_TEST_CONTEXT(descriptor) CheckNoSetup(analysis);
        BOOST_CHECK(HasIssue(analysis, IssueKind::UNSUPPORTED_SHAPE));
        // The structure is still returned for display.
        BOOST_CHECK(!analysis.policy.nodes.empty());
    }
}

BOOST_AUTO_TEST_CASE(multipath_branches)
{
    // Both branches are the same setup.
    BOOST_CHECK(Analyze("wsh(sortedmulti(2," + A + R + "," + B + R + "," + C + R + "))").IsRecognized());

    // The change branch has a hardened step: addresses can't be derived for it.
    // The receive branch alone is recognized, but the result claims no setup.
    const auto analysis{Analyze("wsh(sortedmulti(2," + A + "/<0;1h>/*," + B + R + "," + C + R + "))")};
    CheckNoSetup(analysis);
    BOOST_CHECK(HasIssue(analysis, IssueKind::BRANCHES_DIFFER));
}

BOOST_AUTO_TEST_SUITE_END()
