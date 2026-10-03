// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_PAYMENTDESTINATION_H
#define BITCOIN_COMMON_PAYMENTDESTINATION_H

#include <addresstype.h>
#include <common/bip352.h>
#include <script/script.h>
#include <util/expected.h>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

/**
 * A destination a payment can be made to. This is either a CTxDestination,
 * which maps to a fixed scriptPubKey, or a SilentPaymentsDestination, whose
 * scriptPubKey can only be derived once the inputs of the paying transaction
 * are known.
 *
 * A default constructed PaymentDestination holds a CNoDestination.
 */
class PaymentDestination
{
private:
    std::variant<CTxDestination, bip352::SilentPaymentsDestination> m_destination;

public:
    PaymentDestination() = default;
    /** Build a payment destination from a CTxDestination. This does not validate the CTxDestination. */
    explicit PaymentDestination(CTxDestination dest) : m_destination(std::move(dest)) {}
    explicit PaymentDestination(bip352::SilentPaymentsDestination dest) : m_destination(std::move(dest)) {}

    /**
     * Parse an address or a silent payments address. On failure, return an error
     * message and, if error_locations is not nullptr, set it to the indices of
     * likely error locations in the address, if known.
     */
    static util::Expected<PaymentDestination, std::string> FromString(std::string_view str, std::vector<int>* error_locations = nullptr);

    /** Return the CTxDestination, or nullptr if this is a silent payments destination. */
    const CTxDestination* GetTxDestination() const { return std::get_if<CTxDestination>(&m_destination); }

    /** Return the SilentPaymentsDestination, or nullptr if this is not a silent payments destination. */
    const bip352::SilentPaymentsDestination* GetSilentPaymentsDestination() const { return std::get_if<bip352::SilentPaymentsDestination>(&m_destination); }

    /** Whether this is a valid CTxDestination or a SilentPaymentsDestination */
    bool IsValid() const;

    /** Whether this is a silent payments destination. */
    bool IsSilentPayment() const { return std::holds_alternative<bip352::SilentPaymentsDestination>(m_destination); }

    /**
     * Return the scriptPubKey paid to, or std::nullopt for a silent payments
     * destination, whose scriptPubKey depends on the inputs of the paying transaction.
     */
    std::optional<CScript> GetStaticScript() const;

    friend bool operator==(const PaymentDestination&, const PaymentDestination&) = default;
    friend bool operator<(const PaymentDestination& a, const PaymentDestination& b) { return a.m_destination < b.m_destination; }
};

#endif // BITCOIN_COMMON_PAYMENTDESTINATION_H
