// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <string>
#include <script/sign.h>
#include <script/standard.h>
#include <test/test_bitcoin.h>
#include <boost/test/unit_test.hpp>
#include <script/descriptor.h>
#include <utilstrencodings.h>

namespace {

void CheckUnparsable(const std::string& prv, const std::string& pub)
{
    FlatSigningProvider keys_priv, keys_pub;
    auto parse_priv = Parse(prv, keys_priv);
    auto parse_pub = Parse(pub, keys_pub);
    BOOST_CHECK(!parse_priv);
    BOOST_CHECK(!parse_pub);
}

constexpr int DEFAULT = 0;
constexpr int RANGE = 1; // Expected to be ranged descriptor
constexpr int HARDENED = 2; // Derivation needs access to private keys
constexpr int UNSOLVABLE = 4; // This descriptor is not expected to be solvable
constexpr int SIGNABLE = 8; // We can sign with this descriptor (this is not true when actual BIP32 derivation is used, as that's not integrated in our signing code)

std::string MaybeUseHInsteadOfApostrophy(std::string ret)
{
    if (InsecureRandBool()) {
        while (true) {
            auto it = ret.find("'");
            if (it != std::string::npos) {
                ret[it] = 'h';
            } else {
                break;
            }
        }
    }
    return ret;
}

void Check(const std::string& prv, const std::string& pub, int flags, const std::vector<std::vector<std::string>>& scripts)
{
    FlatSigningProvider keys_priv, keys_pub;

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
    std::string pub2 = parse_priv->ToString();
    BOOST_CHECK_EQUAL(pub, pub1);
    BOOST_CHECK_EQUAL(pub, pub2);

    // Check that both can be serialized with private key back to the private version, but not without private key.
    std::string prv1, prv2;
    BOOST_CHECK(parse_priv->ToPrivateString(keys_priv, prv1));
    BOOST_CHECK_EQUAL(prv, prv1);
    BOOST_CHECK(!parse_priv->ToPrivateString(keys_pub, prv1));
    BOOST_CHECK(parse_pub->ToPrivateString(keys_priv, prv1));
    BOOST_CHECK_EQUAL(prv, prv1);
    BOOST_CHECK(!parse_pub->ToPrivateString(keys_pub, prv1));

    // Check whether IsRange on both returns the expected result
    BOOST_CHECK_EQUAL(parse_pub->IsRange(), (flags & RANGE) != 0);
    BOOST_CHECK_EQUAL(parse_priv->IsRange(), (flags & RANGE) != 0);


    // Is not ranged descriptor, only a single result is expected.
    if (!(flags & RANGE)) assert(scripts.size() == 1);

    size_t max = (flags & RANGE) ? scripts.size() : 3;
    for (size_t i = 0; i < max; ++i) {
        const auto& ref = scripts[(flags & RANGE) ? i : 0];
        for (int t = 0; t < 2; ++t) {
            FlatSigningProvider key_provider = (flags & HARDENED) ? keys_priv : keys_pub;
            FlatSigningProvider script_provider;
            std::vector<CScript> spks;
            BOOST_CHECK((t ? parse_priv : parse_pub)->Expand(i, key_provider, spks, script_provider));
            BOOST_CHECK_EQUAL(spks.size(), ref.size());
            for (size_t n = 0; n < spks.size(); ++n) {
                BOOST_CHECK_EQUAL(ref[n], HexStr(spks[n].begin(), spks[n].end()));
                BOOST_CHECK_EQUAL(IsSolvable(Merge(key_provider, script_provider), spks[n]), (flags & UNSOLVABLE) == 0);

                if (flags & SIGNABLE) {
                    CMutableTransaction spend;
                    spend.vin.resize(1);
                    spend.vout.resize(1);
                    BOOST_CHECK_MESSAGE(SignSignature(Merge(keys_priv, script_provider), spks[n], spend, 0, 1, SIGHASH_ALL), prv);
                }
            }

        }
    }
}

}

BOOST_FIXTURE_TEST_SUITE(descriptor_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(descriptor_test)
{
    // Basic single-key compressed
    Check("combo(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)", "combo(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)", SIGNABLE, {{"2102a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cac","76a9140c13eca58177d726d65b8556f351d081b1103bf088ac","00140c13eca58177d726d65b8556f351d081b1103bf0","a9147bdb0f7aa46efb97e699cef65234d80481fab37287"}});
    Check("pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)", "pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)", SIGNABLE, {{"2102a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cac"}});
    Check("pkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)", "pkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)", SIGNABLE, {{"76a9140c13eca58177d726d65b8556f351d081b1103bf088ac"}});
    Check("wpkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)", "wpkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)", SIGNABLE, {{"00140c13eca58177d726d65b8556f351d081b1103bf0"}});
    Check("sh(wpkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "sh(wpkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))", SIGNABLE, {{"a9147bdb0f7aa46efb97e699cef65234d80481fab37287"}});

    // Basic single-key uncompressed
    Check("combo(6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S)", "combo(04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762)", SIGNABLE, {{"4104a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762ac","76a91419c13841a49884bd124fed9f4e036ec2462403fd88ac"}});
    Check("pk(6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S)", "pk(04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762)", SIGNABLE, {{"4104a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762ac"}});
    Check("pkh(6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S)", "pkh(04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762)", SIGNABLE, {{"76a91419c13841a49884bd124fed9f4e036ec2462403fd88ac"}});
    CheckUnparsable("wpkh(6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S)", "wpkh(04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762)"); // No uncompressed keys in witness
    CheckUnparsable("wsh(pk(6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S))", "wsh(pk(04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762))"); // No uncompressed keys in witness
    CheckUnparsable("sh(wpkh(6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S))", "sh(wpkh(04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762))"); // No uncompressed keys in witness
    
    // Some unconventional single-key constructions
    Check("sh(pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "sh(pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))", SIGNABLE, {{"a914de92af19990cd11b293afd89b9d1985fa0b8166f87"}});
    Check("sh(pkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "sh(pkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))", SIGNABLE, {{"a91401c62701a59dea1cf6d8b834a994f77575df782487"}});
    Check("wsh(pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "wsh(pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))", SIGNABLE, {{"0020f971e2b302ee0977ddb08d4150e50acabda3f53478fbd474a866b616ce041868"}});
    Check("wsh(pkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "wsh(pkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))", SIGNABLE, {{"002099dba00720686a1275aae85494e4eafc1744ba11d4eb6059e83e383e49fe7274"}});
    Check("sh(wsh(pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)))", "sh(wsh(pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)))", SIGNABLE, {{"a914bd45baff235b772370d6265d268b1ddb08917b0e87"}});
    Check("sh(wsh(pkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)))", "sh(wsh(pkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)))", SIGNABLE, {{"a914c36470aa22c4026cf75d377d3611f86225f7676c87"}});
    
    // Versions with BIP32 derivations  
    Check("combo(xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc)", "combo(xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL)", SIGNABLE, {{"2102d2b36900396c9282fa14628566582f206a5dd0bcc8d5e892611806cafb0301f0ac","76a91431a507b815593dfc51ffc7245ae7e5aee304246e88ac","001431a507b815593dfc51ffc7245ae7e5aee304246e","a9142aafb926eb247cb18240a7f4c07983ad1f37922687"}});
    Check("pk(xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0)", "pk(xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0)", DEFAULT, {{"210379e45b3cf75f9c5f9befd8e9506fb962f6a9d185ac87001ec44a8d3df8d4a9e3ac"}});
    Check("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0)", HARDENED, {{"76a914ebdc90806a9c4356c1c88e42216611e1cb4c1c1788ac"}});
    Check("wpkh(xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*)", "wpkh(xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*)", RANGE, {{"0014326b2249e3a25d5dc60935f044ee835d090ba859"},{"0014af0bd98abc2f2cae66e36896a39ffe2d32984fb7"},{"00141fa798efd1cbf95cebf912c031b8a4a6e9fb9f27"}});
    Check("sh(wpkh(xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "sh(wpkh(xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))", RANGE | HARDENED, {{"a9149a4d9901d6af519b2a23d4a2f51650fcba87ce7b87"},{"a914bed59fc0024fae941d6e20a3b44a109ae740129287"},{"a9148483aa1116eb9c05c482a72bada4b1db24af654387"}});
    Check("combo(xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334/*)", "combo(xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV/*)", RANGE, {{"2102df12b7035bdac8e3bab862a3a83d06ea6b17b6753d52edecba9be46f5d09e076ac","76a914f90e3178ca25f2c808dc76624032d352fdbdfaf288ac","0014f90e3178ca25f2c808dc76624032d352fdbdfaf2","a91408f3ea8c68d4a7585bf9e8bda226723f70e445f087"},{"21032869a233c9adff9a994e4966e5b821fd5bac066da6c3112488dc52383b4a98ecac","76a914a8409d1b6dfb1ed2a3e8aa5e0ef2ff26b15b75b788ac","0014a8409d1b6dfb1ed2a3e8aa5e0ef2ff26b15b75b7","a91473e39884cb71ae4e5ac9739e9225026c99763e6687"}});
    CheckUnparsable("pkh(xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483648)", "pkh(xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483648)"); // BIP 32 path element overflow

    // Multisig constructions  
    Check("multi(1,T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte,6uWuTLr7s7iTXhh3semxLf3w4BxzpXbwzbrYzHENHCbeWiqiD9S)", "multi(1,02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c,04a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c1762)", SIGNABLE, {{"512102a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c4104a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8cf2858b37e9ef7162964cfaf304f859350af59745166f1a9345916c7ba43c176252ae"}});
    Check("sh(multi(2,xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc,xprv9uPDJpEQgRQfDcW7BkF7eTya6RPxXeJCqCJGHuCJ4GiRVLzkTXBAJMu2qaMWPrS7AANYqdq6vcBcBUdJCVVFceUvJFjaPdGZ2y9WACViL4L/0))", "sh(multi(2,xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL,xpub68NZiKmJWnxxS6aaHmn81bvJeTESw724CRDs6HbuccFQN9Ku14VQrADWgqbhhTHBaohPX4CjNLf9fq9MYo6oDaPPLPxSb7gwQN3ih19Zm4Y/0))", DEFAULT, {{"a91445a9a622a8b0a1269944be477640eedc447bbd8487"}});
    Check("wsh(multi(2,xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U/2147483647'/0,xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt/1/2/*,xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi/10/20/30/40/*'))", "wsh(multi(2,xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB/2147483647'/0,xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH/1/2/*,xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8/10/20/30/40/*'))", HARDENED | RANGE, {{"0020b92623201f3bb7c3771d45b2ad1d0351ea8fbf8cfe0a0e570264e1075fa1948f"},{"002036a08bbe4923af41cf4316817c93b8d37e2f635dd25cfff06bd50df6ae7ea203"},{"0020a96e7ab4607ca6b261bfe3245ffda9c746b28d3f59e83d34820ec0e2b36c139c"}});
    Check("sh(wsh(multi(16,TAf9F3b4zyzkypAZdxuANwTK7rQ1zZ1UANzLEoBmGpD62KFBya11,T6561bd1Rb3SmcPBAXuA1eBxjELeuJieSFtGAHCSxRkNcgQPm7z7,T6i9NKx85hkDcDJ5YMmZrhNreHaAcN56WrtzyHyxVsko1zbJRBdk,T3VcqHoFsbA6PAFtDS6P8LLWcmjWRduhEowHAMwaaVSCijzhQbQR,TBLPRzGYQ1xvN3rT7g9jvcmzQpXuRpFkRAaEQvVT1t7HqLwTFEB5,T8FCx3oLynpPkvMr1RKBJSEZCzkMudDrDCpGkFumK6S9MR5EzdPd,TAxZhPGg88xt3f7j9ZAppJvYQ8hek26Y3kijx7n13MCE9po6cK8s,TB2UddAb7JyPpXC6fathUhdchoSywt9C2uaEqMLaLr4dm3PQVVUi,T51c6PHXCYSA4kvLiDG6PWiRWKJFDwTMt1KRh4KhUyHUAT94ugYD,T6VnK1h7jrVmKiLuBwvh5mYzeP3PcgwTxxGTgpB96j1KeE6Qv4x2,T3qLuf78utL2TJMCgdhFCpqHozDN3zVMkS6e2FMyPPhbawG7KnWL,T6qqDEyaHaNbLRmeyQY9X4hyC4G3fMdxdyAG23XwiRRbyHpXkHDk,TA6EN8Lm7m7FDVcBxiaRBMkyd4wK4MNi4JWhWtktLAJVubRLC125,T6uo9ne56G3aDCYRk3DpTnZTph2QPKhEVKecSAEykHdDdz75akQ2,TBZ73tU5RYaVf6wyWLiXLhmF5Vcdzcy15KUnCXi8LjThaT3aPAod,T6iHVFsP9PHWMqkZNHjqGH5ZdySXBB6qtFVayy6dsnZkzYEPA1tZ)))","sh(wsh(multi(16,028efd34c41e28bb2cfea70611f81d05f10fc38f74f1293f290892f97cb5ebc2e2,02955a0848ac4f4d6a9335bae723652931959aff5c98352bd1c4fc7a1952f30820,035d767016855ea58a94ab5d1dd90f47396b647b31e31a130bada62e156c0a0602,02b34c46466f57a967850e4ab99b483990430011a4c618163cfbf1874a423fd91f,020bec74d47d3ff6de0dde6120bbe3f1df0d473ccf4f5b4d58b95d93a5f51d9ca0,036a2f831e3f57dfe5b1b096139e0d9d63cbfda585327658d9008969473a39e27e,028a06fe1b6521aa1dad38e0bfb5db526e9fbd5dcbd96629a05767116b763980de,03536e727875e0c63b993512d65941aeff9ec41aff2185c2623d959f03ff52a1a1,03e9027817347cb801baf6c94cdc884778f4a6c49de093bf9b9af057a62c29285f,025e0d8b108cd7cf32bbcb3f634823e713d39ccf55c88d9f2348e91e9e47e4b8ff,03ea735c72c0d0d398f23cd8e2ed9055b876a02cba8af809475c36ffc7159740ca,0289286273cb1da95c75b5ac64a39b293ec83e2c20b36b65318318885222cf4f42,026949f2ae270ac8795a232727daf4fb87808e280e4628ffa7c999425730ba2a23,02ff70b460fcc3c3ca88df3a408c5bd41a8114782699e543e8c5140be9366f5503,03fdd51fda82b2b2e5d5b4f8757810f071f1c6569b55ee18b73db168c22d6fa4e4,02a2d3cd71d0e342eabae548becd25256d508031c23a6fdfc39183c8b4dab9d611)))", SIGNABLE, {{"a914110bf0af71c7ab0f383d0fd89f13518a539c8aeb87"}});
    CheckUnparsable("sh(multi(16,TAf9F3b4zyzkypAZdxuANwTK7rQ1zZ1UANzLEoBmGpD62KFBya11,T6561bd1Rb3SmcPBAXuA1eBxjELeuJieSFtGAHCSxRkNcgQPm7z7,T6i9NKx85hkDcDJ5YMmZrhNreHaAcN56WrtzyHyxVsko1zbJRBdk,T3VcqHoFsbA6PAFtDS6P8LLWcmjWRduhEowHAMwaaVSCijzhQbQR,TBLPRzGYQ1xvN3rT7g9jvcmzQpXuRpFkRAaEQvVT1t7HqLwTFEB5,T8FCx3oLynpPkvMr1RKBJSEZCzkMudDrDCpGkFumK6S9MR5EzdPd,TAxZhPGg88xt3f7j9ZAppJvYQ8hek26Y3kijx7n13MCE9po6cK8s,TB2UddAb7JyPpXC6fathUhdchoSywt9C2uaEqMLaLr4dm3PQVVUi,T51c6PHXCYSA4kvLiDG6PWiRWKJFDwTMt1KRh4KhUyHUAT94ugYD,T6VnK1h7jrVmKiLuBwvh5mYzeP3PcgwTxxGTgpB96j1KeE6Qv4x2,T3qLuf78utL2TJMCgdhFCpqHozDN3zVMkS6e2FMyPPhbawG7KnWLs,T6qqDEyaHaNbLRmeyQY9X4hyC4G3fMdxdyAG23XwiRRbyHpXkHDk,TA6EN8Lm7m7FDVcBxiaRBMkyd4wK4MNi4JWhWtktLAJVubRLC125,T6uo9ne56G3aDCYRk3DpTnZTph2QPKhEVKecSAEykHdDdz75akQ2,TBZ73tU5RYaVf6wyWLiXLhmF5Vcdzcy15KUnCXi8LjThaT3aPAod,T6iHVFsP9PHWMqkZNHjqGH5ZdySXBB6qtFVayy6dsnZkzYEPA1tZ))","sh(multi(16,028efd34c41e28bb2cfea70611f81d05f10fc38f74f1293f290892f97cb5ebc2e2,02955a0848ac4f4d6a9335bae723652931959aff5c98352bd1c4fc7a1952f30820,035d767016855ea58a94ab5d1dd90f47396b647b31e31a130bada62e156c0a0602,02b34c46466f57a967850e4ab99b483990430011a4c618163cfbf1874a423fd91f,020bec74d47d3ff6de0dde6120bbe3f1df0d473ccf4f5b4d58b95d93a5f51d9ca0,036a2f831e3f57dfe5b1b096139e0d9d63cbfda585327658d9008969473a39e27e,028a06fe1b6521aa1dad38e0bfb5db526e9fbd5dcbd96629a05767116b763980de,03536e727875e0c63b993512d65941aeff9ec41aff2185c2623d959f03ff52a1a1,03e9027817347cb801baf6c94cdc884778f4a6c49de093bf9b9af057a62c29285f,025e0d8b108cd7cf32bbcb3f634823e713d39ccf55c88d9f2348e91e9e47e4b8ff,03ea735c72c0d0d398f23cd8e2ed9055b876a02cba8af809475c36ffc7159740ca,0289286273cb1da95c75b5ac64a39b293ec83e2c20b36b65318318885222cf4f42,026949f2ae270ac8795a232727daf4fb87808e280e4628ffa7c999425730ba2a23,02ff70b460fcc3c3ca88df3a408c5bd41a8114782699e543e8c5140be9366f5503,03fdd51fda82b2b2e5d5b4f8757810f071f1c6569b55ee18b73db168c22d6fa4e4,02a2d3cd71d0e342eabae548becd25256d508031c23a6fdfc39183c8b4dab9d611))"); // P2SH does not fit 16 compressed pubkeys in a redeemscript
    
    // Check for invalid nesting of structures
    CheckUnparsable("sh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)", "sh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)"); // P2SH needs a script, not a key
    CheckUnparsable("sh(combo(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "sh(combo(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))"); // Old must be top level
    CheckUnparsable("wsh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)", "wsh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)"); // P2WSH needs a script, not a key
    CheckUnparsable("wsh(wpkh(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte))", "wsh(wpkh(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c))"); // Cannot embed witness inside witness
    CheckUnparsable("wsh(sh(pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)))", "wsh(sh(pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)))"); // Cannot embed P2SH inside P2WSH
    CheckUnparsable("sh(sh(pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)))", "sh(sh(pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)))"); // Cannot embed P2SH inside P2SH
    CheckUnparsable("wsh(wsh(pk(T4nzXGboJCdz6WbmgZjRFkwwb5QACn5FjEqiBpdzvWBva3PMLwte)))", "wsh(wsh(pk(02a5e85e848c4107607d7b8522018d6dffa6b40aad853ae634f95cded666dd1d8c)))"); // Cannot embed P2WSH inside P2WSH
}

BOOST_AUTO_TEST_SUITE_END()
