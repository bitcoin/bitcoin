#pragma once

#include <mw/common/Macros.h>
#include <mw/models/crypto/BlindingFactor.h>
#include <mw/models/crypto/Commitment.h>

#include <amount.h>
#include <boost/optional.hpp>

MW_NAMESPACE

/// <summary>
/// Change outputs will use the stealth address generated using index 2,000,000.
/// </summary>
static constexpr uint32_t CHANGE_INDEX{0};

/// <summary>
/// Peg-in outputs will use the stealth address generated using index 2,000,000.
/// </summary>
static constexpr uint32_t PEGIN_INDEX{1};

/// <summary>
/// Represents an output owned by the wallet, and the keys necessary to spend it.
/// </summary>
struct Coin : public Traits::ISerializable {
    // Version byte to more easily support adding new fields to the object.
    uint8_t version{0};

    // Index of the subaddress this coin was received at.
    uint32_t address_index;

    // The private key needed in order to spend the coin.
    // May be empty for watch-only wallets.
    boost::optional<SecretKey> key;

    // The blinding factor needed in order to spend the coin.
    // May be empty for watch-only wallets.
    boost::optional<BlindingFactor> blind;

    // The output amount in litoshis.
    // Typically positive, but could be 0 in the future when we start using decoys to improve privacy.
    CAmount amount;

    // The output's ID (hash).
    mw::Hash output_id;

    bool IsChange() const noexcept { return address_index == CHANGE_INDEX; }
    bool IsPegIn() const noexcept { return address_index == PEGIN_INDEX; }

    IMPL_SERIALIZABLE(Coin, obj)
    {
        READWRITE(obj.version);
        READWRITE(VARINT(obj.address_index));
        READWRITE(obj.key);
        READWRITE(obj.blind);
        READWRITE(VARINT_MODE(obj.amount, VarIntMode::NONNEGATIVE_SIGNED));
        READWRITE(obj.output_id);
    }
};

END_NAMESPACE