#pragma once

#include <mw/common/Macros.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/crypto/Commitment.h>
#include <mw/models/wallet/StealthAddress.h>

#include <amount.h>
#include <boost/optional.hpp>

MW_NAMESPACE

/// <summary>
/// Change outputs will use the stealth address generated using index 0
/// </summary>
static constexpr uint32_t CHANGE_INDEX{0};

/// <summary>
/// Peg-in outputs will use the stealth address generated using index 1.
/// </summary>
static constexpr uint32_t PEGIN_INDEX{1};

/// <summary>
/// Outputs sent to others will be marked with an address_index of 0xffffffff.
/// </summary>
static constexpr uint32_t UNKNOWN_INDEX{std::numeric_limits<uint32_t>::max()};

/// <summary>
/// Represents an output owned by the wallet, and the keys necessary to spend it.
/// </summary>
struct Coin : public Traits::ISerializable {
    // Version byte to more easily support adding new fields to the object.
    uint8_t version{1};

    // Index of the subaddress this coin was received at.
    uint32_t address_index{UNKNOWN_INDEX};

    // The private key needed in order to spend the coin.
    // May be empty for watch-only wallets.
    boost::optional<SecretKey> key;

    // The blinding factor of the coin's output.
    // May be empty for watch-only wallets.
    boost::optional<BlindingFactor> blind;

    // The output amount in litoshis.
    // Typically positive, but could be 0 in the future when we start using decoys to improve privacy.
    CAmount amount;

    // The output's ID (hash).
    mw::Hash output_id;

    // The ephemeral private key used by the sender to create the output.
    // This will only be populated when the coin has flag HAS_SENDER_INFO.
    boost::optional<SecretKey> sender_key;

    // The StealthAddress the coin was sent to.
    // This will only be populated when the coin has flag HAS_SENDER_INFO.
    boost::optional<StealthAddress> address;

    bool IsChange() const noexcept { return address_index == CHANGE_INDEX; }
    bool IsPegIn() const noexcept { return address_index == PEGIN_INDEX; }
    bool IsMine() const noexcept { return address_index != UNKNOWN_INDEX; }
    bool HasAddress() const noexcept { return !!address; }

    IMPL_SERIALIZABLE(Coin, obj)
    {
        READWRITE(obj.version);
        READWRITE(VARINT(obj.address_index));
        READWRITE(obj.key);
        READWRITE(obj.blind);
        READWRITE(VARINT_MODE(obj.amount, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(obj.output_id);

        if (obj.version >= 1) {
            READWRITE(obj.sender_key);
            READWRITE(obj.address);
        }
    }
};

END_NAMESPACE