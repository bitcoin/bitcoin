// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SIGNMESSAGE_H
#define BITCOIN_COMMON_SIGNMESSAGE_H

#include <addresstype.h>
#include <primitives/transaction.h>
#include <uint256.h>

#include <optional>
#include <string>
#include <vector>

class CKey;

extern const std::string MESSAGE_MAGIC;

enum class MessageSignatureFormat {
    //! Legacy format, which only works on legacy addresses
    LEGACY,

    //! Simple BIP-322 format, i.e. the script witness stack only
    SIMPLE,

    //! Full BIP-322 format, i.e. the serialized to_sign transaction in full
    FULL,
};

/** The result of a signed message verification.
 * Message verification takes as an input:
 * - address (with whose private key the message is supposed to have been signed)
 * - signature
 * - message
 */
enum class MessageVerificationResult {
    //! The provided address is invalid.
    ERR_INVALID_ADDRESS,

    //! The provided address is valid but does not refer to a public key.
    ERR_ADDRESS_NO_KEY,

    //! The provided signature couldn't be parsed (maybe invalid base64).
    ERR_MALFORMED_SIGNATURE,

    //! A public key could not be recovered from the provided signature and message.
    ERR_PUBKEY_NOT_RECOVERED,

    //! The message was not signed with the private key of the provided address.
    ERR_NOT_SIGNED,

    //! The message verification was successful.
    OK,

    //
    // BIP-322 extensions
    //

    //! The validator was unable to check the scripts (BIP-322)
    INCONCLUSIVE,

    //! Some check failed (BIP-322)
    ERR_INVALID,

    //! Proof of funds require the wallet-enabled verifier (BIP-322)
    ERR_POF,
};

enum class SigningResult {
    OK, //!< No error
    PRIVATE_KEY_NOT_AVAILABLE,
    SIGNING_FAILED,
};

/** Verify a signed message.
 * @param[in] address Signer's bitcoin address.
 * @param[in] signature The signature in base64 format.
 * @param[in] message The message that was signed.
 * @return result code */
MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message);

/** Sign a message using legacy format.
 * @param[in] privkey Private key to sign with.
 * @param[in] message The message to sign.
 * @param[out] signature Signature, base64 encoded, only set if true is returned.
 * @return true if signing was successful. */
bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    std::string& signature);

/**
 * Hashes a message for signing and verification in a manner that prevents
 * inadvertently signing a transaction.
 */
uint256 MessageHash(const std::string& message, MessageSignatureFormat format);

std::string SigningResultString(const SigningResult res);

/**
 * Generate the BIP-322 tx corresponding to the given challenge
 */
class BIP322Txs {
private:
    template<class T1, class T2>
    BIP322Txs(const T1& to_spend, const T2& to_sign) : m_to_spend{to_spend}, m_to_sign{to_sign} { }

public:
    static std::optional<BIP322Txs> Create(const CTxDestination& destination, const std::string& message, MessageVerificationResult& result, std::optional<const std::vector<unsigned char>> = std::nullopt);

    const CTransaction m_to_spend;
    const CTransaction m_to_sign;
};

#endif // BITCOIN_COMMON_SIGNMESSAGE_H
