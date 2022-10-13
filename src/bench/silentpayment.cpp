// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <random.h>
#include <secp256k1.h>
#include <secp256k1_ecdh.h>
#include <secp256k1_extrakeys.h>
#include <silentpayment.h>
#include <test/util/setup_common.h>

static void SumXOnlyPublicKeys(benchmark::Bench& bench, size_t key_count)
{
    ECC_Start();

    std::vector<CPubKey> sender_pub_keys;
    std::vector<XOnlyPubKey> sender_x_only_pub_keys;

    // non-taproot inputs
    for(size_t i =0; i < key_count; i++) {
        CKey senderkey;
        senderkey.MakeNewKey(true);
        CPubKey senderPubkey = senderkey.GetPubKey();
        sender_pub_keys.push_back(senderPubkey);
    }

    // taproot inputs
    for(size_t i =0; i < key_count; i++) {
        CKey senderkey;
        senderkey.MakeNewKey(true);
        sender_x_only_pub_keys.push_back(XOnlyPubKey{senderkey.GetPubKey()});
    }

    bench.run([&] {
        CPubKey sum_tx_pubkeys{silentpayment::Recipient::CombinePublicKeys(sender_pub_keys, sender_x_only_pub_keys)};
        assert(sum_tx_pubkeys.IsFullyValid());
    });

    ECC_Stop();
}

static void ECDHPerformance(benchmark::Bench& bench, int32_t pool_size)
{
    ECC_Start();

    auto txid1 = uint256S("0x00000000000002dc756eebf4f49723ed8d30cc28a5f108eb94b1ba88ac4f9c22");
    uint32_t vout1 = 4;

    auto txid2 = uint256S("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
    uint32_t vout2 = 7;

    std::vector<COutPoint> tx_outpoints;
    tx_outpoints.emplace_back(txid1, vout1);
    tx_outpoints.emplace_back(txid2, vout2);

    std::vector<std::tuple<CKey, bool>> sender_secret_keys;

    CKey senderkey1;
    senderkey1.MakeNewKey(true);
    CPubKey senderPubkey1 = senderkey1.GetPubKey();
    sender_secret_keys.push_back({senderkey1, false});

    CKey senderkey2;
    senderkey2.MakeNewKey(true);
    XOnlyPubKey senderPubkey2 = XOnlyPubKey(senderkey2.GetPubKey());
    sender_secret_keys.push_back({senderkey2, true});

    CKey recipient_spend_seckey;
    recipient_spend_seckey.MakeNewKey(true);

    auto silent_recipient = silentpayment::Recipient(recipient_spend_seckey, pool_size);
    CPubKey sum_tx_pubkeys{silentpayment::Recipient::CombinePublicKeys({senderPubkey1}, {senderPubkey2})};

    bench.run([&] {
        silent_recipient.SetSenderPublicKey(sum_tx_pubkeys, tx_outpoints);

        for (int32_t identifier = 0; identifier < pool_size; identifier++) {
            const auto&[recipient_scan_pubkey, recipient_spend_pubkey]{silent_recipient.GetAddress(identifier)};

            silentpayment::Sender silent_sender{
                sender_secret_keys,
                tx_outpoints,
                recipient_scan_pubkey
            };

            XOnlyPubKey sender_tweaked_pubkey = silent_sender.Tweak(recipient_spend_pubkey);
            const auto [recipient_priv_key, recipient_pub_key] = silent_recipient.Tweak(identifier);

            assert(sender_tweaked_pubkey == recipient_pub_key);
        }
    });

    ECC_Stop();
}

static void SumXOnlyPublicKeys_1(benchmark::Bench& bench)
{
    SumXOnlyPublicKeys(bench, 1);
}
static void SumXOnlyPublicKeys_10(benchmark::Bench& bench)
{
    SumXOnlyPublicKeys(bench, 10);
}
static void SumXOnlyPublicKeys_100(benchmark::Bench& bench)
{
    SumXOnlyPublicKeys(bench, 100);
}
static void SumXOnlyPublicKeys_1000(benchmark::Bench& bench)
{
    SumXOnlyPublicKeys(bench, 1000);
}

static void ECDHPerformance_100(benchmark::Bench& bench)
{
    ECDHPerformance(bench, 100);
}

BENCHMARK(SumXOnlyPublicKeys_1, benchmark::PriorityLevel::HIGH);
BENCHMARK(SumXOnlyPublicKeys_10, benchmark::PriorityLevel::HIGH);
BENCHMARK(SumXOnlyPublicKeys_100, benchmark::PriorityLevel::HIGH);
BENCHMARK(SumXOnlyPublicKeys_1000, benchmark::PriorityLevel::HIGH);
BENCHMARK(ECDHPerformance_100, benchmark::PriorityLevel::HIGH);