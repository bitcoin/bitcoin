// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>            // For CHashWriter
#include <key.h>             // For CKey
#include <key_io.h>          // For DecodeDestination()
#include <pubkey.h>          // For CPubKey
#include <script/standard.h> // For CTxDestination, IsValidDestination(), PKHash
#include <serialize.h>       // For SER_GETHASH
#include <util/message.h>
#include <util/strencodings.h> // For DecodeBase64()
#include <outputtype.h>      // for OutputType
#include <script/proof.h>    // BIP-322

#include <string>
#include <vector>

MessageVerificationResult MessageVerify(
    const std::string& address,
    const std::string& signature,
    const std::string& message)
{
    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        return MessageVerificationResult::ERR_INVALID_ADDRESS;
    }

    bool invalid = false;
    std::vector<unsigned char> signature_bytes = DecodeBase64(signature.c_str(), &invalid);
    if (invalid) {
        return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    }

    using proof::Result;
    Result r = proof::VerifySignature(message, destination, signature_bytes);
    switch (r) {
    case Result::Inconclusive:
    case Result::Incomplete: return MessageVerificationResult::ERR_NOT_SIGNED;
    case Result::Invalid: return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    case Result::Error: return MessageVerificationResult::ERR_MALFORMED_SIGNATURE;
    default: return MessageVerificationResult::OK;
    }
}

bool MessageSign(
    const CKey& privkey,
    const std::string& message,
    const OutputType& address_type,
    std::string& signature)
{
    std::vector<unsigned char> signature_bytes;

    try {
        proof::SignMessageWithPrivateKey(privkey, address_type, message, signature_bytes);
    } catch (const proof::signing_error&) {
        return false;
    }

    signature = EncodeBase64(signature_bytes.data(), signature_bytes.size());

    return true;
}

uint256 MessageHash(const std::string& message)
{
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << MESSAGE_MAGIC << message;

    return hasher.GetHash();
}
