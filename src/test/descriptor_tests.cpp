// Copyright (c) 2018-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <string>
#include <script/sign.h>
#include <script/standard.h>
#include <test/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <script/descriptor.h>
#include <util/strencodings.h>

namespace {

void CheckUnparsable(const std::string& prv, const std::string& pub)
{
    FlatSigningProvider keys_priv, keys_pub;
    auto parse_priv = Parse(prv, keys_priv);
    auto parse_pub = Parse(pub, keys_pub);
    BOOST_CHECK_MESSAGE(!parse_priv, prv);
    BOOST_CHECK_MESSAGE(!parse_pub, pub);
}

constexpr int DEFAULT = 0;
constexpr int RANGE = 1; // Expected to be ranged descriptor
constexpr int HARDENED = 2; // Derivation needs access to private keys
constexpr int UNSOLVABLE = 4; // This descriptor is not expected to be solvable
constexpr int SIGNABLE = 8; // We can sign with this descriptor (this is not true when actual BIP32 derivation is used, as that's not integrated in our signing code)

/** Compare two descriptors. If only one of them has a checksum, the checksum is ignored. */
bool EqualDescriptor(std::string a, std::string b)
{
    bool a_check = (a.size() > 9 && a[a.size() - 9] == '#');
    bool b_check = (b.size() > 9 && b[b.size() - 9] == '#');
    if (a_check != b_check) {
        if (a_check) a = a.substr(0, a.size() - 9);
        if (b_check) b = b.substr(0, b.size() - 9);
    }
    return a == b;
}

std::string MaybeUseHInsteadOfApostrophy(std::string ret)
{
    if (InsecureRandBool()) {
        while (true) {
            auto it = ret.find("'");
            if (it != std::string::npos) {
                ret[it] = 'h';
                if (ret.size() > 9 && ret[ret.size() - 9] == '#') ret = ret.substr(0, ret.size() - 9); // Changing apostrophe to h breaks the checksum
            } else {
                break;
            }
        }
    }
    return ret;
}

const std::set<std::vector<uint32_t>> ONLY_EMPTY{{}};

void Check(const std::string& prv, const std::string& pub, int flags, const std::vector<std::vector<std::string>>& scripts, const std::set<std::vector<uint32_t>>& paths = ONLY_EMPTY)
{
    FlatSigningProvider keys_priv, keys_pub;
    std::set<std::vector<uint32_t>> left_paths = paths;

    // Check that parsing succeeds.
    auto parse_priv = Parse(MaybeUseHInsteadOfApostrophy(prv), keys_priv);
    auto parse_pub = Parse(MaybeUseHInsteadOfApostrophy(pub), keys_pub);
    BOOST_CHECK(parse_priv);
    BOOST_CHECK(parse_pub);

    // Check private keys are extracted from the private version but not the public one.
    BOOST_CHECK(keys_priv.keys.size());
    BOOST_CHECK(!keys_pub.keys.size());

    // Check that both versions serialize back to the public version.
    std::string pub1 = parse_priv->ToString();
    std::string pub2 = parse_pub->ToString();
    BOOST_CHECK(EqualDescriptor(pub, pub1));
    BOOST_CHECK(EqualDescriptor(pub, pub2));

    // Check that both can be serialized with private key back to the private version, but not without private key.
    std::string prv1;
    BOOST_CHECK(parse_priv->ToPrivateString(keys_priv, prv1));
    BOOST_CHECK(EqualDescriptor(prv, prv1));
    BOOST_CHECK(!parse_priv->ToPrivateString(keys_pub, prv1));
    BOOST_CHECK(parse_pub->ToPrivateString(keys_priv, prv1));
    BOOST_CHECK(EqualDescriptor(prv, prv1));
    BOOST_CHECK(!parse_pub->ToPrivateString(keys_pub, prv1));

    // Check whether IsRange on both returns the expected result
    BOOST_CHECK_EQUAL(parse_pub->IsRange(), (flags & RANGE) != 0);
    BOOST_CHECK_EQUAL(parse_priv->IsRange(), (flags & RANGE) != 0);

    // * For ranged descriptors,  the `scripts` parameter is a list of expected result outputs, for subsequent
    //   positions to evaluate the descriptors on (so the first element of `scripts` is for evaluating the
    //   descriptor at 0; the second at 1; and so on). To verify this, we evaluate the descriptors once for
    //   each element in `scripts`.
    // * For non-ranged descriptors, we evaluate the descriptors at positions 0, 1, and 2, but expect the
    //   same result in each case, namely the first element of `scripts`. Because of that, the size of
    //   `scripts` must be one in that case.
    if (!(flags & RANGE)) assert(scripts.size() == 1);
    size_t max = (flags & RANGE) ? scripts.size() : 3;

    // Iterate over the position we'll evaluate the descriptors in.
    for (size_t i = 0; i < max; ++i) {
        // Call the expected result scripts `ref`.
        const auto& ref = scripts[(flags & RANGE) ? i : 0];
        // When t=0, evaluate the `prv` descriptor; when t=1, evaluate the `pub` descriptor.
        for (int t = 0; t < 2; ++t) {
            // When the descriptor is hardened, evaluate with access to the private keys inside.
            const FlatSigningProvider& key_provider = (flags & HARDENED) ? keys_priv : keys_pub;

            // Evaluate the descriptor selected by `t` in poisition `i`.
            FlatSigningProvider script_provider, script_provider_cached;
            std::vector<CScript> spks, spks_cached;
            std::vector<unsigned char> cache;
            BOOST_CHECK((t ? parse_priv : parse_pub)->Expand(i, key_provider, spks, script_provider, &cache));

            // Compare the output with the expected result.
            BOOST_CHECK_EQUAL(spks.size(), ref.size());

            // Try to expand again using cached data, and compare.
            BOOST_CHECK(parse_pub->ExpandFromCache(i, cache, spks_cached, script_provider_cached));
            BOOST_CHECK(spks == spks_cached);
            BOOST_CHECK(script_provider.pubkeys == script_provider_cached.pubkeys);
            BOOST_CHECK(script_provider.scripts == script_provider_cached.scripts);
            BOOST_CHECK(script_provider.origins == script_provider_cached.origins);

            // For each of the produced scripts, verify solvability, and when possible, try to sign a transaction spending it.
            for (size_t n = 0; n < spks.size(); ++n) {
                BOOST_CHECK_EQUAL(ref[n], HexStr(spks[n].begin(), spks[n].end()));
                BOOST_CHECK_EQUAL(IsSolvable(Merge(key_provider, script_provider), spks[n]), (flags & UNSOLVABLE) == 0);

                if (flags & SIGNABLE) {
                    CMutableTransaction spend;
                    spend.vin.resize(1);
                    spend.vout.resize(1);
                    BOOST_CHECK_MESSAGE(SignSignature(Merge(keys_priv, script_provider), spks[n], spend, 0, 1, SIGHASH_ALL), prv);
                }

                /* Infer a descriptor from the generated script, and verify its solvability and that it roundtrips. */
                auto inferred = InferDescriptor(spks[n], script_provider);
                BOOST_CHECK_EQUAL(inferred->IsSolvable(), !(flags & UNSOLVABLE));
                std::vector<CScript> spks_inferred;
                FlatSigningProvider provider_inferred;
                BOOST_CHECK(inferred->Expand(0, provider_inferred, spks_inferred, provider_inferred));
                BOOST_CHECK_EQUAL(spks_inferred.size(), 1);
                BOOST_CHECK(spks_inferred[0] == spks[n]);
                BOOST_CHECK_EQUAL(IsSolvable(provider_inferred, spks_inferred[0]), !(flags & UNSOLVABLE));
                BOOST_CHECK(provider_inferred.origins == script_provider.origins);
            }

            // Test whether the observed key path is present in the 'paths' variable (which contains expected, unobserved paths),
            // and then remove it from that set.
            for (const auto& origin : script_provider.origins) {
                BOOST_CHECK_MESSAGE(paths.count(origin.second.second.path), "Unexpected key path: " + prv);
                left_paths.erase(origin.second.second.path);
            }
        }
    }

    // Verify no expected paths remain that were not observed.
    BOOST_CHECK_MESSAGE(left_paths.empty(), "Not all expected key paths found: " + prv);
}

}

BOOST_FIXTURE_TEST_SUITE(descriptor_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(descriptor_test)
{
    // Basic single-key compressed
    Check("combo(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)", "combo(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)", SIGNABLE, {{"2103acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859ac","76a914bdd026058d16836887af3500dd5c88823ff2218d88ac","0014bdd026058d16836887af3500dd5c88823ff2218d","a914c803d129359a688a885b4081dc57646972d3eb0987"}});
    Check("pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)", "pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)", SIGNABLE, {{"2103acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859ac"}});
    Check("pkh([deadbeef/1/2'/3/4']7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)", "pkh([deadbeef/1/2'/3/4']03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)", SIGNABLE, {{"76a914bdd026058d16836887af3500dd5c88823ff2218d88ac"}}, {{1,0x80000002UL,3,0x80000004UL}});
    Check("wpkh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)", "wpkh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)", SIGNABLE, {{"0014bdd026058d16836887af3500dd5c88823ff2218d"}});
    Check("sh(wpkh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "sh(wpkh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))", SIGNABLE, {{"a914c803d129359a688a885b4081dc57646972d3eb0987"}});

    // Basic single-key uncompressed
    Check("combo(2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN)", "combo(0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9)", SIGNABLE, {{"410473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9ac","76a914441ebabc7a1f1782a1359d3b70b8531670f8e36088ac"}});
    Check("pk(2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN)", "pk(0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9)", SIGNABLE, {{"410473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9ac"}});
    Check("pkh(2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN)", "pkh(0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9)", SIGNABLE, {{"76a914441ebabc7a1f1782a1359d3b70b8531670f8e36088ac"}});
    CheckUnparsable("wpkh(2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN)", "wpkh(0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9)"); // No uncompressed keys in witness
    CheckUnparsable("wsh(pk(2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN))", "wsh(pk(0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9))"); // No uncompressed keys in witness
    CheckUnparsable("sh(wpkh(2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN))", "sh(wpkh(0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9))"); // No uncompressed keys in witness

    // Some unconventional single-key constructions
    Check("sh(pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "sh(pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))", SIGNABLE, {{"a9146706565b906c64ea059fb952c9f2adad510e6f6e87"}});
    Check("sh(pkh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "sh(pkh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))", SIGNABLE, {{"a914dc09c48281c9c9fc6cc51d82d5613c5003d1e3c187"}});
    Check("wsh(pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "wsh(pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))", SIGNABLE, {{"0020c40d4130ca86a2926ed22e3f15df60d9ef731edaec98f79d2c6fb0d3e7cf636f"}});
    Check("wsh(pkh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "wsh(pkh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))", SIGNABLE, {{"0020f8d2d0373b890874b8a31931b4e3aba72ba06a22fb0a707b4a5b736e66300363"}});
    Check("sh(wsh(pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)))", "sh(wsh(pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)))", SIGNABLE, {{"a9141c0e5362d260c6d33231ef4ec0afdc71ba29d0c087"}});
    Check("sh(wsh(pkh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)))", "sh(wsh(pkh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)))", SIGNABLE, {{"a91485738a070d2300f946befb7c1472e9f7ac6c33b587"}});

    // Versions with BIP32 derivations
    Check("combo([01234567]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)", "combo([01234567]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)", SIGNABLE, {{"2102d2b36900396c9282fa14628566582f206a5dd0bcc8d5e892611806cafb0301f0ac","76a91431a507b815593dfc51ffc7245ae7e5aee304246e88ac","001431a507b815593dfc51ffc7245ae7e5aee304246e","a9142aafb926eb247cb18240a7f4c07983ad1f37922687"}});
    Check("pk(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0)", "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)", DEFAULT, {{"210379e45b3cf75f9c5f9befd8e9506fb962f6a9d185ac87001ec44a8d3df8d4a9e3ac"}}, {{0}});
    Check("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0)", HARDENED, {{"76a914ebdc90806a9c4356c1c88e42216611e1cb4c1c1788ac"}}, {{0xFFFFFFFFUL,0}});
    Check("wpkh([ffffffff/13']xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*)", "wpkh([ffffffff/13']xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*)", RANGE, {{"0014326b2249e3a25d5dc60935f044ee835d090ba859"},{"0014af0bd98abc2f2cae66e36896a39ffe2d32984fb7"},{"00141fa798efd1cbf95cebf912c031b8a4a6e9fb9f27"}}, {{0x8000000DUL, 1, 2, 0}, {0x8000000DUL, 1, 2, 1}, {0x8000000DUL, 1, 2, 2}});
    Check("sh(wpkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))", RANGE | HARDENED, {{"a9149a4d9901d6af519b2a23d4a2f51650fcba87ce7b87"},{"a914bed59fc0024fae941d6e20a3b44a109ae740129287"},{"a9148483aa1116eb9c05c482a72bada4b1db24af654387"}}, {{10, 20, 30, 40, 0x80000000UL}, {10, 20, 30, 40, 0x80000001UL}, {10, 20, 30, 40, 0x80000002UL}});
    Check("combo(xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334/*)", "combo(xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/*)", RANGE, {{"2102df12b7035bdac8e3bab862a3a83d06ea6b17b6753d52edecba9be46f5d09e076ac","76a914f90e3178ca25f2c808dc76624032d352fdbdfaf288ac","0014f90e3178ca25f2c808dc76624032d352fdbdfaf2","a91408f3ea8c68d4a7585bf9e8bda226723f70e445f087"},{"21032869a233c9adff9a994e4966e5b821fd5bac066da6c3112488dc52383b4a98ecac","76a914a8409d1b6dfb1ed2a3e8aa5e0ef2ff26b15b75b788ac","0014a8409d1b6dfb1ed2a3e8aa5e0ef2ff26b15b75b7","a91473e39884cb71ae4e5ac9739e9225026c99763e6687"}}, {{0}, {1}});
    CheckUnparsable("combo([012345678]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)", "combo([012345678]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)"); // Too long key fingerprint
    CheckUnparsable("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483648)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483648)"); // BIP 32 path element overflow

    // Multisig constructions
    Check("multi(1,7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118,2YfxyiDw9Simitvmf4zYfWyn4Kj1oRRZLiw8czmsNQNZoKQY9xN)", "multi(1,03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859,0473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a9)", SIGNABLE, {{"512103acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859410473fd51f3a89fa9487cff820b4a25a2819ba32d5f38f020cea69f516774007e89a552041d1c802f10bbfa56ab53c0d32bb57efbf158a662e4783c9d9dc8a6e5a952ae"}});
    Check("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}}, {{0x8000006FUL,222},{0}});
    Check("wsh(multi(2,xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))", HARDENED | RANGE, {{"0020b92623201f3bb7c3771d45b2ad1d0351ea8fbf8cfe0a0e570264e1075fa1948f"},{"002036a08bbe4923af41cf4316817c93b8d37e2f635dd25cfff06bd50df6ae7ea203"},{"0020a96e7ab4607ca6b261bfe3245ffda9c746b28d3f59e83d34820ec0e2b36c139c"}}, {{0xFFFFFFFFUL,0}, {1,2,0}, {1,2,1}, {1,2,2}, {10, 20, 30, 40, 0x80000000UL}, {10, 20, 30, 40, 0x80000001UL}, {10, 20, 30, 40, 0x80000002UL}});
    Check("sh(wsh(multi(16,7tj2WZdB4vwuSuCgrPVvJ6T7eP1sXnGB3ZcrpfxweLswUZdbHWo6,7qxZLiqPUatExeZpx3MtkPX7Z4RYBmdLZ8svkiQ6Xz3S36souWgH,7vbF19crZLCYfkxiMSHFtQPuHZgJMaVS1cyNp4EQzbYWGYMK4xEb,7qqFKgDqQTektEoNPvYY9evp4rCz2srtAvSggekXr7iZCZTT5tHE,7sFG9cN6pgfW6ssWYRYUv4c3Wmh7S52mYuidYV9yc5uXsz7b7ZL7,7sVQAjRgZfgr2uCUTr5PiUppsqU385zSj2wLpovgvPZDeSGW8cbp,7tHSQNPKwTfEKN5tFMJk4j66j2f1Mct1pdsm9WS6fcQtoProvpMK,7pvt3CaoDAfpQ6KkAhk1xodagHSWLFH8vDVk1Ztq645oPjRyZPZJ,7pqTmF65S9HTKeQ8vFbmoizJ2sshteRCMkmxrMM2G5RyLLCs5MXy,7qBMgugSAnZN2qAe949bUa7G22QLV3TKVpcCFEMsjPuAQfxfMi5d,7owxgxWqiUQ6FmyMStPCEBGqV9sqjp55mSSUjUQR1SYwLj23S1En,7tFmVyZ8pQQhHpnYjhGPiiXcgHN6faFtaGH3oMpEFXqpZCtSm3ke,7rHQPBG1y44aBhLSSjJdgopYXo9UDjMHWERqpLcrP716m6vQ4JA4,7qE1fySd2kTcW8wR4Hdwzc2qTnpbd5SA8pffqnp78C7UwwtQsokc,7uRhqaN2vuMYEtBd93YASCxFtmYct7xibsoD4mSw6bx2uWxcDseh,7qEB9zbfUWryhUUChCnv779K24DZaDT3sPS3NRpG1kM37hTjmA7B)))","sh(wsh(multi(16,02189afba2713a6effd9713eca50a55f41e40faba1e4e360a0390a42b8749a94b1,028aec35f6e17db217819b3602b317f9c34eefd1e180311b456032ea68d37d5d3d,03093e6f82dc3e421de3b7f6ed6efdbb172b536f65e642e7b7a7f27372f0a81659,031128bf8a17da69581d5247e2e7d5962115e0dcac73af7c0c3d02cb28e48880bd,0390cd0588ba2416feb24761f710c50ad40993bca3b30b252ea8c3f703a68f8650,02e3776dd22538507822b3a45f8fea342814b4b0afbfcca6203db228b459a34dc9,0220c33a1d300d80683177d897525616e06542b04f35819cb9cc8b7569dc28e96f,0214903afc3f9422f72b9bda7d8c3facff9ee4160531f361f50f18ea9483614489,026ec6d7c42410764ffcb3b1f8843f97da965edc2187e806fbd125df22feb51b87,02f57e9435ff61dc1f6809742b55ac06dc7eb1669effc0255c4783195ff6a6e072,0397cb209c64de58f2468d09ebf20bb7a4ead8e844e1eced35afc46f16989cc9de,032b4e05f21d40a9947302e88569d2c66c8e020a345cc865fe6083a7f17807f8aa,0316f1b196bb6d23ec97d2860b5f6aa2b5987b9626990136fab032366101a378c9,03e12c67b4f5ec5827cc69b962061aef523eb9e36821d691cd7d3303414dea5e5b,022513608dda7d7139d656b7a8d36367234909bdd5d31864237ba4cac746afb843,033b8dcbe009a1c9cc6efc74ca738dbbe2edde0750e04bc9cb660cdaf44b785016)))", SIGNABLE, {{"a9149470d464209f429d532c4481ca7e0f24643abd0e87"}});
    CheckUnparsable("sh(multi(16,7tj2WZdB4vwuSuCgrPVvJ6T7eP1sXnGB3ZcrpfxweLswUZdbHWo6,7qxZLiqPUatExeZpx3MtkPX7Z4RYBmdLZ8svkiQ6Xz3S36souWgH,7vbF19crZLCYfkxiMSHFtQPuHZgJMaVS1cyNp4EQzbYWGYMK4xEb,7qqFKgDqQTektEoNPvYY9evp4rCz2srtAvSggekXr7iZCZTT5tHE,7sFG9cN6pgfW6ssWYRYUv4c3Wmh7S52mYuidYV9yc5uXsz7b7ZL7,7sVQAjRgZfgr2uCUTr5PiUppsqU385zSj2wLpovgvPZDeSGW8cbp,7tHSQNPKwTfEKN5tFMJk4j66j2f1Mct1pdsm9WS6fcQtoProvpMK,7pvt3CaoDAfpQ6KkAhk1xodagHSWLFH8vDVk1Ztq645oPjRyZPZJ,7pqTmF65S9HTKeQ8vFbmoizJ2sshteRCMkmxrMM2G5RyLLCs5MXy,7qBMgugSAnZN2qAe949bUa7G22QLV3TKVpcCFEMsjPuAQfxfMi5d,7owxgxWqiUQ6FmyMStPCEBGqV9sqjp55mSSUjUQR1SYwLj23S1En,7tFmVyZ8pQQhHpnYjhGPiiXcgHN6faFtaGH3oMpEFXqpZCtSm3ke,7rHQPBG1y44aBhLSSjJdgopYXo9UDjMHWERqpLcrP716m6vQ4JA4,7qE1fySd2kTcW8wR4Hdwzc2qTnpbd5SA8pffqnp78C7UwwtQsokc,7uRhqaN2vuMYEtBd93YASCxFtmYct7xibsoD4mSw6bx2uWxcDseh,7qEB9zbfUWryhUUChCnv779K24DZaDT3sPS3NRpG1kM37hTjmA7B)))","sh(wsh(multi(16,02189afba2713a6effd9713eca50a55f41e40faba1e4e360a0390a42b8749a94b1,028aec35f6e17db217819b3602b317f9c34eefd1e180311b456032ea68d37d5d3d,03093e6f82dc3e421de3b7f6ed6efdbb172b536f65e642e7b7a7f27372f0a81659,031128bf8a17da69581d5247e2e7d5962115e0dcac73af7c0c3d02cb28e48880bd,0390cd0588ba2416feb24761f710c50ad40993bca3b30b252ea8c3f703a68f8650,02e3776dd22538507822b3a45f8fea342814b4b0afbfcca6203db228b459a34dc9,0220c33a1d300d80683177d897525616e06542b04f35819cb9cc8b7569dc28e96f,0214903afc3f9422f72b9bda7d8c3facff9ee4160531f361f50f18ea9483614489,026ec6d7c42410764ffcb3b1f8843f97da965edc2187e806fbd125df22feb51b87,02f57e9435ff61dc1f6809742b55ac06dc7eb1669effc0255c4783195ff6a6e072,0397cb209c64de58f2468d09ebf20bb7a4ead8e844e1eced35afc46f16989cc9de,032b4e05f21d40a9947302e88569d2c66c8e020a345cc865fe6083a7f17807f8aa,0316f1b196bb6d23ec97d2860b5f6aa2b5987b9626990136fab032366101a378c9,03e12c67b4f5ec5827cc69b962061aef523eb9e36821d691cd7d3303414dea5e5b,022513608dda7d7139d656b7a8d36367234909bdd5d31864237ba4cac746afb843,033b8dcbe009a1c9cc6efc74ca738dbbe2edde0750e04bc9cb660cdaf44b785016))"); // P2SH does not fit 16 compressed pubkeys in a redeemscript
    CheckUnparsable("wsh(multi(2,[aaaaaaaa][aaaaaaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaaa][aaaaaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))"); // Double key origin descriptor
    CheckUnparsable("wsh(multi(2,[aaaagaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaagaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))"); // Non hex fingerprint
    CheckUnparsable("wsh(multi(2,[aaaaaaaa],xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaaa],xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))"); // No public key with origin
    CheckUnparsable("wsh(multi(2,[aaaaaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))"); // Too short fingerprint
    CheckUnparsable("wsh(multi(2,[aaaaaaaaa]xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,[aaaaaaaaa]xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))"); // Too long fingerprint

    // Check for invalid nesting of structures
    CheckUnparsable("sh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)", "sh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)"); // P2SH needs a script, not a key
    CheckUnparsable("sh(combo(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "sh(combo(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))"); // Old must be top level
    CheckUnparsable("wsh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)", "wsh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)"); // P2WSH needs a script, not a key
    CheckUnparsable("wsh(wpkh(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118))", "wsh(wpkh(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859))"); // Cannot embed witness inside witness
    CheckUnparsable("wsh(sh(pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)))", "wsh(sh(pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)))"); // Cannot embed P2SH inside P2WSH
    CheckUnparsable("sh(sh(pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)))", "sh(sh(pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)))"); // Cannot embed P2SH inside P2SH
    CheckUnparsable("wsh(wsh(pk(7umPveWmiQadGQ2fjXjWLdCsafm2cxZgCXdHno28fZQJdxg3C118)))", "wsh(wsh(pk(03acb2c7783fd6c27b635ff3f182b9bceda07c1c8c6da7c71b70a6583f1b4b0859)))"); // Cannot embed P2WSH inside P2WSH

    // Checksums
    Check("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxfy", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5t", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}}, {{0x8000006FUL,222},{0}});
    Check("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}}, {{0x8000006FUL,222},{0}});
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#"); // Empty checksum
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxfyq", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5tq"); // Too long checksum
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxf", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5"); // Too short checksum
    CheckUnparsable("sh(multi(3,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggrsrxfy", "sh(multi(3,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjg09x5t"); // Error in payload
    CheckUnparsable("sh(multi(2,[00000000/111'/222]xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))#ggssrxfy", "sh(multi(2,[00000000/111'/222]xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))#tjq09x4t"); // Error in checksum
}

BOOST_AUTO_TEST_SUITE_END()
