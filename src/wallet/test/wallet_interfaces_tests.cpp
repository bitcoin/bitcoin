// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <interfaces/wallet.h>
#include <key.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/descriptor.h>
#include <script/signingprovider.h>
#include <test/util/setup_common.h>
#include <uint256.h>
#include <util/bip32.h>
#include <util/strencodings.h>
#include <wallet/context.h>
#include <wallet/descriptor_analyzer.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

#include <boost/test/unit_test.hpp>
#include <optional>
#include <string>
#include <vector>

namespace wallet {
namespace {

using KeyMatch = WalletDescriptorAnalysis::KeyMatch;

CExtKey NewHDKey()
{
    CExtKey key;
    key.SetSeed(GenerateRandomKey());
    return key;
}

//! `[fingerprint/path]xpub` for the key at `path` below `master`.
std::string OriginKey(const CExtKey& master, const std::vector<uint32_t>& path)
{
    const auto child{Assert(DeriveExtKey(master, path))};
    return "[" + HexStr(child->second.fingerprint) + FormatHDKeypath(child->second.path) + "]" + EncodeExtPubKey(child->first.Neuter());
}

//! `[fingerprint/48h/1h/0h/2h]xpub`: a cosigner key as exported for a multisig setup.
std::string AccountKey(const CExtKey& master)
{
    return OriginKey(master, {0x80000030, 0x80000001, 0x80000000, 0x80000002});
}

//! A private key whose public key has an odd Y.
CKey OddKey()
{
    CKey key;
    do {
        key = GenerateRandomKey();
    } while (key.GetPubKey()[0] != 0x03);
    return key;
}

void AddDescriptor(CWallet& wallet, const std::string& descriptor)
{
    LOCK(wallet.cs_wallet);
    FlatSigningProvider provider;
    std::string error;
    auto descs{Parse(descriptor, provider, error, /*require_checksum=*/false)};
    BOOST_REQUIRE_MESSAGE(descs.size() == 1, error);
    WalletDescriptor w_desc(std::move(descs.at(0)), /*creation_time=*/0, /*range_start=*/0, /*range_end=*/1, /*next_index=*/1);
    BOOST_REQUIRE(wallet.AddWalletDescriptor(w_desc, provider, /*label=*/"", /*internal=*/false));
}

const std::string R{"/<0;1>/*"};

} // namespace

BOOST_FIXTURE_TEST_SUITE(wallet_interfaces_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(addhdkey)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    auto result = interface->addHDKey(std::nullopt);
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->pubkey.IsValid());
}

BOOST_AUTO_TEST_CASE(addhdkey_with_key)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    CKey seed_key = GenerateRandomKey();
    CExtKey key;
    key.SetSeed(seed_key);

    auto result = interface->addHDKey(key);
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->pubkey.IsValid());
    BOOST_CHECK_EQUAL(EncodeExtPubKey(*result), EncodeExtPubKey(key.Neuter()));
}

BOOST_AUTO_TEST_CASE(addhdkey_with_malformed_key)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    CExtKey key;
    key.SetSeed(GenerateRandomKey());
    key.nChild = 1;

    auto result = interface->addHDKey(key);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().code == WalletErrorCode::GenericError);
    BOOST_CHECK_EQUAL(result.error().message.original, "Invalid HD key");
}

BOOST_AUTO_TEST_CASE(addhdkey_wallet_locked)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    BOOST_REQUIRE(wallet->EncryptWallet("hunter2"));
    BOOST_REQUIRE(wallet->Lock());

    auto interface = interfaces::MakeWallet(context, wallet);
    auto result = interface->addHDKey(std::nullopt);
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().code == WalletErrorCode::UnlockNeeded);
}

BOOST_AUTO_TEST_CASE(analyzedescriptor)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);
    const CExtKey master{NewHDKey()};
    BOOST_REQUIRE(interface->addHDKey(master));

    // A 2-of-3 with this wallet's key, derived from its HD key, in the second slot.
    const std::string mine{AccountKey(master)}, b{AccountKey(NewHDKey())}, c{AccountKey(NewHDKey())};
    auto result{interface->analyzeDescriptor("wsh(sortedmulti(2," + b + R + "," + mine + R + "," + c + R + "))")};
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->analysis.IsRecognized());
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::MINE, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == true);
    BOOST_CHECK(!result->has_private_keys);

    // The first addresses are index 0 of the receive and change paths.
    const auto first_script = [&](const std::string& path) {
        FlatSigningProvider provider, out;
        std::string error;
        std::vector<CScript> scripts;
        const auto descs{Parse("wsh(sortedmulti(2," + b + path + "," + mine + path + "," + c + path + "))", provider, error)};
        BOOST_REQUIRE(descs.at(0)->Expand(/*pos=*/0, provider, scripts, out));
        return scripts.at(0);
    };
    BOOST_REQUIRE_EQUAL(result->first_addresses.size(), 2U);
    BOOST_CHECK(GetScriptForDestination(result->first_addresses[0]) == first_script("/0/*"));
    BOOST_CHECK(GetScriptForDestination(result->first_addresses[1]) == first_script("/1/*"));

    // A different change path is the same setup, with the same receive address,
    // but a different change address.
    const auto other_change{interface->analyzeDescriptor("wsh(sortedmulti(2," + b + R + "," + mine + R + "," + c + "/<0;2>/*))")};
    BOOST_REQUIRE(other_change);
    BOOST_CHECK(other_change->analysis.IsRecognized());
    BOOST_REQUIRE_EQUAL(other_change->first_addresses.size(), 2U);
    BOOST_CHECK(other_change->first_addresses[0] == result->first_addresses[0]);
    BOOST_CHECK(other_change->first_addresses[1] != result->first_addresses[1]);

    // The wallet's HD key itself, without an origin.
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + EncodeExtPubKey(master.Neuter()) + "/0/*," + b + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::NOT_MINE}));
    // Its depth, parent fingerprint and child number don't matter, but its chain code does.
    CExtPubKey relabeled{master.Neuter()};
    relabeled.nDepth = 3;
    relabeled.fingerprint = NewHDKey().Neuter().id_key_fingerprint();
    relabeled.nChild = 7;
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + EncodeExtPubKey(relabeled) + "/0/*," + b + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::NOT_MINE}));
    CExtPubKey other_chaincode{master.Neuter()};
    other_chaincode.chaincode = ChainCode{uint256::ONE};
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + EncodeExtPubKey(other_chaincode) + "/0/*," + b + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::NOT_MINE}));

    // The wallet's fingerprint on someone else's key is not this wallet's key.
    const std::string impostor{"[" + HexStr(master.Neuter().id_key_fingerprint()) + "/48h/1h/0h/2h]" + b.substr(b.find(']') + 1)};
    result = interface->analyzeDescriptor("wsh(sortedmulti(2," + impostor + R + "," + c + R + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == false);

    // MuSig participants are matched; the aggregate is not.
    result = interface->analyzeDescriptor("tr(musig(" + mine + "," + b + ")" + R + ",sortedmulti_a(2," + mine + R + "," + b + R + "," + c + R + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->analysis.IsRecognized());
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::MINE, KeyMatch::NOT_MINE,
                                                       KeyMatch::MINE, KeyMatch::NOT_MINE, KeyMatch::NOT_MINE}));

    // A single-sig descriptor is not a multisig setup, but its key is matched and the wallet can sign.
    result = interface->analyzeDescriptor("wpkh(" + mine + "/0/*)");
    BOOST_REQUIRE(result);
    BOOST_CHECK(!result->analysis.IsRecognized());
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE}));
    BOOST_CHECK(result->CanSign() == true);
    BOOST_CHECK_EQUAL(result->first_addresses.size(), 1U);

    // combo() starts with a P2PK script, which has no address: its first address is the P2PKH one.
    const auto first_key{Assert(DeriveExtKey(master, {0x80000030, 0x80000001, 0x80000000, 0x80000002, 0, 0}))};
    result = interface->analyzeDescriptor("combo(" + mine + "/0/*)");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->first_addresses == std::vector<CTxDestination>({PKHash{first_key->first.Neuter().pubkey}}));
    // A P2PK script alone has no address.
    result = interface->analyzeDescriptor("pk(" + mine + "/0/*)");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->first_addresses.empty());

    // Private keys in the descriptor derive the first address through hardened steps.
    const CExtKey other{NewHDKey()};
    result = interface->analyzeDescriptor("wpkh(" + EncodeExtKey(other) + "/84h/1h/0h/0/*)");
    BOOST_REQUIRE(result);
    const auto receive_key{Assert(DeriveExtKey(other, {0x80000054, 0x80000001, 0x80000000, 0, 0}))};
    BOOST_CHECK(result->first_addresses == std::vector<CTxDestination>({WitnessV0KeyHash{receive_key->first.Neuter().pubkey}}));
    // They are reported, and don't make the key the wallet's.
    BOOST_CHECK(result->has_private_keys);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE}));

    // Without the private key, the hardened steps can't be derived.
    result = interface->analyzeDescriptor("wpkh(" + EncodeExtPubKey(other.Neuter()) + "/84h/1h/0h/0/*)");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->first_addresses.empty());
    // All paths or none: a change path that can't be derived leaves out the receive address too.
    result = interface->analyzeDescriptor("wpkh(" + EncodeExtPubKey(other.Neuter()) + "/<0;1h>/*)");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->first_addresses.empty());
    result = interface->analyzeDescriptor("wpkh(" + EncodeExtPubKey(other.Neuter()) + "/<0;1>/*)");
    BOOST_REQUIRE(result);
    BOOST_CHECK_EQUAL(result->first_addresses.size(), 2U);

    result = interface->analyzeDescriptor("wsh(sortedmulti(2,");
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().code == WalletErrorCode::GenericError);
}

BOOST_AUTO_TEST_CASE(analyzedescriptor_wallet_locked)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);
    const CExtKey master{NewHDKey()};
    BOOST_REQUIRE(interface->addHDKey(master));
    BOOST_REQUIRE(wallet->EncryptWallet("hunter2"));
    BOOST_REQUIRE(wallet->Lock());

    // Deriving a hardened path needs the private key: a locked wallet can't tell.
    const std::string derived{"wsh(sortedmulti(2," + AccountKey(master) + R + "," + AccountKey(NewHDKey()) + R + "))"};
    auto result{interface->analyzeDescriptor(derived)};
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::UNKNOWN, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == std::nullopt);

    // The HD key itself is found without decrypting anything.
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + EncodeExtPubKey(master.Neuter()) + "/0/*," + AccountKey(NewHDKey()) + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::NOT_MINE}));
    BOOST_CHECK(wallet->IsLocked());

    // So is a key with an unhardened origin path: public derivation needs no private key.
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + OriginKey(master, {0, 1}) + "/0/*," + AccountKey(NewHDKey()) + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == true);
    BOOST_CHECK(wallet->IsLocked());

    // One key it can't tell doesn't hide another that is the wallet's.
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + AccountKey(master) + "/0/*," + OriginKey(master, {0, 1}) + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::UNKNOWN, KeyMatch::MINE}));
    BOOST_CHECK(result->CanSign() == true);

    // The account xpub is also held without its private key, but the locked
    // master may still hold it, so the key stays "unknown".
    AddDescriptor(*wallet, "wpkh(" + AccountKey(master) + "/0/*)");
    result = interface->analyzeDescriptor(derived);
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::UNKNOWN, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == std::nullopt);

    BOOST_REQUIRE(wallet->Unlock("hunter2"));
    result = interface->analyzeDescriptor(derived);
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == true);
}

BOOST_AUTO_TEST_CASE(analyzedescriptor_watch_only_and_constant_keys)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    // An xpub the wallet has without its private key.
    const CExtKey watched_master{NewHDKey()};
    const std::string watched{EncodeExtPubKey(watched_master.Neuter())};
    AddDescriptor(*wallet, "wpkh(" + watched + "/0/*)");

    // A private key whose public key has an odd Y: its x-only form must still match.
    const CKey key{OddKey()};
    AddKey(*wallet, key);

    const std::string desc{"tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",multi_a(1," + watched + "/0/*," + HexStr(XOnlyPubKey{key.GetPubKey()}) + "))"};
    auto result{interface->analyzeDescriptor(desc)};
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::WATCH_ONLY, KeyMatch::MINE}));
    // A constant cosigner key makes the setup unrecognized, but the wallet can still sign with it.
    BOOST_CHECK(!result->analysis.IsRecognized());
    BOOST_CHECK(result->CanSign() == true);

    // In Taproot the even-Y full key is the same key; for ECDSA it is not.
    const std::string even{"02" + HexStr(XOnlyPubKey{key.GetPubKey()})};
    result = interface->analyzeDescriptor("tr(" + HexStr(XOnlyPubKey::NUMS_H) + ",multi_a(1," + even + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::MINE}));
    result = interface->analyzeDescriptor("wsh(multi(1," + even + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE}));

    // Deriving a hardened origin path from a watched xpub is impossible even
    // when unlocked, so the key is not "unknown" and doesn't hide can_sign.
    result = interface->analyzeDescriptor("wsh(sortedmulti(1," + AccountKey(watched_master) + "/0/*," + AccountKey(NewHDKey()) + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::NOT_MINE}));
    BOOST_CHECK(result->CanSign() == false);

    // A plain public key the wallet has without its private key.
    const std::string watched_key{HexStr(GenerateRandomKey().GetPubKey())};
    AddDescriptor(*wallet, "wpkh(" + watched_key + ")");
    result = interface->analyzeDescriptor("wsh(multi(1," + watched_key + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::WATCH_ONLY}));
    BOOST_CHECK(result->CanSign() == false);

    // The watched xpub's private key, imported alone in another descriptor: the
    // wallet signs with each descriptor's own keys, and none has both the private
    // key and the chain code, so the xpub is still watch-only, as in gethdkeys.
    AddDescriptor(*wallet, "pk(" + EncodeSecret(watched_master.key) + ")");
    result = interface->analyzeDescriptor("wsh(multi(1," + watched + "/0/*))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::WATCH_ONLY}));
    BOOST_CHECK(result->CanSign() == false);
}

BOOST_AUTO_TEST_CASE(analyzedescriptor_derived_constant_keys)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    // The wallet derives receive keys from its HD key, so it stores no private
    // key for them, only where it derived them.
    const CExtKey master{NewHDKey()};
    AddDescriptor(*wallet, "wpkh(" + EncodeExtKey(master) + "/84h/1h/0h/0/*)");
    const auto receive_key = [&](uint32_t index) {
        return HexStr(Assert(DeriveExtKey(master, {0x80000054, 0x80000001, 0x80000000, 0, index}))->first.Neuter().pubkey);
    };
    const std::string mine{receive_key(0)};
    const std::string other{HexStr(GenerateRandomKey().GetPubKey())};

    auto result{interface->analyzeDescriptor("wpkh(" + mine + ")")};
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE}));
    BOOST_CHECK(result->CanSign() == true);
    result = interface->analyzeDescriptor("wsh(multi(1," + mine + "," + other + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::NOT_MINE}));
    // In Taproot, as an x-only key.
    result = interface->analyzeDescriptor("tr(" + mine.substr(2) + ")");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE}));

    // A key the wallet hasn't derived yet is only found with its origin.
    const std::string later{receive_key(5000)};
    result = interface->analyzeDescriptor("wpkh(" + later + ")");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE}));
    const std::string origin{"[" + HexStr(master.Neuter().id_key_fingerprint()) + "/84h/1h/0h/0/5000]"};
    result = interface->analyzeDescriptor("wpkh(" + origin + later + ")");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE}));

    // Another cosigner's key the wallet derived for a multisig descriptor is
    // held without its private key: its recorded origin is the other cosigner's.
    const CExtKey cosigner{NewHDKey()};
    AddDescriptor(*wallet, "wsh(multi(1," + EncodeExtKey(NewHDKey()) + "/0/*," + AccountKey(cosigner) + "/0/*))");
    const std::string cosigner_key{HexStr(Assert(DeriveExtKey(cosigner, {0x80000030, 0x80000001, 0x80000000, 0x80000002, 0, 0}))->first.Neuter().pubkey)};
    result = interface->analyzeDescriptor("wpkh(" + cosigner_key + ")");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::WATCH_ONLY}));
    BOOST_CHECK(result->CanSign() == false);

    // Its origin path is hardened, so a locked wallet can't tell.
    BOOST_REQUIRE(wallet->EncryptWallet("hunter2"));
    BOOST_REQUIRE(wallet->Lock());
    result = interface->analyzeDescriptor("wpkh(" + mine + ")");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::UNKNOWN}));
    BOOST_CHECK(result->CanSign() == std::nullopt);
    BOOST_REQUIRE(wallet->Unlock("hunter2"));
    result = interface->analyzeDescriptor("wpkh(" + mine + ")");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE}));
}

BOOST_AUTO_TEST_CASE(analyzedescriptor_account_keys)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    // An account xprv imported with its master's origin, without the master.
    const CExtKey master{NewHDKey()};
    const std::vector<uint32_t> account_path{0x80000054, 0x80000001, 0x80000000};
    const CExtKey account{Assert(DeriveExtKey(master, account_path))->first};
    const std::string fingerprint{HexStr(master.Neuter().id_key_fingerprint())};
    AddDescriptor(*wallet, "wpkh([" + fingerprint + "/84h/1h/0h]" + EncodeExtKey(account) + "/0/*)");
    const auto key_at = [&](const std::vector<uint32_t>& path) { return Assert(DeriveExtKey(account, path))->first.Neuter(); };

    // A key the wallet derived, matched through the origin it recorded, which starts at the master.
    auto result{interface->analyzeDescriptor("wpkh(" + HexStr(key_at({0, 0}).pubkey) + ")")};
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE}));
    BOOST_CHECK(result->CanSign() == true);

    // Keys with an origin below the account's: derived from the account key.
    const std::string later{"[" + fingerprint + "/84h/1h/0h/0/5000]" + HexStr(key_at({0, 5000}).pubkey)};
    const std::string child{"[" + fingerprint + "/84h/1h/0h/0]" + EncodeExtPubKey(key_at({0})) + "/*"};
    const std::string hardened{"[" + fingerprint + "/84h/1h/0h/1h]" + EncodeExtPubKey(key_at({0x80000001})) + "/*"};
    const std::string impostor{"[" + fingerprint + "/84h/1h/0h/0/5000]" + HexStr(GenerateRandomKey().GetPubKey())};
    result = interface->analyzeDescriptor("wsh(multi(1," + later + "," + child + "," + hardened + "," + impostor + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::MINE, KeyMatch::MINE, KeyMatch::NOT_MINE}));

    // Only the hardened step needs the private key.
    BOOST_REQUIRE(wallet->EncryptWallet("hunter2"));
    BOOST_REQUIRE(wallet->Lock());
    result = interface->analyzeDescriptor("wsh(multi(1," + later + "," + child + "," + hardened + "," + impostor + "))");
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::MINE, KeyMatch::MINE, KeyMatch::UNKNOWN, KeyMatch::NOT_MINE}));
}

BOOST_AUTO_TEST_CASE(analyzedescriptor_musig_only_keys)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    auto interface = interfaces::MakeWallet(context, wallet);

    // Keys that appear only as musig() participants are not among the wallet's
    // HD keys (gethdkeys), but are still matched.
    const CExtKey mine{NewHDKey()}, other{NewHDKey()};
    AddDescriptor(*wallet, "tr(musig(" + EncodeExtKey(mine) + "," + EncodeExtPubKey(other.Neuter()) + ")/0/*)");
    BOOST_CHECK(!WITH_LOCK(wallet->cs_wallet, return wallet->GetHDPubKeys(CWallet::HDKeyFilter::All)).contains(mine.Neuter()));

    auto result{interface->analyzeDescriptor("tr(musig(" + EncodeExtPubKey(mine.Neuter()) + "," + EncodeExtPubKey(other.Neuter()) + ")/0/*)")};
    BOOST_REQUIRE(result);
    BOOST_CHECK(result->keys == std::vector<KeyMatch>({KeyMatch::NOT_MINE, KeyMatch::MINE, KeyMatch::WATCH_ONLY}));
    BOOST_CHECK(result->CanSign() == true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace wallet
