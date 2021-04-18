// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef WIDECOIN_UTIL_MESSAGE_H
#define WIDECOIN_UTIL_MESSAGE_H

#include <key.h> // For CKey
#include <uint256.h>

#include <string>

extern const std::string MESSAGE_MAGIC;

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
    OK
};

enum class SigningResult {
    OK, //!< No error
    PRIVATE_KEY_NOT_AVAILABLE,
    SIGNING_FAILED,
};

/** Verify a signed message.
 * @param[in] address Signer's widecoin address, it must refer to a public key.
 * @param[in] signature The signature in base64 format.
 * @param[in] message The message that was signed.
 * @return result code */
MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message);

/** Sign a message.
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
uint256 MessageHash(const std::string& message);

std::string SigningResultString(const SigningResult res);

#endif // WIDECOIN_UTIL_MESSAGE_H
