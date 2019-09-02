// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <string>

#include <test/setup_common.h>
#include <boost/test/unit_test.hpp>

#include <hash.h>
#include <pubkey.h>
#include <uint256.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <script/miniscript.h>

namespace {

/** TestData groups various kinds of precomputed data necessary in this test. */
struct TestData {
    //! The only public keys used in this test.
    std::vector<CPubKey> pubkeys;
    //! A map from the public keys to their CKeyIDs (faster than hashing every time).
    std::map<CPubKey, CKeyID> pkhashes;
    std::map<CKeyID, CPubKey> pkmap;

    // Various precomputed hashes
    std::vector<std::vector<unsigned char>> sha256;
    std::vector<std::vector<unsigned char>> ripemd160;
    std::vector<std::vector<unsigned char>> hash256;
    std::vector<std::vector<unsigned char>> hash160;

    TestData()
    {
        // We generate 255 public keys and 255 hashes of each type.
        for (int i = 1; i <= 255; ++i) {
            // This 32-byte array functions as both private key data and hash preimage (31 zero bytes plus any nonzero byte).
            unsigned char keydata[32] = {0};
            keydata[31] = i;

            // Compute CPubkey and CKeyID
            CKey key;
            key.Set(keydata, keydata + 32, true);
            CPubKey pubkey = key.GetPubKey();
            CKeyID keyid = pubkey.GetID();
            pubkeys.push_back(pubkey);
            pkhashes.emplace(pubkey, keyid);
            pkmap.emplace(keyid, pubkey);

            // Compute various hashes
            std::vector<unsigned char> hash;
            hash.resize(32);
            CSHA256().Write(keydata, 32).Finalize(hash.data());
            sha256.push_back(hash);
            CHash256().Write(keydata, 32).Finalize(hash.data());
            hash256.push_back(hash);
            hash.resize(20);
            CRIPEMD160().Write(keydata, 32).Finalize(hash.data());
            ripemd160.push_back(hash);
            CHash160().Write(keydata, 32).Finalize(hash.data());
            hash160.push_back(hash);
        }
    }
};

//! Global TestData object
std::unique_ptr<const TestData> g_testdata;

/** A class encapulating conversion routing for CPubKey. */
struct KeyConverter {
    typedef CPubKey Key;

    //! Public keys in text form are their usual hex notation (no xpubs, ...).
    bool ToString(const CPubKey& key, std::string& ret) const { ret = HexStr(key.begin(), key.end()); return true; }

    //! Convert a public key to bytes.
    std::vector<unsigned char> ToPKBytes(const CPubKey& key) const { return {key.begin(), key.end()}; }

    //! Convert a public key to its Hash160 bytes (precomputed).
    std::vector<unsigned char> ToPKHBytes(const CPubKey& key) const
    {
        auto it = g_testdata->pkhashes.find(key);
        assert(it != g_testdata->pkhashes.end());
        return {it->second.begin(), it->second.end()};
    }

    //! Parse a public key from a range of hex characters.
    template<typename I>
    bool FromString(I first, I last, CPubKey& key) const {
        auto bytes = ParseHex(std::string(first, last));
        key.Set(bytes.begin(), bytes.end());
        return key.IsValid();
    }

    template<typename I>
    bool FromPKBytes(I first, I last, CPubKey& key) const {
        key.Set(first, last);
        return key.IsValid();
    }

    template<typename I>
    bool FromPKHBytes(I first, I last, CPubKey& key) const {
        assert(last - first == 20);
        CKeyID keyid;
        std::copy(first, last, keyid.begin());
        auto it = g_testdata->pkmap.find(keyid);
        assert(it != g_testdata->pkmap.end());
        key = it->second;
        return true;
    }
};

//! Singleton instance of KeyConverter.
const KeyConverter CONVERTER{};

// Helper types and functions that use miniscript instantiated for CPubKey.
using NodeType = miniscript::NodeType;
using NodeRef = miniscript::NodeRef<CPubKey>;
template<typename... Args> NodeRef MakeNodeRef(Args&&... args) { return miniscript::MakeNodeRef<CPubKey>(std::forward<Args>(args)...); }
using miniscript::operator"" _mst;

NodeRef GenNode(miniscript::Type typ, int complexity);

//! Generate a random valid miniscript node of the given type and complexity.
NodeRef RandomNode(miniscript::Type typ, int complexity) {
    assert(complexity > 0);
    NodeRef ret;
    do {
        ret = GenNode(typ, complexity);
    } while (!ret || !ret->IsValid() || !(ret->GetType() << typ));
    return ret;
}

//! Generate a vector of valid miniscript nodes of the given types, and a specified complexity of their sum.
std::vector<NodeRef> MultiNode(int complexity, const std::vector<miniscript::Type>& types)
{
    int nodes = types.size();
    assert(complexity >= nodes);
    std::vector<int> subcomplex(nodes, 1);
    if (nodes == 1) {
        subcomplex[0] = complexity;
    } else {
        // This is a silly inefficient way to construct a multinomial distribution.
        for (int i = 0; i < complexity - nodes; ++i) {
            subcomplex[InsecureRandRange(nodes)]++;
        }
    }
    std::vector<NodeRef> subs;
    for (int i = 0; i < nodes; ++i) {
        subs.push_back(RandomNode(types[i], subcomplex[i]));
    }
    return subs;
}

//! Generate a random (but occasionally invalid) miniscript node of the given type and complexity.
NodeRef GenNode(miniscript::Type typ, int complexity) {
    if (typ << "B"_mst) {
        // Generate a "B" node.
        if (complexity == 1) {
            switch (InsecureRandBits(2)) {
                case 0: return MakeNodeRef(InsecureRandBool() ? NodeType::JUST_0 : NodeType::JUST_1);
                case 1: return MakeNodeRef(InsecureRandBool() ? NodeType::OLDER : NodeType::AFTER, 1 + InsecureRandRange((1ULL << (1 + InsecureRandRange(31))) - 1));
                case 2: {
                    int hashtype = InsecureRandBits(2);
                    int index = InsecureRandRange(255);
                    switch (hashtype) {
                        case 0: return MakeNodeRef(NodeType::SHA256, g_testdata->sha256[index]);
                        case 1: return MakeNodeRef(NodeType::RIPEMD160, g_testdata->ripemd160[index]);
                        case 2: return MakeNodeRef(NodeType::HASH256, g_testdata->hash256[index]);
                        case 3: return MakeNodeRef(NodeType::HASH160, g_testdata->hash160[index]);
                    }
                    break;
                }
                case 3: return MakeNodeRef(NodeType::WRAP_C, MultiNode(complexity, Vector("K"_mst)));
            }
            assert(false);
        }
        switch (InsecureRandRange(7 + (complexity >= 3) * 7 + (complexity >= 4) * 2)) {
            // Complexity >= 2
            case 0: return MakeNodeRef(NodeType::WRAP_C, MultiNode(complexity, Vector("K"_mst)));
            case 1: return MakeNodeRef(NodeType::WRAP_D, MultiNode(complexity - 1, Vector("V"_mst)));
            case 2: return MakeNodeRef(NodeType::WRAP_J, MultiNode(complexity - 1, Vector("B"_mst)));
            case 3: return MakeNodeRef(NodeType::WRAP_N, MultiNode(complexity - 1, Vector("B"_mst)));
            case 4: return MakeNodeRef(NodeType::OR_I, Cat(MultiNode(complexity - 1, Vector("B"_mst)), Vector(MakeNodeRef(NodeType::JUST_0))));
            case 5: return MakeNodeRef(NodeType::OR_I, Cat(Vector(MakeNodeRef(NodeType::JUST_0)), MultiNode(complexity - 1, Vector("B"_mst))));
            case 6: return MakeNodeRef(NodeType::AND_V, Cat(MultiNode(complexity - 1, Vector("V"_mst)), Vector(MakeNodeRef(NodeType::JUST_1))));
            // Complexity >= 3
            case 7: return MakeNodeRef(NodeType::AND_V, MultiNode(complexity - 1, Vector("V"_mst, "B"_mst)));
            case 8: return MakeNodeRef(NodeType::ANDOR, Cat(MultiNode(complexity - 1, Vector("B"_mst, "B"_mst)), Vector(MakeNodeRef(NodeType::JUST_0))));
            case 9: return MakeNodeRef(NodeType::AND_B, MultiNode(complexity - 1, Vector("B"_mst, "W"_mst)));
            case 10: return MakeNodeRef(NodeType::OR_B, MultiNode(complexity - 1, Vector("B"_mst, "W"_mst)));
            case 11: return MakeNodeRef(NodeType::OR_D, MultiNode(complexity - 1, Vector("B"_mst, "B"_mst)));
            case 12: return MakeNodeRef(NodeType::OR_I, MultiNode(complexity - 1, Vector("B"_mst, "B"_mst)));
            case 13: {
                if (complexity != 3) return {};
                int nkeys = 1 + (InsecureRandRange(15) * InsecureRandRange(25)) / 17;
                int sigs = 1 + InsecureRandRange(nkeys);
                std::vector<CPubKey> keys;
                for (int i = 0; i < nkeys; ++i) keys.push_back(g_testdata->pubkeys[InsecureRandRange(255)]);
                return MakeNodeRef(NodeType::THRESH_M, std::move(keys), sigs);
            }
            // Complexity >= 4
            case 14: return MakeNodeRef(NodeType::ANDOR, MultiNode(complexity - 1, Vector("B"_mst, "B"_mst, "B"_mst)));
            case 15: {
                int args = 3 + InsecureRandRange(std::min(3, complexity - 3));
                int sats = 2 + InsecureRandRange(args - 2);
                return MakeNodeRef(NodeType::THRESH, MultiNode(complexity - 1, Cat(Vector("B"_mst), std::vector<miniscript::Type>(args - 1, "W"_mst))), sats);
            }
        }
    } else if (typ << "V"_mst) {
        // Generate a "V" node.
        switch (InsecureRandRange(1 + (complexity >= 3) * 3 + (complexity >= 4))) {
            // Complexity >= 1
            case 0: return MakeNodeRef(NodeType::WRAP_V, MultiNode(complexity, Vector("B"_mst)));
            // Complexity >= 3
            case 1: return MakeNodeRef(NodeType::AND_V, MultiNode(complexity - 1, Vector("V"_mst, "V"_mst)));
            case 2: return MakeNodeRef(NodeType::OR_C, MultiNode(complexity - 1, Vector("B"_mst, "V"_mst)));
            case 3: return MakeNodeRef(NodeType::OR_I, MultiNode(complexity - 1, Vector("V"_mst, "V"_mst)));
            // Complexity >= 4
            case 4: return MakeNodeRef(NodeType::ANDOR, MultiNode(complexity - 1, Vector("B"_mst, "V"_mst, "V"_mst)));
        }
    } else if (typ << "W"_mst) {
        // Generate a "W" node by wrapping a "B" node.
        auto sub = RandomNode("B"_mst, complexity);
        if (sub->GetType() << "o"_mst) {
            if (InsecureRandBool()) return MakeNodeRef(NodeType::WRAP_S, Vector(std::move(sub)));
        }
        return MakeNodeRef(NodeType::WRAP_A, Vector(std::move(sub)));
    } else if (typ << "K"_mst) {
        // Generate a "K" node.
        if (complexity == 1 || complexity == 2) {
            if (InsecureRandBool()) {
                return MakeNodeRef(NodeType::PK, Vector(g_testdata->pubkeys[InsecureRandRange(255)]));
            } else {
                return MakeNodeRef(NodeType::PK_H, Vector(g_testdata->pubkeys[InsecureRandRange(255)]));
            }
        }
        switch (InsecureRandRange(2 + (complexity >= 4))) {
            // Complexity >= 3
            case 0: return MakeNodeRef(NodeType::AND_V, MultiNode(complexity - 1, Vector("V"_mst, "K"_mst)));
            case 1: return MakeNodeRef(NodeType::OR_I, MultiNode(complexity - 1, Vector("K"_mst, "K"_mst)));
            // Complexity >= 4
            case 2: return MakeNodeRef(NodeType::ANDOR, MultiNode(complexity - 1, Vector("B"_mst, "K"_mst, "K"_mst)));
        }
    }
    assert(false);
    return {};
}

enum TestMode : int {
    TESTMODE_INVALID = 0,
    TESTMODE_VALID = 1,
    TESTMODE_NONMAL = 2,
    TESTMODE_NEEDSIG = 4
};

void Test(const std::string& ms, const std::string& hexscript, int mode)
{
    auto node = miniscript::FromString(ms, CONVERTER);
    if (mode == TESTMODE_INVALID) {
        BOOST_CHECK_MESSAGE(!node || !node->IsValid(), "Unexpectedly valid: " + ms);
    } else {
        BOOST_CHECK_MESSAGE(node, "Unparseable: " + ms);
        BOOST_CHECK_MESSAGE(node->IsValid(), "Invalid: " + ms);
        BOOST_CHECK_MESSAGE(node->IsValidTopLevel(), "Invalid top level: " + ms);
        auto computed_script = node->ToScript(CONVERTER);
        BOOST_CHECK_MESSAGE(node->ScriptSize() == computed_script.size(), "Script size mismatch: " + ms);
        if (hexscript != "?") BOOST_CHECK_MESSAGE(HexStr(computed_script) == hexscript, "Script mismatch: " + ms + " (" + HexStr(computed_script) + " vs " + hexscript + ")");
        BOOST_CHECK_MESSAGE(node->IsNonMalleable() == !!(mode & TESTMODE_NONMAL), "Malleability mismatch: " + ms);
        BOOST_CHECK_MESSAGE(node->NeedsSignature() == !!(mode & TESTMODE_NEEDSIG), "Signature necessity mismatch: " + ms);
        auto inferred_miniscript = miniscript::FromScript(computed_script, CONVERTER);
        BOOST_CHECK_MESSAGE(inferred_miniscript, "Cannot infer miniscript from script: " + ms);
        BOOST_CHECK_MESSAGE(inferred_miniscript->ToScript(CONVERTER) == computed_script, "Roundtrip failure: miniscript->script != miniscript->script->miniscript->script: " + ms);
    }

}

} // namespace

BOOST_FIXTURE_TEST_SUITE(miniscript_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(fixed_tests)
{
    g_testdata.reset(new TestData());

    // Randomly generated test set that covers the majority of type and node type combinations
    Test("lltvln:after(1231488000)", "6300676300676300670400046749b1926869516868", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("uuj:and_v(v:thresh_m(2,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a,025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc),after(1231488000))", "6363829263522103d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a21025601570cb47f238d2b0286db4a990fa0f3ba28d1a319f5e7cf55c2a2444da7cc52af0400046749b168670068670068", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);
    Test("or_b(un:thresh_m(2,03daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729,024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97),al:older(16))", "63522103daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee872921024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c9752ae926700686b63006760b2686c9b", TESTMODE_VALID);
    Test("j:and_v(vdv:after(1567547623),older(2016))", "829263766304e7e06e5db169686902e007b268", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("t:and_v(vu:hash256(131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b),v:sha256(ec4916dd28fc4c10d78e287ca5d9cc51ee1ae73cbfde08c6b37324cbfaac8bc5))", "6382012088aa20131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b876700686982012088a820ec4916dd28fc4c10d78e287ca5d9cc51ee1ae73cbfde08c6b37324cbfaac8bc58851", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("t:andor(thresh_m(3,02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e,03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556,02e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13),v:older(4194305),v:sha256(9267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2))", "532102d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e2103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a14602975562102e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd1353ae6482012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2886703010040b2696851", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("or_d(thresh_m(1,02f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f9),or_b(thresh_m(3,022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01,032fa2104d6b38d11b0230010559879124e42ab8dfeff5ff29dc9cdadd4ecacc3f,03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a),su:after(500000)))", "512102f9308a019258c31049344f85f89d5229b531c845836f99b08601f113bce036f951ae73645321022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a0121032fa2104d6b38d11b0230010559879124e42ab8dfeff5ff29dc9cdadd4ecacc3f2103d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a53ae7c630320a107b16700689b68", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("or_d(sha256(38df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b6),and_n(un:after(499999999),older(4194305)))", "82012088a82038df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b68773646304ff64cd1db19267006864006703010040b26868", TESTMODE_VALID);
    Test("and_v(or_i(v:thresh_m(2,02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5,03774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb),v:thresh_m(2,03e60fce93b59e9ec53011aabc21c23e97b2a31369b87a5ae9c44ee89e2a6dec0a,025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc)),sha256(d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c68))", "63522102c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee52103774ae7f858a9411e5ef4246b70c65aac5649980be5c17891bbec17895da008cb52af67522103e60fce93b59e9ec53011aabc21c23e97b2a31369b87a5ae9c44ee89e2a6dec0a21025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc52af6882012088a820d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c6887", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);
    Test("j:and_b(thresh_m(2,0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798,024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97),s:or_i(older(1),older(4252898)))", "82926352210279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f8179821024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c9752ae7c6351b26703e2e440b2689a68", TESTMODE_VALID | TESTMODE_NEEDSIG);
    Test("and_b(older(16),s:or_d(sha256(e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f),n:after(1567547623)))", "60b27c82012088a820e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f87736404e7e06e5db192689a", TESTMODE_VALID);
    Test("j:and_v(v:hash160(20195b5a3d650c17f0f29f91c33f8f6335193d07),or_d(sha256(96de8fc8c256fa1e1556d41af431cace7dca68707c78dd88c3acab8b17164c47),older(16)))", "82926382012088a91420195b5a3d650c17f0f29f91c33f8f6335193d078882012088a82096de8fc8c256fa1e1556d41af431cace7dca68707c78dd88c3acab8b17164c4787736460b26868", TESTMODE_VALID);
    Test("and_b(hash256(32ba476771d01e37807990ead8719f08af494723de1d228f2c2c07cc0aa40bac),a:and_b(hash256(131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b),a:older(1)))", "82012088aa2032ba476771d01e37807990ead8719f08af494723de1d228f2c2c07cc0aa40bac876b82012088aa20131772552c01444cd81360818376a040b7c3b2b7b0a53550ee3edde216cec61b876b51b26c9a6c9a", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("thresh(2,thresh_m(2,03a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c7,036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00),a:thresh_m(1,036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00),ac:pk(022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01))", "522103a0434d9e47f3c86235477c7b1ae6ae5d3442d49b1943c2b752a68e2a47e247c721036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a0052ae6b5121036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a0051ae6c936b21022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01ac6c935287", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);
    Test("and_n(sha256(d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c68),t:or_i(v:older(4252898),v:older(144)))", "82012088a820d1ec675902ef1633427ca360b290b0b3045a0d9058ddb5e648b4c3c3224c5c68876400676303e2e440b26967029000b269685168", TESTMODE_VALID);
    Test("or_d(d:and_v(v:older(4252898),v:older(4252898)),sha256(38df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b6))", "766303e2e440b26903e2e440b26968736482012088a82038df1c1f64a24a77b23393bca50dff872e31edc4f3b5aa3b90ad0b82f4f089b68768", TESTMODE_VALID);
    Test("c:and_v(or_c(sha256(9267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2),v:thresh_m(1,02c44d12c7065d812e8acf28d7cbb19f9011ecd9e9fdf281b0e6a3b5e87d22e7db)),pk(03acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbe))", "82012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed28764512102c44d12c7065d812e8acf28d7cbb19f9011ecd9e9fdf281b0e6a3b5e87d22e7db51af682103acd484e2f0c7f65309ad178a9f559abde09796974c57e714c35f110dfc27ccbeac", TESTMODE_VALID | TESTMODE_NEEDSIG);
    Test("c:and_v(or_c(thresh_m(2,036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a00,02352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d5),v:ripemd160(1b0f3c404d12075c68c938f9f60ebea4f74941a0)),pk(03fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556))", "5221036d2b085e9e382ed10b69fc311a03f8641ccfff21574de0927513a49d9a688a002102352bbf4a4cdd12564f93fa332ce333301d9ad40271f8107181340aef25be59d552ae6482012088a6141b0f3c404d12075c68c938f9f60ebea4f74941a088682103fff97bd5755eeea420453a14355235d382f6472f8568a18b2f057a1460297556ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);
    Test("and_v(andor(hash256(8a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b25),v:hash256(939894f70e6c3a25da75da0cc2071b4076d9b006563cf635986ada2e93c0d735),v:older(50000)),after(499999999))", "82012088aa208a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b2587640350c300b2696782012088aa20939894f70e6c3a25da75da0cc2071b4076d9b006563cf635986ada2e93c0d735886804ff64cd1db1", TESTMODE_VALID);
    Test("andor(hash256(5f8d30e655a7ba0d7596bb3ddfb1d2d20390d23b1845000e1e118b3be1b3f040),j:and_v(v:hash160(3a2bff0da9d96868e66abc4427bea4691cf61ccd),older(4194305)),ripemd160(44d90e2d3714c8663b632fcf0f9d5f22192cc4c8))", "82012088aa205f8d30e655a7ba0d7596bb3ddfb1d2d20390d23b1845000e1e118b3be1b3f040876482012088a61444d90e2d3714c8663b632fcf0f9d5f22192cc4c8876782926382012088a9143a2bff0da9d96868e66abc4427bea4691cf61ccd8803010040b26868", TESTMODE_VALID);
    Test("or_i(c:and_v(v:after(500000),pk(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)),sha256(d9147961436944f43cd99d28b2bbddbf452ef872b30c8279e255e7daafc7f946))", "630320a107b1692102c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5ac6782012088a820d9147961436944f43cd99d28b2bbddbf452ef872b30c8279e255e7daafc7f9468768", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("thresh(2,c:pk_h(025cbdf0646e5db4eaa398f365f2ea7a0e3d419b7e0330e39ce92bddedcac4f9bc),s:sha256(e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f),a:hash160(dd69735817e0e3f6f826a9238dc2e291184f0131))", "76a9145dedfbf9ea599dd4e3ca6a80b333c472fd0b3f6988ac7c82012088a820e38990d0c7fc009880a9c07c23842e886c6bbdc964ce6bdd5817ad357335ee6f87936b82012088a914dd69735817e0e3f6f826a9238dc2e291184f0131876c935287", TESTMODE_VALID);
    Test("and_n(sha256(9267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed2),uc:and_v(v:older(144),pk(03fe72c435413d33d48ac09c9161ba8b09683215439d62b7940502bda8b202e6ce)))", "82012088a8209267d3dbed802941483f1afa2a6bc68de5f653128aca9bf1461c5d0a3ad36ed28764006763029000b2692103fe72c435413d33d48ac09c9161ba8b09683215439d62b7940502bda8b202e6ceac67006868", TESTMODE_VALID | TESTMODE_NEEDSIG);
    Test("and_n(c:pk(03daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729),and_b(l:older(4252898),a:older(16)))", "2103daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729ac64006763006703e2e440b2686b60b26c9a68", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);
    Test("c:or_i(and_v(v:older(16),pk_h(02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e)),pk_h(026a245bf6dc698504c89a20cfded60853152b695336c28063b61c65cbd269e6b4))", "6360b26976a9149fc5dbe5efdce10374a4dd4053c93af540211718886776a9142fbd32c8dd59ee7c17e66cb6ebea7e9846c3040f8868ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);
    Test("or_d(c:pk_h(02e493dbf1c10d80f3581e4904930b1404cc6c13900ee0758474fa94abe8c4cd13),andor(c:pk(024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97),older(2016),after(1567547623)))", "76a914c42e7ef92fdb603af844d064faad95db9bcdfd3d88ac736421024ce119c96e2fa357200b559b2f7dd5a5f02d5290aff74b03f3e471b273211c97ac6404e7e06e5db16702e007b26868", TESTMODE_VALID | TESTMODE_NONMAL);
    Test("c:andor(ripemd160(6ad07d21fd5dfc646f0b30577045ce201616b9ba),pk_h(02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e),and_v(v:hash256(8a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b25),pk_h(03d01115d548e7561b15c38f004d734633687cf4419620095bc5b0f47070afe85a)))", "82012088a6146ad07d21fd5dfc646f0b30577045ce201616b9ba876482012088aa208a35d9ca92a48eaade6f53a64985e9e2afeb74dcf8acb4c3721e0dc7e4294b258876a914dd100be7d9aea5721158ebde6d6a1fd8fff93bb1886776a9149fc5dbe5efdce10374a4dd4053c93af5402117188868ac", TESTMODE_VALID | TESTMODE_NEEDSIG);
    Test("c:andor(u:ripemd160(6ad07d21fd5dfc646f0b30577045ce201616b9ba),pk_h(03daed4f2be3a8bf278e70132fb0beb7522f570e144bf615c07e996d443dee8729),or_i(pk_h(022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01),pk_h(0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798)))", "6382012088a6146ad07d21fd5dfc646f0b30577045ce201616b9ba87670068646376a9149652d86bedf43ad264362e6e6eba6eb764508127886776a914751e76e8199196d454941c45d1b3a323f1433bd688686776a91420d637c1a6404d2227f3561fdbaff5a680dba6488868ac", TESTMODE_VALID | TESTMODE_NEEDSIG);
    Test("c:or_i(andor(c:pk_h(03d30199d74fb5a22d47b6e054e2f378cedacffcb89904a61d75d0dbd407143e65),pk_h(022f01e5e15cca351daff3843fb70f3c2f0a1bdd05e5af888a67784ef3e10a2a01),pk_h(02c6047f9441ed7d6d3045406e95c07cd85c778e4b8cef3ca7abac09b95c709ee5)),pk(02d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e))", "6376a914fcd35ddacad9f2d5be5e464639441c6065e6955d88ac6476a91406afd46bcdfd22ef94ac122aa11f241244a37ecc886776a9149652d86bedf43ad264362e6e6eba6eb7645081278868672102d7924d4f7d43ea965a465ae3095ff41131e5946f3c85f79e44adbcf8e27e080e68ac", TESTMODE_VALID | TESTMODE_NONMAL | TESTMODE_NEEDSIG);

    g_testdata.reset();
}

BOOST_AUTO_TEST_CASE(random_tests)
{
    // Initialize precomputed data.
    g_testdata.reset(new TestData());

    for (int i = 0; i < 1000; ++i) {
        bool safe = InsecureRandRange(20) == 0; // In 5% of the cases, generate safe top-level expressions.
        // Generate a random B (or Bms) node of variable complexity, which should be valid as a top-level expression.
        auto node = RandomNode(safe ? "Bms"_mst : "B"_mst, 1 + InsecureRandRange(90));
        BOOST_CHECK(node && node->IsValid() && node->IsValidTopLevel());
        auto script = node->ToScript(CONVERTER);
        BOOST_CHECK(node->ScriptSize() == script.size()); // Check consistency between script size estimation and real size
        // Check consistency of "x" property with the script (relying on the fact that no top-level scripts end with a hash or key push, whose last byte could match these opcodes).
        bool ends_in_verify = !(node->GetType() << "x"_mst);
        BOOST_CHECK(ends_in_verify == (script.back() == OP_CHECKSIG || script.back() == OP_CHECKMULTISIG || script.back() == OP_EQUAL));
        std::string str;
        BOOST_CHECK(node->ToString(CONVERTER, str)); // Check that we can convert to text
        auto parsed = miniscript::FromString(str, CONVERTER);
        BOOST_CHECK(parsed); // Check that we can convert back
        BOOST_CHECK(*parsed == *node); // Check that it matches the original
        auto decoded = miniscript::FromScript(script, CONVERTER);
        BOOST_CHECK(decoded); // Check that we can decode the miniscript back from the script.
        // Check that it matches the original (we can't use *decoded == *node because the miniscript representation may differ)
        BOOST_CHECK(decoded->ToScript(CONVERTER) == script); // The script corresponding to that decoded form must match exactly.
        BOOST_CHECK(decoded->GetType() == node->GetType()); // The type also has to match exactly.
    }

    g_testdata.reset();
}

BOOST_AUTO_TEST_SUITE_END()
