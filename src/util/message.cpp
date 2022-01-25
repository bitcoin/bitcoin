// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <key.h>
#include <key_io.h>
#include <pubkey.h>
#include <script/standard.h>
#include <uint256.h>
#include <util/message.h>
#include <util/strencodings.h>

#include <cassert>
#include <optional>
#include <string>
#include <variant>
#include <vector>

/**
 * Text used to signify that a signed message follows and to prevent
 * inadvertently signing a transaction.
 */
const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";

/**
 * BIP-322 tagged hash
 */
static const HashWriter HASHER_BIP322{TaggedHash("BIP0322-signed-message")};

MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message)
{
    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        return MessageVerificationResult::ERR_INVALID_ADDRESS;
    }

    if (std::get_if<PKHash>(&destination) == nullptr) {
        return MessageVerificationResult::ERR_ADDRESS_NO_KEY;
    }

    auto signature_bytes = DecodeBase64(signature);
    if (!signature_bytes) {
        return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    }

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(MessageHash(message, MessageSignatureFormat::LEGACY), *signature_bytes)) {
        return MessageVerificationResult::ERR_PUBKEY_NOT_RECOVERED;
    }

    if (!(CTxDestination(PKHash(pubkey)) == destination)) {
        return MessageVerificationResult::ERR_NOT_SIGNED;
    }

    return MessageVerificationResult::OK;
}

bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    std::string& signature)
{
    std::vector<unsigned char> signature_bytes;

    if (!privkey.SignCompact(MessageHash(message, MessageSignatureFormat::LEGACY), signature_bytes)) {
        return false;
    }

    signature = EncodeBase64(signature_bytes);

    return true;
}

uint256 MessageHash(const std::string& message, MessageSignatureFormat format)
{
    switch (format) {
    case MessageSignatureFormat::LEGACY:
        {
            HashWriter hasher{};
            hasher << MESSAGE_MAGIC << message;
            return hasher.GetHash();
        }

    case MessageSignatureFormat::SIMPLE:
    case MessageSignatureFormat::FULL:
        {
            HashWriter hasher{HASHER_BIP322};
            if (!message.empty()) {
                hasher.write(AsBytes(Span{message.data(), message.size() * sizeof(char)}));
            }
            return hasher.GetSHA256();
        }
    }
    assert(false);
}

std::string SigningResultString(const SigningResult res)
{
    switch (res) {
        case SigningResult::OK:
            return "No error";
        case SigningResult::PRIVATE_KEY_NOT_AVAILABLE:
            return "Private key not available";
        case SigningResult::SIGNING_FAILED:
            return "Sign failed";
        // no default case, so the compiler can warn about missing cases
    }
    assert(false);
}
