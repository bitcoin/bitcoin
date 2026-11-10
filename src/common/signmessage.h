// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SIGNMESSAGE_H
#define BITCOIN_COMMON_SIGNMESSAGE_H

#include <addresstype.h>
#include <primitives/transaction.h>
#include <script/script_error.h>
#include <uint256.h>

#include <string>
#include <string_view>

class CKey;
class CScript;

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
 * @param[in] address Signer's bitcoin address, it must refer to a public key.
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

/** Hash raw message bytes with the BIP0322-signed-message tag. */
uint256 BIP322MessageHash(std::string_view message);

/** The virtual transactions used by BIP322 simple signing and verification. */
struct BIP322Transactions {
    CMutableTransaction to_spend;
    CMutableTransaction to_sign;
};

/** Construct the zero-valued BIP322 simple transactions for a challenge script. */
BIP322Transactions CreateBIP322Transactions(const CScript& message_challenge, std::string_view message);

enum class BIP322VerificationResult {
    OK,
    INVALID,
    INCONCLUSIVE,
    UNSUPPORTED_ADDRESS,
    UNSUPPORTED_FORMAT,
    MALFORMED_BASE64,
    MALFORMED_WITNESS,
};

/** Verify a prefixed or pre-final unprefixed BIP322 simple proof, without chain or wallet data.
 * Only OK establishes a valid proof. Inconclusive upgrade mechanisms never establish success.
 * script_error, when provided, receives the failing interpreter rule for an invalid proof.
 */
BIP322VerificationResult BIP322Verify(const CTxDestination& destination, std::string_view signature,
                                    std::string_view message, ScriptError* script_error = nullptr);

std::string SigningResultString(SigningResult res);

#endif // BITCOIN_COMMON_SIGNMESSAGE_H
