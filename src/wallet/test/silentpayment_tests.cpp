#include <test/util/setup_common.h>
#include <silentpayment.h>

#include <boost/test/unit_test.hpp>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(silentpayment_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(silent_addresses)
{
    auto txid1 = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
    uint32_t vout1 = 4;

    auto txid2 = uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
    uint32_t vout2 = 7;

    std::vector<COutPoint> tx_outpoints;
    tx_outpoints.emplace_back(txid1, vout1);
    tx_outpoints.emplace_back(txid2, vout2);

    std::vector<std::tuple<CKey, bool>> sender_secret_keys;
    std::vector<CPubKey> sender_pub_keys;
    std::vector<XOnlyPubKey> sender_x_only_pub_keys;

    // non-taproot inputs
    for(size_t i =0; i < 34; i++) {
        CKey senderkey;
        senderkey.MakeNewKey(true);
        CPubKey senderPubkey = senderkey.GetPubKey();

        sender_secret_keys.emplace_back(senderkey, false);
        sender_pub_keys.emplace_back(senderPubkey);
    }

    // taproot inputs
    for(size_t i =0; i < 45; i++) {
        CKey senderkey;
        senderkey.MakeNewKey(true);

        sender_secret_keys.emplace_back(senderkey, true);
        sender_x_only_pub_keys.emplace_back(senderkey.GetPubKey());
    }

    CKey recipient_spend_seckey;
    recipient_spend_seckey.MakeNewKey(true);

    auto silent_recipient = silentpayment::Recipient(recipient_spend_seckey, 440);

    CPubKey combined_tx_pubkeys{silentpayment::Recipient::CombinePublicKeys(sender_pub_keys, sender_x_only_pub_keys)};

    silent_recipient.SetSenderPublicKey(combined_tx_pubkeys, tx_outpoints);

    for (int32_t identifier = 0; identifier < 234; identifier++) {
        const auto&[recipient_scan_pubkey, recipient_spend_pubkey]{silent_recipient.GetAddress(identifier)};

        silentpayment::Sender silent_sender{
            sender_secret_keys,
            tx_outpoints,
            recipient_scan_pubkey
        };

        XOnlyPubKey sender_tweaked_pubkey = silent_sender.Tweak(recipient_spend_pubkey);
        const auto [recipient_priv_key, recipient_pub_key] = silent_recipient.Tweak(identifier);

        BOOST_CHECK(XOnlyPubKey{recipient_priv_key.GetPubKey()} == recipient_pub_key);
        BOOST_CHECK(sender_tweaked_pubkey == recipient_pub_key);
    }

}

class TestRecipient: public silentpayment::Recipient {
    public:
        TestRecipient(const CKey& spend_seckey, size_t pool_size) : Recipient(spend_seckey, pool_size) {}
        CKey GetNegatedScanSecKey() { return m_negated_scan_seckey; }
        std::vector<std::pair<CKey, XOnlyPubKey>> GetSpendKeys() { return m_spend_keys; }
        XOnlyPubKey GetScanPubkey() { return m_scan_pubkey; }
        std::array<unsigned char, 32> GetSharedSecret() {
            std::array<unsigned char, 32> result;
            std::copy(std::begin(m_shared_secret), std::end(m_shared_secret), std::begin(result));
            return result;
        }
};

class TestSender: public silentpayment::Sender {
    public:
        TestSender(const std::vector<std::tuple<CKey, bool>>& sender_secret_keys, const std::vector<COutPoint>& tx_outpoints, const XOnlyPubKey& recipient_scan_xonly_pubkey) :
            Sender(sender_secret_keys, tx_outpoints, recipient_scan_xonly_pubkey) {}
        std::array<unsigned char, 32> GetSharedSecret() {
            std::array<unsigned char, 32> result;
            std::copy(std::begin(m_shared_secret), std::end(m_shared_secret), std::begin(result));
            return result;
    }
};

BOOST_AUTO_TEST_CASE(silent_addresses2)
{
    std::vector<std::tuple<CKey, bool>> sender_secret_keys;
    std::vector<CPubKey> sender_pub_keys;
    std::vector<XOnlyPubKey> sender_x_only_pub_keys;

    const std::vector<std::pair<std::string, bool>> sender_keys = {
        {"KxeDWuvKtXfBxLGmP6ZT4crvbamCnicJg2xgkgESSLURYyr6kVvu", false},
        {"L3nddfqwDWmAeskCjWwLnnF5ZFt1BZXFxNav92eA3sS7kKbh3nT3", false},
        {"KyAWmQjXmkNL8xFaFc3Kp4S4SJSqYmGfvdePvuGwpsb7YyWAW5wS", true},
        {"KzrGQWApg8wpX4pCQM1chMrJhgSfzmwS8fZLfUDujA73sJNgreeF", true},
    };

    for (const auto& [wif, is_taproot]: sender_keys) {
        CKey senderkey = DecodeSecret(wif);

        sender_secret_keys.emplace_back(senderkey, is_taproot);
        if (is_taproot) {
            sender_x_only_pub_keys.emplace_back(senderkey.GetPubKey());
        } else {
            sender_pub_keys.emplace_back(senderkey.GetPubKey());
        }
    }

    CKey recipient_spend_seckey = DecodeSecret("L4mGgWaqFknPASp5cwrAbqhEcLuHqDMzm4UjJyAo1A7f85XPiGVU");

    auto silent_recipient = TestRecipient(recipient_spend_seckey, 5);

    BOOST_CHECK(EncodeSecret(silent_recipient.GetNegatedScanSecKey()) == "L2HBwB2tkUdGeb2KWx2EaUWD1YjW8u4eZ7uXJfJA8u3ibkrPtvh5");
    BOOST_CHECK(HexStr(silent_recipient.GetScanPubkey()) == "bfa2fb9b2d094a039d4b5bf76a159d028f15a8350d5c957651d6dc00af4ccf41");

    const std::vector<std::pair<std::string, std::string>> spend_keys = {
        {"KxFnFy2MjkfxGYbLWXbLztcbM5T5qQnaRaWcnCT9fySCB8XXVfRw","aef5a67267768f18f8efc327ca7add15d2bb9fcd6b6f4911424565eb6db0ae63"},
        {"L4jKwLMPDuMhwJzVv3GYbtNdqRng7Zf9tGqfCj277ATejavame9C","3246572723b9aa4c601f2b5a277e9af81b61ca53de8ad88eedfb563dec8c2fce"},
        {"KxKfkKVFoTXJipEVvLkazoFntugKGiCGB9nkzgkXTxkCx7nbQ9p4","c2ece65cae45f7475f99a911754189ac2aa999eba8691eab3adb51d711f73cd2"},
        {"L4fSSytVACWMV3MLWE7JbyjSHbZSgGFU8hZWzEijKB9dxbeSUA6b","8bc5214b98ad9491ba96b0c2dbb1b9ebf45849e599ac143e487246d783da56e9"},
        {"L4dVhof38M5gFuXkoKXgc2QqWgSpxcYdFuvSsza3RBVda73d52bA","c1547a55f2f4fd3bd7dbc94ba751bc0b6108be7ff0b6b995826db2dd0aebddba"},
    };

    size_t idx = 0;
    for(const auto& [spend_seckey, spend_pubkey]: silent_recipient.GetSpendKeys()) {

        const auto& [seckey, pubkey] = spend_keys.at(idx++);

        BOOST_CHECK(EncodeSecret(spend_seckey) == seckey);
        BOOST_CHECK(HexStr(spend_pubkey) == pubkey);
    }

    CPubKey combined_tx_pubkeys{silentpayment::Recipient::CombinePublicKeys(sender_pub_keys, sender_x_only_pub_keys)};

    BOOST_CHECK(HexStr(combined_tx_pubkeys) == "031ea6a65ac33fd26dd296299dbce0b40b8105af395c68655bc303a943f0f75025");

    auto txid1 = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
    uint32_t vout1 = 4;

    auto txid2 = uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
    uint32_t vout2 = 7;

    std::vector<COutPoint> tx_outpoints;
    tx_outpoints.emplace_back(txid1, vout1);
    tx_outpoints.emplace_back(txid2, vout2);

    const auto& [truncated_hash, outpoint_hash] = silentpayment::HashOutpoints(tx_outpoints);

    BOOST_CHECK(HexStr(outpoint_hash) == "db30d92d7e60ff38f4ad821d2d94996c0151f0706178d081254732808a9066f5");
    BOOST_CHECK(HexStr(truncated_hash) == "bd37fdb110dc3df7");

    silent_recipient.SetSenderPublicKey(combined_tx_pubkeys, tx_outpoints);

    auto _recipient_shared_secret = silent_recipient.GetSharedSecret();

    CKey recipient_shared_secret;
    recipient_shared_secret.Set(std::begin(_recipient_shared_secret), std::end(_recipient_shared_secret), true);

    BOOST_CHECK(EncodeSecret(recipient_shared_secret) == "Kx2qKiKqFMSRerYZeEUz1d9MRpKATczVXuJinaJZcAPgaTPaJC5v");

    int32_t identifier = 0;

    const auto&[recipient_scan_pubkey, recipient_spend_pubkey]{silent_recipient.GetAddress(identifier)};

    TestSender silent_sender{
        sender_secret_keys,
        tx_outpoints,
        recipient_scan_pubkey
    };

    auto _silent_shared_secret = silent_sender.GetSharedSecret();

    CKey silent_shared_secret;
    silent_shared_secret.Set(std::begin(_silent_shared_secret), std::end(_silent_shared_secret), true);

    BOOST_CHECK(recipient_shared_secret == silent_shared_secret);

    const std::vector<std::pair<std::string, std::string>> results = {
        {"1669506efdf338f5610a6b3314b7a0ad40940383f3314b0af6f4ab9708fd00f9","Ky4uQ2E3aRqnCASaN4Z27EebvH3kSU527w5ewdQbRtMmiDLWqgyr"},
        {"6ceda6fddc975987b642ef3d589b9028ad0b977316fcdc1ad0d7f92c42b60477","L5YT5PZ54aXXrvqjmaEDiEQeQdPLicwbadQhN9yYs5PEGfjzwEqY"},
        {"6669869ddb5b0e10e47f3ff76d5258172c7f6b1305993a51072d0354e196d0d3","Ky8ntNgwe8h8eS5jmsiG79HoU7GysmUhsWMoA7hyDsfnVCZP7cvJ"},
    };

    for (int32_t identifier = 0; identifier < 3; identifier++) {
        const auto&[recipient_scan_pubkey, recipient_spend_pubkey]{silent_recipient.GetAddress(identifier)};

        TestSender silent_sender{
            sender_secret_keys,
            tx_outpoints,
            recipient_scan_pubkey
        };

        XOnlyPubKey sender_tweaked_pubkey = silent_sender.Tweak(recipient_spend_pubkey);
        const auto [recipient_priv_key, recipient_pub_key] = silent_recipient.Tweak(identifier);

        BOOST_CHECK(XOnlyPubKey{recipient_priv_key.GetPubKey()} == recipient_pub_key);
        BOOST_CHECK(sender_tweaked_pubkey == recipient_pub_key);

        const auto& [expected_pubkey, expected_prvkey] = results.at(identifier);

        BOOST_CHECK(expected_pubkey == HexStr(sender_tweaked_pubkey));
        BOOST_CHECK(expected_prvkey == EncodeSecret(recipient_priv_key));
    }
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
