// Copyright (c) 2025 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_CUSTOM_COIN_TYPES_H
#define BITCOIN_TEST_CUSTOM_COIN_TYPES_H

/**
 * Custom Coin Type Extensions
 *
 * This header provides extended coin types that demonstrate how to build
 * custom coin variants on top of Bitcoin Core's base Coin class.
 *
 * Use cases:
 * - Research and experimentation with coin metadata
 * - Colored coins / asset tracking prototypes
 * - Enhanced UTXO set analytics
 * - Custom indexing and tagging systems
 *
 * WARNING: This is for testing and educational purposes only.
 * Not intended for production use in Bitcoin Core.
 */

#include <coins.h>
#include <memusage.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <test/util/random.h>
#include <uint256.h>

#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>

/**
 * ExtendedCoin - Coin with additional metadata
 *
 * Wraps a standard Coin and adds custom tracking information:
 * - Creation timestamp
 * - User-defined tags
 * - Custom metadata string
 * - Tracking ID for analytics
 */
class ExtendedCoin
{
public:
    Coin base_coin;                          // The underlying Bitcoin coin
    uint64_t timestamp;                       // Creation timestamp (Unix epoch)
    std::string tag;                          // User-defined tag (e.g., "exchange", "savings")
    std::string metadata;                     // Arbitrary metadata
    uint256 tracking_id;                      // Unique tracking identifier

    ExtendedCoin() : timestamp(0), tracking_id() {}

    ExtendedCoin(Coin&& coin, const std::string& tag_str = "")
        : base_coin(std::move(coin))
        , timestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
        , tag(tag_str)
        , tracking_id(InsecureRand256())
    {}

    ExtendedCoin(const Coin& coin, const std::string& tag_str = "")
        : base_coin(coin)
        , timestamp(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
        , tag(tag_str)
        , tracking_id(InsecureRand256())
    {}

    // Accessors matching base Coin interface
    bool IsSpent() const { return base_coin.IsSpent(); }
    bool IsCoinBase() const { return base_coin.IsCoinBase(); }
    CAmount GetValue() const { return base_coin.out.nValue; }
    int GetHeight() const { return base_coin.nHeight; }

    // Extended functionality
    void SetMetadata(const std::string& data) { metadata = data; }
    std::string GetTag() const { return tag; }
    uint256 GetTrackingId() const { return tracking_id; }
    uint64_t GetTimestamp() const { return timestamp; }

    size_t DynamicMemoryUsage() const {
        return base_coin.DynamicMemoryUsage() +
               memusage::DynamicUsage(tag) +
               memusage::DynamicUsage(metadata);
    }

    SERIALIZE_METHODS(ExtendedCoin, obj) {
        READWRITE(obj.base_coin, obj.timestamp, obj.tag, obj.metadata, obj.tracking_id);
    }
};

/**
 * ColoredCoin - Coin with asset/token identification
 *
 * Extends the concept of a coin to support "colored coins" - UTXOs that
 * represent ownership of assets or tokens beyond just BTC value.
 *
 * Properties:
 * - Asset ID: Unique identifier for the asset type
 * - Asset amount: Quantity of the asset (separate from BTC value)
 * - Issuer: Optional issuer identification
 * - Properties: Key-value metadata for the asset
 */
class ColoredCoin
{
public:
    enum class AssetType : uint8_t {
        STANDARD = 0,      // Regular BTC
        TOKEN = 1,         // Fungible token
        NFT = 2,           // Non-fungible token
        BOND = 3,          // Debt instrument
        EQUITY = 4,        // Equity share
        CUSTOM = 255       // Custom asset type
    };

    Coin base_coin;                          // Underlying coin
    uint256 asset_id;                        // Asset identifier
    uint64_t asset_amount;                   // Amount of the asset
    AssetType asset_type;                    // Type of asset
    std::optional<uint256> issuer_id;        // Optional issuer identifier
    std::map<std::string, std::string> properties;  // Asset properties

    ColoredCoin()
        : asset_amount(0)
        , asset_type(AssetType::STANDARD)
    {}

    ColoredCoin(Coin&& coin, const uint256& asset, uint64_t amount, AssetType type = AssetType::TOKEN)
        : base_coin(std::move(coin))
        , asset_id(asset)
        , asset_amount(amount)
        , asset_type(type)
    {}

    // Asset operations
    bool IsStandardCoin() const { return asset_type == AssetType::STANDARD; }
    bool IsToken() const { return asset_type == AssetType::TOKEN; }
    bool IsNFT() const { return asset_type == AssetType::NFT; }

    void SetIssuer(const uint256& issuer) { issuer_id = issuer; }
    void SetProperty(const std::string& key, const std::string& value) {
        properties[key] = value;
    }

    std::optional<std::string> GetProperty(const std::string& key) const {
        auto it = properties.find(key);
        if (it != properties.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    // Base coin interface
    bool IsSpent() const { return base_coin.IsSpent(); }
    CAmount GetBTCValue() const { return base_coin.out.nValue; }
    uint64_t GetAssetAmount() const { return asset_amount; }

    size_t DynamicMemoryUsage() const {
        size_t usage = base_coin.DynamicMemoryUsage();
        for (const auto& [key, value] : properties) {
            usage += memusage::DynamicUsage(key) + memusage::DynamicUsage(value);
        }
        return usage;
    }

    SERIALIZE_METHODS(ColoredCoin, obj) {
        READWRITE(obj.base_coin, obj.asset_id, obj.asset_amount);
        READWRITE(Using<CustomUintFormatter<1>>(obj.asset_type));
        READWRITE(obj.issuer_id, obj.properties);
    }
};

/**
 * TimeLockCoin - Coin with time-based restrictions
 *
 * Adds absolute or relative timelock information to coins.
 * Useful for demonstrating time-locked UTXOs and covenant-like behavior.
 */
class TimeLockCoin
{
public:
    enum class LockType : uint8_t {
        NONE = 0,           // No lock
        ABSOLUTE_TIME = 1,  // Locked until specific timestamp
        ABSOLUTE_HEIGHT = 2,// Locked until specific block height
        RELATIVE_TIME = 3,  // Locked for time duration after confirmation
        RELATIVE_HEIGHT = 4 // Locked for number of blocks after confirmation
    };

    Coin base_coin;
    LockType lock_type;
    uint64_t lock_value;  // Timestamp, height, or duration depending on lock_type
    uint64_t activation_time;  // When the coin was created/confirmed

    TimeLockCoin()
        : lock_type(LockType::NONE)
        , lock_value(0)
        , activation_time(0)
    {}

    TimeLockCoin(Coin&& coin, LockType type, uint64_t value)
        : base_coin(std::move(coin))
        , lock_type(type)
        , lock_value(value)
        , activation_time(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
    {}

    // Check if coin is spendable
    bool IsSpendable(uint64_t current_time, int current_height) const {
        if (base_coin.IsSpent()) return false;

        switch (lock_type) {
            case LockType::NONE:
                return true;
            case LockType::ABSOLUTE_TIME:
                return current_time >= lock_value;
            case LockType::ABSOLUTE_HEIGHT:
                return current_height >= static_cast<int>(lock_value);
            case LockType::RELATIVE_TIME:
                return current_time >= (activation_time + lock_value);
            case LockType::RELATIVE_HEIGHT:
                return current_height >= (base_coin.nHeight + static_cast<int>(lock_value));
            default:
                return false;
        }
    }

    uint64_t GetUnlockTime() const {
        switch (lock_type) {
            case LockType::ABSOLUTE_TIME:
                return lock_value;
            case LockType::RELATIVE_TIME:
                return activation_time + lock_value;
            default:
                return 0;
        }
    }

    int GetUnlockHeight() const {
        switch (lock_type) {
            case LockType::ABSOLUTE_HEIGHT:
                return static_cast<int>(lock_value);
            case LockType::RELATIVE_HEIGHT:
                return base_coin.nHeight + static_cast<int>(lock_value);
            default:
                return 0;
        }
    }

    bool IsSpent() const { return base_coin.IsSpent(); }

    SERIALIZE_METHODS(TimeLockCoin, obj) {
        READWRITE(obj.base_coin);
        READWRITE(Using<CustomUintFormatter<1>>(obj.lock_type));
        READWRITE(obj.lock_value, obj.activation_time);
    }
};

/**
 * AnnotatedCoin - Coin with rich analytics metadata
 *
 * Designed for UTXO set analysis and research.
 * Tracks origin, usage patterns, and relationships.
 */
class AnnotatedCoin
{
public:
    struct Analytics {
        uint64_t creation_time;
        uint256 origin_tx;         // Transaction that created this coin
        int origin_output_index;   // Output index in origin transaction
        std::string address_label; // Human-readable address label
        std::string category;      // E.g., "mining", "exchange", "payment"
        uint32_t spend_count;      // Number of times attempted to spend (for tracking)
        bool is_dust;              // Below dust threshold
        bool is_change;            // Likely change output

        Analytics()
            : creation_time(0)
            , origin_output_index(0)
            , spend_count(0)
            , is_dust(false)
            , is_change(false)
        {}

        SERIALIZE_METHODS(Analytics, obj) {
            READWRITE(obj.creation_time, obj.origin_tx, obj.origin_output_index);
            READWRITE(obj.address_label, obj.category, obj.spend_count);
            READWRITE(obj.is_dust, obj.is_change);
        }
    };

    Coin base_coin;
    Analytics analytics;

    AnnotatedCoin() = default;

    AnnotatedCoin(Coin&& coin, const Txid& origin_tx, int output_index)
        : base_coin(std::move(coin))
    {
        analytics.creation_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        analytics.origin_tx = origin_tx;
        analytics.origin_output_index = output_index;
    }

    void SetCategory(const std::string& cat) { analytics.category = cat; }
    void SetAddressLabel(const std::string& label) { analytics.address_label = label; }
    void MarkAsDust(bool is_dust) { analytics.is_dust = is_dust; }
    void MarkAsChange(bool is_change) { analytics.is_change = is_change; }

    bool IsSpent() const { return base_coin.IsSpent(); }
    const Analytics& GetAnalytics() const { return analytics; }

    SERIALIZE_METHODS(AnnotatedCoin, obj) {
        READWRITE(obj.base_coin, obj.analytics);
    }
};

/**
 * MultiCoin - Variant type that can hold any custom coin type
 *
 * Allows polymorphic handling of different coin types.
 */
using MultiCoin = std::variant<
    Coin,           // Standard coin
    ExtendedCoin,   // Coin with metadata
    ColoredCoin,    // Asset/token coin
    TimeLockCoin,   // Time-locked coin
    AnnotatedCoin   // Analytics coin
>;

/**
 * Helper functions for working with custom coin types
 */
namespace CustomCoinHelpers {
    // Get the base coin from any custom coin type
    inline const Coin& GetBaseCoin(const MultiCoin& multi_coin) {
        return std::visit([](auto&& coin) -> const Coin& {
            using T = std::decay_t<decltype(coin)>;
            if constexpr (std::is_same_v<T, Coin>) {
                return coin;
            } else {
                return coin.base_coin;
            }
        }, multi_coin);
    }

    // Check if any coin type is spent
    inline bool IsSpent(const MultiCoin& multi_coin) {
        return std::visit([](auto&& coin) { return coin.IsSpent(); }, multi_coin);
    }

    // Get value from any coin type
    inline CAmount GetValue(const MultiCoin& multi_coin) {
        return std::visit([](auto&& coin) -> CAmount {
            using T = std::decay_t<decltype(coin)>;
            if constexpr (std::is_same_v<T, Coin>) {
                return coin.out.nValue;
            } else if constexpr (std::is_same_v<T, ColoredCoin>) {
                return coin.GetBTCValue();
            } else {
                return coin.base_coin.out.nValue;
            }
        }, multi_coin);
    }
}

#endif // BITCOIN_TEST_CUSTOM_COIN_TYPES_H
