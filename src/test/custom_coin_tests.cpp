// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Custom Coin Types Test Suite
 *
 * Comprehensive tests for extended coin types that demonstrate:
 * - ExtendedCoin: Coins with metadata and tracking
 * - ColoredCoin: Asset/token representation
 * - TimeLockCoin: Time-based spending restrictions
 * - AnnotatedCoin: Analytics and research metadata
 * - MultiCoin: Polymorphic coin handling
 */

#include <test/custom_coin_types.h>

#include <coins.h>
#include <consensus/amount.h>
#include <policy/policy.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <thread>

BOOST_FIXTURE_TEST_SUITE(custom_coin_tests, BasicTestingSetup)

// Helper: Create a basic coin for testing
static Coin CreateBaseCoin(CAmount value = COIN, int height = 100, bool is_coinbase = false)
{
    CTxOut txout;
    txout.nValue = value;
    txout.scriptPubKey = CScript() << OP_DUP << OP_HASH160
                                   << std::vector<uint8_t>(20, 0x01)
                                   << OP_EQUALVERIFY << OP_CHECKSIG;
    return Coin(std::move(txout), height, is_coinbase);
}

/**
 * Test 1: ExtendedCoin - Basic functionality
 */
BOOST_AUTO_TEST_CASE(extended_coin_basic)
{
    // Create an extended coin with a tag
    Coin base = CreateBaseCoin(5 * COIN, 200, false);
    ExtendedCoin ext_coin(std::move(base), "savings");

    // Verify base coin properties are preserved
    BOOST_CHECK_EQUAL(ext_coin.GetValue(), 5 * COIN);
    BOOST_CHECK_EQUAL(ext_coin.GetHeight(), 200);
    BOOST_CHECK(!ext_coin.IsCoinBase());
    BOOST_CHECK(!ext_coin.IsSpent());

    // Verify extended properties
    BOOST_CHECK_EQUAL(ext_coin.GetTag(), "savings");
    BOOST_CHECK(ext_coin.GetTimestamp() > 0);
    BOOST_CHECK(!ext_coin.GetTrackingId().IsNull());

    // Test metadata
    ext_coin.SetMetadata("User: Alice, Purpose: Long-term storage");
    BOOST_CHECK_EQUAL(ext_coin.metadata, "User: Alice, Purpose: Long-term storage");

    // Test memory usage tracking
    size_t base_memory = ext_coin.base_coin.DynamicMemoryUsage();
    size_t total_memory = ext_coin.DynamicMemoryUsage();
    BOOST_CHECK_GE(total_memory, base_memory);
}

/**
 * Test 2: ExtendedCoin - Multiple coins with different tags
 */
BOOST_AUTO_TEST_CASE(extended_coin_tagging)
{
    std::vector<ExtendedCoin> wallet_coins;

    // Create coins with different tags
    const std::vector<std::string> tags = {"hot_wallet", "cold_storage", "exchange", "payments"};

    for (const auto& tag : tags) {
        Coin base = CreateBaseCoin(COIN, 100, false);
        ExtendedCoin ext(std::move(base), tag);
        wallet_coins.push_back(std::move(ext));
    }

    // Verify each coin has correct tag
    for (size_t i = 0; i < wallet_coins.size(); i++) {
        BOOST_CHECK_EQUAL(wallet_coins[i].GetTag(), tags[i]);
    }

    // Each coin should have unique tracking ID
    for (size_t i = 0; i < wallet_coins.size(); i++) {
        for (size_t j = i + 1; j < wallet_coins.size(); j++) {
            BOOST_CHECK(wallet_coins[i].GetTrackingId() != wallet_coins[j].GetTrackingId());
        }
    }
}

/**
 * Test 3: ColoredCoin - Token creation
 */
BOOST_AUTO_TEST_CASE(colored_coin_tokens)
{
    // Create a token asset
    uint256 asset_id = uint256S("0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef");
    Coin base = CreateBaseCoin(1000, 100, false);  // 1000 satoshis for UTXO dust limit

    // Create colored coin representing 1,000,000 tokens
    ColoredCoin token_coin(std::move(base), asset_id, 1000000, ColoredCoin::AssetType::TOKEN);

    // Verify it's a token
    BOOST_CHECK(token_coin.IsToken());
    BOOST_CHECK(!token_coin.IsNFT());
    BOOST_CHECK(!token_coin.IsStandardCoin());

    // Verify amounts
    BOOST_CHECK_EQUAL(token_coin.GetBTCValue(), 1000);  // BTC value for UTXO
    BOOST_CHECK_EQUAL(token_coin.GetAssetAmount(), 1000000);  // Token amount

    // Set issuer
    uint256 issuer = uint256S("0xabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcdefabcd");
    token_coin.SetIssuer(issuer);
    BOOST_CHECK(token_coin.issuer_id.has_value());
    BOOST_CHECK_EQUAL(token_coin.issuer_id.value(), issuer);

    // Set token properties
    token_coin.SetProperty("name", "MyToken");
    token_coin.SetProperty("symbol", "MTK");
    token_coin.SetProperty("decimals", "8");

    BOOST_CHECK_EQUAL(token_coin.GetProperty("name").value(), "MyToken");
    BOOST_CHECK_EQUAL(token_coin.GetProperty("symbol").value(), "MTK");
    BOOST_CHECK_EQUAL(token_coin.GetProperty("decimals").value(), "8");
    BOOST_CHECK(!token_coin.GetProperty("nonexistent").has_value());
}

/**
 * Test 4: ColoredCoin - NFT creation
 */
BOOST_AUTO_TEST_CASE(colored_coin_nft)
{
    // Create an NFT
    uint256 nft_id = uint256S("0xdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef");
    Coin base = CreateBaseCoin(10000, 100, false);

    ColoredCoin nft(std::move(base), nft_id, 1, ColoredCoin::AssetType::NFT);

    BOOST_CHECK(nft.IsNFT());
    BOOST_CHECK(!nft.IsToken());
    BOOST_CHECK_EQUAL(nft.GetAssetAmount(), 1);  // NFTs have amount = 1

    // Set NFT metadata
    nft.SetProperty("name", "CryptoPunk #1234");
    nft.SetProperty("image", "ipfs://QmXyZ...");
    nft.SetProperty("rarity", "legendary");
    nft.SetProperty("collection", "CryptoPunks");

    BOOST_CHECK_EQUAL(nft.GetProperty("name").value(), "CryptoPunk #1234");
    BOOST_CHECK_EQUAL(nft.GetProperty("rarity").value(), "legendary");
}

/**
 * Test 5: ColoredCoin - Multiple asset types
 */
BOOST_AUTO_TEST_CASE(colored_coin_asset_types)
{
    std::vector<ColoredCoin> assets;

    // Create different asset types
    assets.push_back(ColoredCoin(
        CreateBaseCoin(COIN, 100, false),
        InsecureRand256(),
        1000000,
        ColoredCoin::AssetType::TOKEN
    ));

    assets.push_back(ColoredCoin(
        CreateBaseCoin(COIN, 100, false),
        InsecureRand256(),
        100,
        ColoredCoin::AssetType::BOND
    ));

    assets.push_back(ColoredCoin(
        CreateBaseCoin(COIN, 100, false),
        InsecureRand256(),
        500,
        ColoredCoin::AssetType::EQUITY
    ));

    // Verify each has correct type
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(assets[0].asset_type), static_cast<uint8_t>(ColoredCoin::AssetType::TOKEN));
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(assets[1].asset_type), static_cast<uint8_t>(ColoredCoin::AssetType::BOND));
    BOOST_CHECK_EQUAL(static_cast<uint8_t>(assets[2].asset_type), static_cast<uint8_t>(ColoredCoin::AssetType::EQUITY));
}

/**
 * Test 6: TimeLockCoin - Absolute time lock
 */
BOOST_AUTO_TEST_CASE(timelock_coin_absolute_time)
{
    Coin base = CreateBaseCoin(10 * COIN, 100, false);

    // Lock until timestamp 2000000000 (May 2033)
    uint64_t unlock_time = 2000000000;
    TimeLockCoin locked_coin(std::move(base), TimeLockCoin::LockType::ABSOLUTE_TIME, unlock_time);

    // Should not be spendable before unlock time
    BOOST_CHECK(!locked_coin.IsSpendable(unlock_time - 1, 1000));

    // Should be spendable at unlock time
    BOOST_CHECK(locked_coin.IsSpendable(unlock_time, 1000));

    // Should be spendable after unlock time
    BOOST_CHECK(locked_coin.IsSpendable(unlock_time + 1000, 1000));

    BOOST_CHECK_EQUAL(locked_coin.GetUnlockTime(), unlock_time);
}

/**
 * Test 7: TimeLockCoin - Absolute height lock
 */
BOOST_AUTO_TEST_CASE(timelock_coin_absolute_height)
{
    Coin base = CreateBaseCoin(10 * COIN, 100, false);

    // Lock until block height 1000
    uint64_t unlock_height = 1000;
    TimeLockCoin locked_coin(std::move(base), TimeLockCoin::LockType::ABSOLUTE_HEIGHT, unlock_height);

    // Should not be spendable before unlock height
    BOOST_CHECK(!locked_coin.IsSpendable(0, 999));

    // Should be spendable at unlock height
    BOOST_CHECK(locked_coin.IsSpendable(0, 1000));

    // Should be spendable after unlock height
    BOOST_CHECK(locked_coin.IsSpendable(0, 1001));

    BOOST_CHECK_EQUAL(locked_coin.GetUnlockHeight(), 1000);
}

/**
 * Test 8: TimeLockCoin - Relative time lock
 */
BOOST_AUTO_TEST_CASE(timelock_coin_relative_time)
{
    Coin base = CreateBaseCoin(10 * COIN, 100, false);

    // Lock for 3600 seconds (1 hour) after creation
    uint64_t lock_duration = 3600;
    TimeLockCoin locked_coin(std::move(base), TimeLockCoin::LockType::RELATIVE_TIME, lock_duration);

    uint64_t creation_time = locked_coin.activation_time;
    uint64_t unlock_time = creation_time + lock_duration;

    // Should not be spendable before duration expires
    BOOST_CHECK(!locked_coin.IsSpendable(unlock_time - 1, 1000));

    // Should be spendable after duration
    BOOST_CHECK(locked_coin.IsSpendable(unlock_time, 1000));
}

/**
 * Test 9: TimeLockCoin - Relative height lock (CSV-like)
 */
BOOST_AUTO_TEST_CASE(timelock_coin_relative_height)
{
    Coin base = CreateBaseCoin(10 * COIN, 100, false);

    // Lock for 144 blocks (approximately 1 day)
    uint64_t lock_blocks = 144;
    TimeLockCoin locked_coin(std::move(base), TimeLockCoin::LockType::RELATIVE_HEIGHT, lock_blocks);

    int creation_height = locked_coin.base_coin.nHeight;  // 100
    int unlock_height = creation_height + lock_blocks;     // 244

    // Should not be spendable before required blocks
    BOOST_CHECK(!locked_coin.IsSpendable(0, unlock_height - 1));

    // Should be spendable after required blocks
    BOOST_CHECK(locked_coin.IsSpendable(0, unlock_height));

    BOOST_CHECK_EQUAL(locked_coin.GetUnlockHeight(), 244);
}

/**
 * Test 10: AnnotatedCoin - Analytics tracking
 */
BOOST_AUTO_TEST_CASE(annotated_coin_analytics)
{
    Txid origin_tx = Txid::FromUint256(InsecureRand256());
    Coin base = CreateBaseCoin(2 * COIN, 150, false);

    AnnotatedCoin annotated(std::move(base), origin_tx, 3);

    // Verify origin tracking
    const auto& analytics = annotated.GetAnalytics();
    BOOST_CHECK_EQUAL(analytics.origin_tx, origin_tx);
    BOOST_CHECK_EQUAL(analytics.origin_output_index, 3);
    BOOST_CHECK(analytics.creation_time > 0);

    // Set category and label
    annotated.SetCategory("mining_reward");
    annotated.SetAddressLabel("Mining Pool XYZ");
    BOOST_CHECK_EQUAL(analytics.category, "mining_reward");
    BOOST_CHECK_EQUAL(analytics.address_label, "Mining Pool XYZ");

    // Mark as dust
    annotated.MarkAsDust(true);
    BOOST_CHECK(analytics.is_dust);

    // Mark as change
    annotated.MarkAsChange(true);
    BOOST_CHECK(analytics.is_change);
}

/**
 * Test 11: AnnotatedCoin - UTXO set analysis
 */
BOOST_AUTO_TEST_CASE(annotated_coin_utxo_analysis)
{
    std::vector<AnnotatedCoin> utxo_set;

    // Simulate different coin categories
    struct CoinInfo {
        CAmount value;
        std::string category;
        bool is_dust;
        bool is_change;
    };

    std::vector<CoinInfo> test_coins = {
        {100 * COIN, "exchange_deposit", false, false},
        {0.5 * COIN, "payment", false, false},
        {546, "dust", true, false},
        {0.001 * COIN, "change", false, true},
        {50 * COIN, "mining_reward", false, false},
    };

    for (size_t i = 0; i < test_coins.size(); i++) {
        const auto& info = test_coins[i];
        Coin base = CreateBaseCoin(info.value, 100 + i, false);
        AnnotatedCoin annotated(std::move(base), Txid::FromUint256(InsecureRand256()), i);

        annotated.SetCategory(info.category);
        annotated.MarkAsDust(info.is_dust);
        annotated.MarkAsChange(info.is_change);

        utxo_set.push_back(std::move(annotated));
    }

    // Analyze the UTXO set
    int dust_count = 0;
    int change_count = 0;
    CAmount total_value = 0;

    for (const auto& coin : utxo_set) {
        const auto& analytics = coin.GetAnalytics();
        if (analytics.is_dust) dust_count++;
        if (analytics.is_change) change_count++;
        total_value += coin.base_coin.out.nValue;
    }

    BOOST_CHECK_EQUAL(dust_count, 1);
    BOOST_CHECK_EQUAL(change_count, 1);
    BOOST_CHECK_GT(total_value, 150 * COIN);
}

/**
 * Test 12: MultiCoin - Polymorphic handling
 */
BOOST_AUTO_TEST_CASE(multicoin_variant)
{
    std::vector<MultiCoin> coin_collection;

    // Add different coin types to collection
    coin_collection.push_back(CreateBaseCoin(COIN, 100, false));

    ExtendedCoin ext(CreateBaseCoin(2 * COIN, 100, false), "tagged");
    coin_collection.push_back(std::move(ext));

    ColoredCoin colored(CreateBaseCoin(1000, 100, false), InsecureRand256(), 100, ColoredCoin::AssetType::TOKEN);
    coin_collection.push_back(std::move(colored));

    TimeLockCoin locked(CreateBaseCoin(5 * COIN, 100, false), TimeLockCoin::LockType::ABSOLUTE_HEIGHT, 1000);
    coin_collection.push_back(std::move(locked));

    AnnotatedCoin annotated(CreateBaseCoin(3 * COIN, 100, false), Txid::FromUint256(InsecureRand256()), 0);
    coin_collection.push_back(std::move(annotated));

    // Use helper functions to work with any coin type
    BOOST_CHECK_EQUAL(coin_collection.size(), 5);

    for (const auto& coin : coin_collection) {
        // All should be unspent
        BOOST_CHECK(!CustomCoinHelpers::IsSpent(coin));

        // All should have positive value
        BOOST_CHECK_GT(CustomCoinHelpers::GetValue(coin), 0);
    }
}

/**
 * Test 13: Serialization - ExtendedCoin
 */
BOOST_AUTO_TEST_CASE(serialization_extended_coin)
{
    // Create original coin
    ExtendedCoin original(CreateBaseCoin(7 * COIN, 200, false), "test_tag");
    original.SetMetadata("test metadata");

    // Serialize
    DataStream ss{};
    ss << original;

    // Deserialize
    ExtendedCoin deserialized;
    ss >> deserialized;

    // Verify all properties match
    BOOST_CHECK_EQUAL(deserialized.GetValue(), 7 * COIN);
    BOOST_CHECK_EQUAL(deserialized.GetHeight(), 200);
    BOOST_CHECK_EQUAL(deserialized.GetTag(), "test_tag");
    BOOST_CHECK_EQUAL(deserialized.metadata, "test metadata");
    BOOST_CHECK_EQUAL(deserialized.GetTrackingId(), original.GetTrackingId());
}

/**
 * Test 14: Serialization - ColoredCoin
 */
BOOST_AUTO_TEST_CASE(serialization_colored_coin)
{
    uint256 asset_id = InsecureRand256();
    ColoredCoin original(CreateBaseCoin(COIN, 100, false), asset_id, 5000, ColoredCoin::AssetType::TOKEN);
    original.SetProperty("name", "TestToken");

    // Serialize
    DataStream ss{};
    ss << original;

    // Deserialize
    ColoredCoin deserialized;
    ss >> deserialized;

    // Verify
    BOOST_CHECK_EQUAL(deserialized.asset_id, asset_id);
    BOOST_CHECK_EQUAL(deserialized.GetAssetAmount(), 5000);
    BOOST_CHECK(deserialized.IsToken());
    BOOST_CHECK_EQUAL(deserialized.GetProperty("name").value(), "TestToken");
}

/**
 * Test 15: Memory usage comparison
 */
BOOST_AUTO_TEST_CASE(memory_usage_comparison)
{
    Coin standard = CreateBaseCoin(COIN, 100, false);
    size_t standard_memory = standard.DynamicMemoryUsage();

    ExtendedCoin extended(CreateBaseCoin(COIN, 100, false), "long_tag_name");
    extended.SetMetadata("Some lengthy metadata string for testing memory usage");
    size_t extended_memory = extended.DynamicMemoryUsage();

    ColoredCoin colored(CreateBaseCoin(COIN, 100, false), InsecureRand256(), 1000, ColoredCoin::AssetType::TOKEN);
    colored.SetProperty("name", "Token");
    colored.SetProperty("symbol", "TKN");
    colored.SetProperty("description", "A test token with properties");
    size_t colored_memory = colored.DynamicMemoryUsage();

    // Extended and colored coins should use more memory
    BOOST_CHECK_GT(extended_memory, standard_memory);
    BOOST_CHECK_GT(colored_memory, standard_memory);

    // More properties = more memory
    ColoredCoin minimal_colored(CreateBaseCoin(COIN, 100, false), InsecureRand256(), 1000, ColoredCoin::AssetType::TOKEN);
    size_t minimal_colored_memory = minimal_colored.DynamicMemoryUsage();
    BOOST_CHECK_GT(colored_memory, minimal_colored_memory);
}

BOOST_AUTO_TEST_SUITE_END()
